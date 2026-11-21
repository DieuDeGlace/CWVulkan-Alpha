// Checks the network code: first the byte-level test vectors of docs/specs/alpha-net-packets.md
// §6, then a real server on a local port with two clients that see each other, chat, and see
// each other leave. Uses only the loopback interface.
#include "Bytes.h"
#include "ZlibStream.h"
#include "Saves.h"
#include "net/Client.h"
#include "net/RemoteCreatures.h"
#include "ItemRequest.h"
#include "net/Server.h"
#include "worldgen/Random.h"
#include <bit>
#include <chrono>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <set>
#include <thread>
#include <algorithm>

using namespace net;

static int failures = 0;
static void check(bool ok, const std::string &what) {
    if (!ok) {
        failures++;
        std::printf("FAIL %s\n", what.c_str());
    }
}

static std::vector<uint8_t> hex(const char *text) {
    std::vector<uint8_t> out;
    for (const char *p = text; p[0] && p[1];) {
        if (*p == ' ') {
            p++;
            continue;
        }
        out.push_back((uint8_t)std::strtoul(std::string(p, 2).c_str(), nullptr, 16));
        p += 2;
    }
    return out;
}

static std::string hexOf(const std::vector<uint8_t> &bytes) {
    std::string out;
    char two[4];
    for (uint8_t b : bytes) {
        std::snprintf(two, sizeof two, "%02x", b);
        out += two;
    }
    return out;
}

static bool waitFor(const std::function<bool()> &condition, int ms = 3000) {
    const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
    while (std::chrono::steady_clock::now() < end) {
        if (condition())
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return condition();
}

static void checkIdentityAdoption() {
    Client client;
    check(client.id() == 1, "a fresh client starts from the native literal identity1");
    for (const auto [replyId, expected] : std::array<std::pair<uint64_t, uint64_t>, 5>{{
            {7, 7}, {UINT64_MAX, 7}, {0, 0}, {uint64_t(INT64_MIN), 0}, {5, 5}}}) {
        Socket listener;
        if (!listener.listen(0, nullptr, "127.0.0.1")) {
            check(false, "identity fixture listens");
            return;
        }
        const uint64_t remoteKey = replyId & (uint64_t(1) << 63) ? replyId : 99;
        std::atomic<bool> finish{false}, receivedOwn{false};
        std::atomic<uint64_t> sentIdentity{0};
        std::thread peer([&] {
            Socket socket = listener.accept();
            if (!socket.valid())
                return;
            socket.setReadTimeout(3000);
            uint32_t packet = 0, version = 0;
            if (!socket.readU32(packet) || packet != HELLO || !socket.readU32(version) || version != VERSION)
                return;
            const auto initial = CreatureRecord::nativeDefaults();
            ByteWriter response;
            response.u32(ACCEPTED);
            response.u32(CREATURE);
            response.u64(replyId);
            response.write(initial.bytes.data(), initial.bytes.size());
            response.u32(SEED);
            response.u32(41);
            CreatureRecord remote = initial;
            const float health = 31;
            std::memcpy(remote.bytes.data() + 0x15c, &health, 4);
            response.u32(CREATURE);
            response.bytes(compressed(encodeDelta(remoteKey, remote, uint64_t(1) << 27)));
            if (!socket.send(response.data()))
                return;
            CreatureRecord own;
            while (!finish && socket.readU32(packet)) {
                if (packet != CREATURE)
                    return;
                uint32_t length = 0;
                if (!socket.readU32(length) || length > (4u << 20))
                    return;
                std::vector<uint8_t> packed(length);
                if (!socket.readExact(packed.data(), packed.size()))
                    return;
                const auto plain = zlibInflate(packed);
                const auto id = plain ? applyDelta(*plain, own) : std::nullopt;
                if (!id)
                    return;
                sentIdentity = *id;
                receivedOwn = true;
            }
        });
        const bool joined = client.connect("127.0.0.1", listener.boundPort());
        check(joined, "a complete identity reply continues connection setup");
        if (joined) {
            check(client.id() == expected, "nonnegative replies adopt the exact ID, while negative replies retain the prior identity");
            check(waitFor([&] { return receivedOwn.load(); }) && sentIdentity == expected,
                  "the actual sender uses the adopted or retained identity");
            check(waitFor([&] {
                const auto actors = client.others();
                return actors.contains(remoteKey) && actors.at(remoteKey).health() == 31;
            }), "a negative handshake does not misclassify a later negative-key remote actor as controlled");
        }
        finish = true;
        client.disconnect();
        listener.close();
        peer.join();
        check(client.id() == expected, "disconnect preserves the last adopted identity");
    }
}

static void checkWorldRequests() {
    Socket listener;
    if (!listener.listen(0, nullptr, "127.0.0.1")) {
        check(false, "world-change fixture listens");
        return;
    }
    constexpr uint64_t local = 0x100000003ull, firstActor = 17, intermediateActor = 19;
    constexpr uint32_t negativeSeed = 0xfffffff9u;
    const WorldIdentity initial{41, "online_41"}, final{negativeSeed, "online_-7"};
    const auto actor = CreatureRecord::nativeDefaults();
    const auto mission = MissionArea{}.record(-1, 0);
    creatures::Projectile shot;
    shot.shooter = firstActor;
    shot.age = -12;
    const auto projectile = shot.serialize();
    std::atomic<int> stage{0};
    std::thread peer([&] {
        Socket socket = listener.accept();
        if (!socket.valid())
            return;
        uint32_t id = 0, version = 0;
        if (!socket.readU32(id) || id != HELLO || !socket.readU32(version) || version != VERSION)
            return;
        const auto writeActor = [&](ByteWriter &result, uint64_t key) {
            result.u32(CREATURE);
            result.bytes(compressed(encodeDelta(key, actor, uint64_t(1) << 27)));
        };
        const auto writeClock = [](ByteWriter &result, uint32_t day, uint32_t time) {
            result.u32(CLOCK);
            result.u32(day);
            result.u32(time);
        };
        ByteWriter result;
        result.u32(ACCEPTED);
        result.u32(CREATURE);
        result.u64(local);
        result.write(actor.bytes.data(), actor.bytes.size());
        result.u32(SEED);
        result.u32(initial.seed);
        writeActor(result, firstActor);
        if (!socket.send(result.data()) || !waitFor([&] { return stage.load() >= 1; }))
            return;
        result.data().clear();
        result.u32(SEED);
        result.u32(initial.seed);
        writeClock(result, 5, 160);
        if (!socket.send(result.data()) || !waitFor([&] { return stage.load() >= 2; }))
            return;
        result.data().clear();
        WorldUpdate update;
        update.sections[4].emplace_back(projectile.begin(), projectile.end());
        update.sections[12].emplace_back(mission.begin(), mission.end());
        result.u32(WORLD_UPDATE);
        result.bytes(compressed(update.encode()));
        writeClock(result, 0xffffffffu, 46123459);
        result.u32(SEED);
        result.u32(55);
        writeActor(result, intermediateActor);
        result.u32(SEED);
        result.u32(negativeSeed);
        if (socket.send(result.data()))
            waitFor([&] { return stage.load() >= 3; });
    });

    Client client;
    const bool connected = client.connect("127.0.0.1", listener.boundPort());
    check(connected, "world-change client connects");
    if (connected) {
        check(client.worldIdentity() == initial && waitFor([&] { return client.others().contains(firstActor); }),
              "initial signed name accompanies the received actor");
        stage = 1;
        uint32_t day = 0, time = 0;
        check(waitFor([&] { return client.clock(day, time) && day == 5; }) && time == 160 &&
              client.others().contains(firstActor) && !client.worldChange(initial),
              "a repeated requested pair preserves the remote actor and produces no world change");
        stage = 2;
        check(waitFor([&] { return client.worldIdentity() == final; }), "consecutive type-15 messages publish the last complete pair");
        auto frame = client.lockFrame();
        check(client.connected() && client.id() == local && client.others().empty() && client.takeReceipts().empty(),
              "new requests remove the old remote lifetimes without replacing the connection or local key");
        check(client.worldChange(initial) == final && client.worldChange({negativeSeed, "online_7"}) == final &&
              !client.worldChange(final), "the frame compares seed and signed name as one exact pair");
        const auto received = client.takeCombatReceipts();
        check(received.shots.size() == 1 && received.shots[0] == projectile &&
              received.missions.size() == 1 && received.missions[0] == mission &&
              client.clock(day, time) && day == UINT32_MAX && time == 46123459,
              "queued world records and the live connected clock survive a world request");
    }
    stage = 3;
    client.disconnect();
    listener.close();
    peer.join();
}

// Packets §0.1, §3.2-§3.4, §3.10: the client's per-tick order, and the packets an Alpha server
// never sends but its client understands.
static void checkClientPackets() {
    Socket listener;
    if (!listener.listen(0, nullptr, "127.0.0.1")) { check(false, "packet fixture listens"); return; }
    std::atomic<bool> deleteAll{false}, finish{false};
    std::atomic<int> itemBeforeHit{0}; // 1 in order, -1 out of order
    std::atomic<int> requestOrder{0};
    const std::pair<int32_t, int32_t> heldKey{33, 44}, absentKey{34, 44};
    const std::array<int32_t, 4> firstObject{0, 8500, 11400, -2}, secondObject{11, 8511, 11415, 8};
    const std::vector<Client::ZonePlacements> expectedPlacements{
        {heldKey, {firstObject}}, {absentKey, {secondObject}}, {heldKey, {firstObject, secondObject}}, {heldKey, {}}};
    const std::array<std::array<int32_t, 3>, 4> requestPass{{
        {ZONE_REQUEST, 33, 44}, {ZONE_REQUEST, 34, 44}, {ZONE_REQUEST, 33, 44}, {REGION_REQUEST, 1, 2}}};
    std::thread peer([&] {
        Socket socket = listener.accept();
        socket.setReadTimeout(3000);
        uint32_t packet = 0, version = 0;
        if (!socket.readU32(packet) || packet != HELLO || !socket.readU32(version)) return;
        const CreatureRecord initial;
        ByteWriter w;
        w.u32(ACCEPTED), w.u32(CREATURE), w.u64(1);
        w.write(initial.bytes.data(), initial.bytes.size());
        w.u32(SEED), w.u32(7);
        const std::vector<uint8_t> object(120, 0xab);
        w.u32(STATIC_OBJECTS), w.i32(0);
        w.u32(STATIC_OBJECTS), w.i32(1), w.write(object.data(), object.size());
        w.u32(CHAT), w.u64(19), w.i32(-1); // nothing more is read
        WorldUpdate update;
        std::vector<uint8_t> burst(72, 0);
        burst[8] = 1; // (a target id of the local player, were it read as a hit)
        update.sections[2].push_back(burst);
        for (const auto &[key, records] : expectedPlacements) {
            WorldUpdate::KeyedList snapshot;
            snapshot.key = uint64_t(uint32_t(key.first)) | (uint64_t(uint32_t(key.second)) << 32);
            for (const auto &record : records) {
                ByteWriter bytes;
                for (const int32_t field : record)
                    bytes.i32(field);
                snapshot.records.push_back(std::move(bytes.data()));
            }
            update.keyed[1].push_back(std::move(snapshot));
        }
        w.u32(WORLD_UPDATE), w.bytes(compressed(update.encode()));
        CreatureRecord remote;
        remote.setPosition(0, 4242);
        w.u32(CREATURE_BATCH), w.i32(3);
        for (const uint64_t id : {uint64_t(19), uint64_t(23), uint64_t(1)}) {
            w.u64(id), w.u64(1);
            w.write(remote.bytes.data(), 24);
        }
        if (!socket.send(w.data())) return;
        bool item = false;
        size_t requestsSeen = 0;
        bool requestSequenceMatches = true;
        while (!finish && (itemBeforeHit == 0 || requestOrder == 0) && socket.readU32(packet)) {
            uint32_t skip = 0;
            switch (packet) {
            case CREATURE: if (!socket.readU32(skip)) return; break;
            case CHAT: if (!socket.readU32(skip)) return; skip *= 2; break;
            case ITEM_RECORD: item = true, skip = 300; break;
            case HIT: itemBeforeHit = item ? 1 : -1; skip = 72; break;
            case EVENT40: skip = 40; break;
            case SHOOT: skip = 112; break;
            case ZONE_REQUEST: case REGION_REQUEST: skip = 8; break;
            default: return;
            }
            std::vector<uint8_t> bytes(skip);
            if (!socket.readExact(bytes.data(), skip)) return;
            if ((packet == ZONE_REQUEST || packet == REGION_REQUEST) && requestOrder == 0) {
                ByteReader r(bytes);
                const std::array<int32_t, 3> request{(int32_t)packet, r.i32(), r.i32()};
                requestSequenceMatches &= request == requestPass[requestsSeen++ % requestPass.size()];
                if (requestsSeen == 2 * requestPass.size())
                    requestOrder = requestSequenceMatches ? 1 : -1;
            }
        }
        while (!deleteAll && !finish) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        ByteWriter none;
        none.u32(CREATURE_BATCH), none.i32(-1);
        if (!socket.send(none.data())) return;
        while (!finish) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });
    Client client;
    check(client.connect("127.0.0.1", listener.boundPort()), "packet fixture connects");
    {
        auto frame = client.lockFrame();
        client.sendHit(Hit::make(1, 19, 1, 0, {}, 0));
        client.sendItemRequest({});
        client.sendItemRequest(ItemRequest{ItemRequest::UseStatic, {}, {1, 2, 3}}.packet());
        client.sendItemRequest(ItemRequest{ItemRequest::CallPet}.packet());
        client.sendItemRequest(ItemRequest{ItemRequest::UseStatic, {}, {3, 2, 1}}.packet());
        client.requestZones(std::array{heldKey, absentKey, heldKey});
        client.requestRegions(std::array{std::pair<int32_t, int32_t>{1, 2}});
        const std::vector<std::array<int32_t, 3>> expected{{1, 2, 3}, {3, 2, 1}};
        check(client.pendingStaticRequests() == expected && client.pendingStaticRequests() == expected,
              "online frame sampling keeps the pending static subsequence on the sender's queue");
    }
    check(waitFor([&] {
        const auto others = client.others();
        return others.size() == 2 && others.count(19) && others.count(23) && others.at(19).position(0) == 4242;
    }), "packet 1 decodes uncompressed entries onto new creatures and skips the local player");
    const auto receipts = client.takeReceipts();
    check(receipts.size() == 2 && receipts[0].snap && receipts[1].snap, "creatures created by packet 1 snap");
    check(waitFor([&] { return itemBeforeHit != 0; }) && itemBeforeHit == 1, "packet 6 is sent before the tick's hits");
    check(waitFor([&] { return requestOrder != 0; }, 5000) && requestOrder == 1,
          "two sender turns retain and repeat the same ordered packet-11 and packet-12 snapshots");
    client.requestZones({});
    client.requestRegions({});
    check(client.takeHits().empty() && client.takeBursts().size() == 1, "section 3 is not passed to hit handling");
    const auto placements = client.takeZonePlacements();
    check(placements == expectedPlacements && client.takeZonePlacements().empty(),
          "section 8 keeps signed fields, repeated keys and empty inner snapshots in order");
    if (placements == expectedPlacements) {
        worldgen::WorldMap world;
        auto held = std::make_shared<worldgen::Zone>(heldKey.first, heldKey.second);
        held->statics.emplace_back();
        world.add(held);
        check(world.needsZoneContents(heldKey) && !world.needsZoneContents(absentKey),
              "only a held pending zone can be requested");
        for (size_t k = 0; k < 3; k++)
            world.replacePlacedObjects(placements[k].first, placements[k].second);
        const auto *placed = world.placedObjects(heldKey);
        check(placed && *placed == placements[2].second && !world.placedObjects(absentKey) &&
                  !world.needsZoneContents(heldKey),
              "the latest complete list replaces the prior list and acknowledges only a resident zone");
        world.add(std::make_shared<worldgen::Zone>(*held));
        check(!world.needsZoneContents(heldKey), "terrain-only replacement keeps a zone's request acknowledgement");
        world.replacePlacedObjects(heldKey, placements.back().second);
        placed = world.placedObjects(heldKey);
        check(placed && placed->empty() && !world.needsZoneContents(heldKey), "an empty section 8 clears the live list");
        world.remove(heldKey.first, heldKey.second);
        world.add(held);
        check(world.needsZoneContents(heldKey) && !world.placedObjects(heldKey), "unload discards the placed list and latch");
        StaticStates states;
        states.generated(*held);
        const auto valid = states.record(*held, 0);
        auto rejected = valid;
        const int32_t invalidIndex = 1;
        std::memcpy(rejected.data() + 8, &invalidIndex, 4);
        check(!states.apply(rejected.data(), &world) && world.needsZoneContents(heldKey),
              "an invalid static index leaves a held zone's request pending");
        if (const auto key = states.apply(valid.data(), &world))
            world.acknowledgeZoneContents(*key);
        check(!world.needsZoneContents(heldKey), "a valid static response can satisfy the latch without a placement");
    }
    check(client.takeChat().empty() && client.connected(),
          "packet 3 (count 0 and 1) and a negative chat count keep the stream in step");
    deleteAll = true;
    check(waitFor([&] { return client.others().empty(); }), "a packet-1 count <= 0 deletes every other creature");
    finish = true;
    client.disconnect();
    peer.join();
}

static void checkCreatureReceiptPoses() {
    Socket listener;
    if (!listener.listen(0, nullptr, "127.0.0.1")) {
        check(false, "receipt pose fixture listens");
        return;
    }
    std::atomic<bool> finish{false}, sendBatch{false}, sendEnd{false}, sendPartial{false};
    const uint64_t actor = UINT64_MAX, earlier = UINT64_MAX - 1, later = 7;
    const auto poseRecord = [](int64_t x, float yaw, glm::vec3 aim) {
        auto record = CreatureRecord::nativeDefaults();
        record.setPosition(0, x);
        record.setPosition(1, x + 1024);
        record.setPosition(2, x + 2048);
        record.setMotion({}, yaw);
        std::memcpy(record.bytes.data() + 0x150, &aim, sizeof aim);
        return record;
    };
    const auto first = poseRecord(100000, 10.f, {1, 0, 0});
    auto snapped = poseRecord(200000, 20.f, {2, 0, 0});
    snapped.bytes[0x1b0] = 1;
    auto smooth = poseRecord(300000, 30.f, {3, 0, 0});
    smooth.bytes[0x1b0] = 1;
    const auto recreated = poseRecord(400000, 40.f, {4, 0, 0});
    auto latest = poseRecord(500000, 50.f, {5, 0, 0});
    latest.setMotion({6, 4, 0}, glm::vec3(15, 5, 50), {0.05f, 0, 0}, {4, 2, 0});
    latest.setHeadPitch(12.f);
    auto latestFighter = latest.fighter();
    latestFighter.action = 59;
    latestFighter.actionMs = 16777217;
    latestFighter.hitStun = 20;
    latestFighter.slowMs = 20;
    latestFighter.hasteMs = 21;
    latestFighter.recoveryPenalty = -0.25f;
    latestFighter.mode = 0x8400;
    latestFighter.mana = 0.2f;
    latestFighter.charge = 0.3f;
    latestFighter.blockGauge = 0.4f;
    latest.setFighter(latestFighter);
    std::thread peer([&] {
        Socket socket = listener.accept();
        socket.setReadTimeout(3000);
        uint32_t packet = 0, version = 0;
        if (!socket.readU32(packet) || packet != HELLO || !socket.readU32(version) || version != VERSION)
            return;
        ByteWriter wire;
        const auto initial = CreatureRecord::nativeDefaults();
        wire.u32(ACCEPTED), wire.u32(CREATURE), wire.u64(1);
        wire.write(initial.bytes.data(), initial.bytes.size());
        wire.u32(SEED), wire.u32(1);
        const auto append = [&](const CreatureRecord &record, const CreatureRecord *previous) {
            wire.u32(CREATURE);
            wire.bytes(compressed(encodeDelta(actor, record, changedFields(record, previous))));
        };
        append(first, nullptr);
        append(snapped, &first);
        append(smooth, &snapped);
        wire.u32(CREATURE_BATCH), wire.i32(0); // drop the actor before it returns at the same key
        append(recreated, nullptr);
        append(latest, &recreated);
        if (!socket.send(wire.data()))
            return;
        while (!sendBatch && !finish)
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        if (finish)
            return;
        ByteWriter batch;
        batch.u32(CREATURE_BATCH);
        batch.i32(5);
        auto record = poseRecord(250000, 60.f, {});
        batch.bytes(encodeDelta(later, record, 1)); // deliberately out of signed order
        batch.bytes(encodeDelta(earlier, record, 1));
        record.bytes[0x1b0] = 1;
        batch.bytes(encodeDelta(actor, record, uint64_t(1) << 40)); // temporary snap field change
        record.bytes[0x1b0] = 0;
        record.setHealth(123.f);
        constexpr uint64_t health = uint64_t(1) << 27;
        batch.bytes(encodeDelta(actor, record, (uint64_t(1) << 40) | health));
        batch.bytes(encodeDelta(earlier, record, 2 | health));
        if (!socket.send(batch.data()))
            return;
        while (!sendEnd && !finish)
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        if (finish || !socket.sendU32(END_OF_BATCH))
            return;
        while (!sendPartial && !finish)
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        if (finish)
            return;
        ByteWriter partial;
        auto delta = latest;
        delta.setHealth(240.f);
        partial.u32(CREATURE);
        partial.bytes(compressed(encodeDelta(actor, delta, health)));
        delta.bytes[0x1b0] = 2;
        partial.u32(CREATURE_BATCH), partial.i32(1);
        partial.bytes(encodeDelta(actor, delta, uint64_t(1) << 40));
        if (socket.send(partial.data()))
            while (!finish)
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
    });
    Client client;
    const bool connected = client.connect("127.0.0.1", listener.boundPort());
    check(connected, "receipt pose fixture connects");
    if (connected) {
        const bool received = waitFor([&] {
            const auto actors = client.others();
            const auto found = actors.find(actor);
            return found != actors.end() && found->second.position(0) == latest.position(0);
        });
        check(received, "the wire reaches the smooth update of the recreated actor");
        if (received) {
            const auto receipts = client.takeReceipts();
            check(receipts.size() == 5, "every applied packet produces one ordered receipt");
            if (receipts.size() == 5) {
                check(receipts[0].id == actor && receipts[0].initialAim == first.look() &&
                          receipts[1].snap && receipts[1].snap->position == std::array<int64_t, 3>{200000, 201024, 202048} &&
                          receipts[1].snap->rotation.z == 20.f && !receipts[1].initialAim && !receipts[2].snap,
                      "a changed snap field retains that packet's pose even with a newer smooth receipt");
                check(receipts[3].initialAim == recreated.look() && receipts[3].snap &&
                          receipts[3].snap->position[0] == 400000 && !receipts[4].snap,
                      "a deleted key's new incarnation carries its own initial pose and aim");
                const auto record = client.others().at(actor);
                net::RemoteCreatures remotes;
                for (const auto &receipt : receipts)
                    remotes.receive(receipt);
                RemoteFighter state;
                remotes.updateLive(actor, record, state);
                const auto &remote = *remotes.find(actor);
                check(remote.live.follow[0] == 400000 && remote.drawnRotation.z == 40.f &&
                          remote.drawnLook == recreated.look() && remote.smoothing &&
                          remote.live.position[0] == 500000 && remote.live.yaw == 50.f,
                      "a single frame preserves the recreated snap and newest live pose separately");
            }
        }
        sendBatch = true;
        const bool batched = waitFor([&] {
            const auto actors = client.others();
            return actors.size() == 3 && actors.contains(earlier) && actors.at(earlier).health() == 123.f;
        });
        check(batched, "the repeated-key batch reaches its final publication");
        if (batched) {
            const auto actors = client.others();
            check(actors.at(earlier).position(0) == 250000 && actors.at(earlier).yaw() == 60.f &&
                      actors.at(actor).position(0) == 500000 && actors.at(actor).health() == 123.f,
                  "packet 1 keeps earlier occurrences' fields and the seeded live record");
            const auto receipts = client.takeReceipts();
            check(receipts.size() == 3 && receipts[0].id == earlier && receipts[1].id == actor && receipts[2].id == later,
                  "packet 1 commits each distinct key once in signed order");
            if (receipts.size() == 3) {
                check(receipts[0].snap && receipts[0].snap->position[0] == 250000 &&
                          receipts[0].snap->rotation.z == 60.f && !receipts[1].snap && !receipts[1].initialAim,
                      "packet 1 makes one snap decision from the final merged record");
            }
        }
        sendEnd = true;
        const bool pruned = waitFor([&] {
            const auto actors = client.others();
            return actors.size() == 1 && actors.contains(actor);
        });
        check(pruned, "packet 2 keeps the packet-0 key and prunes keys listed only by packet 1");
        if (pruned) {
            const auto record = client.others().at(actor);
            creatures::ActorMap<RemoteFighter> fighters;
            auto &state = fighters[actor];
            RemoteCreatures remotes;
            remotes.receive(CreatureReceipt::fromRecord(actor, recreated, true, true));
            remotes.receive(CreatureReceipt::fromRecord(actor, latest, false, false));
            remotes.updateLive(actor, record, state);
            const creatures::BlockType air = [](int32_t, int32_t, int32_t) { return uint8_t(0); };
            remotes.step(fighters, 20, air, {});
            const auto &advanced = *remotes.find(actor);
            {
                auto frame = client.lockFrame();
                client.publishReceivedState(remotes, fighters);
            }
            const auto motion = client.others().at(actor);
            const auto &body = advanced.live;
            const std::array<int64_t, 3> position{body.position[0], body.position[1], body.position[2]};
            const glm::vec3 orientation(body.swimPitch, body.bank, body.yaw);
            const size_t motionEnd = CREATURE_FIELDS[7].offset;
            const auto liveFighter = motion.fighter();
            const auto inputFighter = record.fighter();
            // Restore the modeled fields and ensure the rest of the record is untouched.
            CreatureRecord restored = motion;
            std::copy_n(record.bytes.begin(), motionEnd, restored.bytes.begin());
            restored.setActionTimers(inputFighter);
            restored.setMovementModes(inputFighter);
            restored.setCombatResources(inputFighter);
            restored.setHealth(record.health());
            check(motion.position(0) != latest.position(0) && motion.position(0) != body.follow[0] &&
                      motion.position(0) == position[0] && motion.position(1) == position[1] && motion.position(2) == position[2] &&
                      motion.orientation() == orientation && motion.velocity() == body.velocity &&
                      motion.intent() == body.intent && motion.response() == body.push && motion.headPitch() == advanced.headPitch &&
                      motion.contactFlags() == body.contact &&
                      restored.bytes == record.bytes,
                  "frame publication only updates the modeled fields, using the live motion rather than the drawn pose");
            check(liveFighter.action == 59 && liveFighter.actionMs == 16777237 && liveFighter.hitStun == 0 &&
                      liveFighter.slowMs == 0 && liveFighter.hasteMs == 1 &&
                      liveFighter.recoveryPenalty == -0.25f + (20.f * 0.001f) / 180.f,
                  "live action, stun and recovery use the duration of the signed actor's world turn");
            check(liveFighter.mode == 0x8000 && liveFighter.mana == 0.2f && liveFighter.charge == 0.2f &&
                      liveFighter.blockGauge == 0.4f + 20.f / 2000.f,
                  "movement clears the intended toggle and publishes the resource cap and blocking recovery");
            sendPartial = true;
            const bool partial = waitFor([&] {
                const auto actors = client.others();
                const auto found = actors.find(actor);
                return found != actors.end() && found->second.bytes[0x1b0] == 2;
            });
            check(partial, "the partial packet 0 and packet 1 records both arrive");
            if (partial) {
                const auto current = client.others().at(actor);
                const auto currentFighter = current.fighter();
                check(current.health() == 240.f &&
                          std::equal(motion.bytes.begin(), motion.bytes.begin() + motionEnd, current.bytes.begin()) &&
                          currentFighter.actionMs == liveFighter.actionMs && currentFighter.hitStun == liveFighter.hitStun &&
                          currentFighter.hasteMs == liveFighter.hasteMs && currentFighter.recoveryPenalty == liveFighter.recoveryPenalty &&
                          currentFighter.mode == liveFighter.mode && currentFighter.charge == liveFighter.charge &&
                          currentFighter.mana == liveFighter.mana && currentFighter.blockGauge == liveFighter.blockGauge,
                      "both packet forms preserve advanced motion, resources and action clocks omitted by their masks");
                const auto receipts = client.takeReceipts();
                check(receipts.size() == 2 && !receipts[0].snap && receipts[1].snap &&
                          receipts[1].snap->position == position && receipts[1].snap->rotation == orientation,
                      "a partial batch snap captures the advanced live pose from its temporary baseline");
            }
        }
    }
    finish = true;
    client.disconnect();
    listener.close();
    peer.join();
}

static void checkCombatTransport() {
    Socket listener;
    std::string bindError;
    check(!listener.listen(0, &bindError, "not-an-ip"), "invalid bind address is rejected without a fallback listener");
    if (!listener.listen(0, nullptr, "127.0.0.1")) { check(false, "buff fixture listens"); return; }
    const creatures::BuffEvent first{77, 1, {11, 1, 30000, 0}};
    creatures::BuffEvent second{88, 19, {4, .25f, 16777217, 0xfeed001122334455}};
    second.record.typePadding = {3, 7, 11};
    second.record.timePadding = {13, 17, 19, 23};
    const creatures::BuffEvent sent{1, 19, {6, 8, -17, 123456789}};
    const auto expectedSend = sent.serialize();
    const std::array<creatures::RelationshipRecord, 3> relationships{{
        {19, 0, -3.25f, {1, 2, 3, 4}}, {19, 1, -0.f, {5, 6, 7, 8}}, {1, 1, 17, {9, 10, 11, 12}}}};
    const std::array<creatures::KillRecord, 5> expectedCredits{{
        {1, 19, 999, 17}, {0, 1, INT32_MIN, -7}, {19, 0, -1, 0},
        {1, 1, INT32_MAX, 50}, {1, 19, 999, -17}}};
    Missions regional;
    auto &mission = regional.edit(0, 0).mission;
    mission.kind = 7; mission.required = 3;
    creatures::Combatant regionTarget, contributor;
    regionTarget.backing = {0, 0, 0};
    regionTarget.appearanceFlags = 0x2000;
    regionTarget.relations.threats[0] = 1;
    std::vector<MissionProgress> progress;
    for (int i = 0; i < 2; i++)
        if (const auto event = regional.fatal(regionTarget, 0, 0,
                [](int32_t, int32_t) { return true; },
                [&](uint64_t key) { return key == 0 ? &contributor : nullptr; }))
            progress.push_back(*event);
    std::vector<std::vector<uint8_t>> expectedMissionPayloads;
    regional.appendUpdates(progress, regional.takeChanged(), {}, expectedMissionPayloads);
    check(expectedMissionPayloads.size() == 2, "regional transport fixture retains both immediate counts");
    if (expectedMissionPayloads.size() == 2)
        expectedMissionPayloads[0][38] = 0xa1, expectedMissionPayloads[1][39] = 0xb2;
    std::vector<creatures::ItemRecord> expectedGifts;
    for (int16_t quantity : {9, 0, -7}) {
        creatures::ItemRecord platinum{};
        platinum[0] = 13, platinum[1] = 0xff, platinum[0x0c] = 0xff, platinum[0x0d] = 0xff, platinum[0x0e] = 0xff;
        creatures::setItemLevel(platinum, quantity);
        expectedGifts.push_back(platinum);
    }
    auto objectZone = std::make_shared<worldgen::Zone>(4, 6);
    for (int32_t association : {7, 8}) {
        worldgen::StaticObject object;
        object.kind = 5;
        object.state = 1;
        object.timer = 250;
        object.descriptorIndex = association;
        object.stateWord = 0xfedcba98u;
        objectZone->statics.push_back(object);
    }
    worldgen::WorldMap objectMap;
    objectMap.add(objectZone);
    StaticStates objectStates;
    objectStates.generated(*objectZone);
    creatures::NpcFrame objectEvents;
    check(objectStates.fatal(objectMap, {4, 6, 7}, objectEvents) &&
          objectStates.fatal(objectMap, {4, 6, 8}, objectEvents), "transport fixture produces two ordered object snapshots");
    std::vector<std::vector<uint8_t>> expectedStaticPayloads;
    objectStates.appendUpdates(objectMap, objectEvents.staticStates, objectStates.takeChanged(), {}, expectedStaticPayloads);
    // Received padding has no semantic value but must survive the raw queue.
    check(expectedStaticPayloads.size() == 2, "publication does not duplicate the fixture's immediate snapshots");
    if (!expectedStaticPayloads.empty())
        expectedStaticPayloads[0][0x0f] = 0xa7;
    creatures::HitRecord inboundHit;
    inboundHit.attacker = 77;
    inboundHit.target = 1;
    inboundHit.lockMs = 16777217;
    inboundHit.stun = -7;
    inboundHit.position = {INT64_MIN + 7, -1, INT64_MAX - 11};
    inboundHit.critical = 0xff;
    inboundHit.area = 0x81;
    inboundHit.beam = 0xfe;
    inboundHit.criticalPadding = {3, 7, 11};
    inboundHit.trailingPadding = 0xe1;
    auto outboundHit = inboundHit;
    outboundHit.attacker = 1;
    outboundHit.target = 19;
    auto laterHit = outboundHit;
    laterHit.target = 0x10000000aull; // frame queue does not require a resolved target
    laterHit.kind = creatures::HIT_COUNTER;
    auto foreignHit = outboundHit;
    foreignHit.attacker |= uint64_t(1) << 32;
    const std::array expectedHits{outboundHit.serialize(), laterHit.serialize()};
    creatures::Projectile firstShot;
    firstShot.shooter = 1;
    firstShot.position[0] = 123456789;
    firstShot.carried = {0x91, 0x81424344, 0xbeef};
    firstShot.age = 180;
    auto secondShot = firstShot;
    secondShot.position[2] = -98765;
    auto foreignShot = firstShot;
    foreignShot.shooter |= uint64_t(1) << 32;
    const std::array expectedShots{firstShot.serialize(), secondShot.serialize()};
    auto inboundShot = foreignShot.serialize();
    inboundShot[0x65] = 0xc7;
    const std::array inboundShots{inboundShot, firstShot.serialize(), inboundShot};
    std::atomic<bool> finish{false}, received{false}, receivedHit{false}, receivedShots{false};
    std::atomic<bool> frameHeld{false}, worldSent{false}, coherentHit{false}, packetOrder{false};
    std::atomic<bool> repeatClock{false};
    constexpr uint32_t clockDay = 0xfedcba98u, clockTime = 0x80123456u;
    std::atomic<bool> followupRequested{false}, finalDeltaRequested{false};
    std::thread peer([&] {
        Socket socket = listener.accept();
        socket.setReadTimeout(3000);
        uint32_t packet = 0, version = 0;
        if (!socket.readU32(packet) || packet != HELLO || !socket.readU32(version) || version != VERSION) return;
        CreatureRecord initial;
        ByteWriter greeting;
        greeting.u32(ACCEPTED);
        greeting.u32(CREATURE);
        greeting.u64(1);
        greeting.write(initial.bytes.data(), initial.bytes.size());
        greeting.u32(SEED);
        greeting.u32(41);
        if (!socket.send(greeting.data())) return;
        while (!frameHeld && !finish) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (finish) return;
        WorldUpdate update;
        const auto hitBytes = inboundHit.serialize();
        update.sections[1].emplace_back(hitBytes.begin(), hitBytes.end());
        for (const auto &shot : inboundShots)
            update.sections[4].emplace_back(shot.begin(), shot.end());
        for (const auto &event : {first, second}) {
            const auto bytes = event.serialize();
            update.sections[11].emplace_back(bytes.begin(), bytes.end());
        }
        for (const auto &event : relationships) {
            const auto bytes = event.serialize();
            update.sections[10].emplace_back(bytes.begin(), bytes.end());
        }
        update.sections[5] = expectedStaticPayloads;
        update.sections[12] = expectedMissionPayloads;
        for (const auto &item : expectedGifts) {
            ByteWriter grant;
            grant.u64(1);
            grant.write(item.data(), item.size());
            update.sections[8].push_back(grant.data());
        }
        for (const auto &credit : expectedCredits) {
            const auto bytes = credit.serialize();
            update.sections[9].emplace_back(bytes.begin(), bytes.end());
        }
        ByteWriter response;
        response.u32(CLOCK);
        response.u32(clockDay);
        response.u32(clockTime);
        CreatureRecord remote;
        remote.setPosition(0, 123456789);
        remote.setPosition(1, -987654321);
        response.u32(CREATURE);
        response.bytes(compressed(encodeDelta(19, remote, 1)));
        response.u32(CREATURE);
        response.bytes(compressed(encodeDelta(1, remote, 1))); // controlled state stays local
        response.u32(WORLD_UPDATE);
        response.bytes(compressed(update.encode()));
        if (!socket.send(response.data())) return;
        worldSent = true;
        CreatureRecord latestOwn;
        size_t hitCount = 0, buffCount = 0, shotCount = 0;
        bool validHits = true, validShots = true;
        while (!finish && socket.readU32(packet)) {
            if (packet == CREATURE) {
                uint32_t size = 0;
                if (!socket.readU32(size) || size > (16u << 20)) return;
                std::vector<uint8_t> packed(size);
                if (!socket.readExact(packed.data(), packed.size())) return;
                const auto plain = zlibInflate(packed);
                if (!plain || !applyDelta(*plain, latestOwn)) return;
            } else if (packet == EVENT40) {
                std::array<uint8_t, 40> bytes{};
                if (!socket.readExact(bytes.data(), bytes.size())) return;
                received = bytes == expectedSend;
                packetOrder = received && receivedHit && buffCount == 0;
                buffCount++;
            } else if (packet == HIT) {
                std::array<uint8_t, 72> bytes{};
                if (!socket.readExact(bytes.data(), bytes.size())) return;
                validHits &= hitCount < expectedHits.size() && bytes == expectedHits[hitCount] && latestOwn.health() == 137.5f;
                hitCount++;
                receivedHit = hitCount >= expectedHits.size();
                coherentHit = validHits && hitCount == expectedHits.size();
            } else if (packet == SHOOT) {
                std::array<uint8_t, 112> bytes{};
                if (!socket.readExact(bytes.data(), bytes.size())) return;
                validShots &= packetOrder && shotCount < expectedShots.size() && bytes == expectedShots[shotCount];
                if (++shotCount == expectedShots.size()) {
                    receivedShots = validShots;
                    break;
                }
            } else return;
        }
        while (!followupRequested && !finish) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (finish) return;
        const float updatedHealth = 123.25f;
        std::memcpy(&remote.bytes[0x15c], &updatedHealth, sizeof updatedHealth);
        for (size_t i = 0; i < 24; ++i) remote.bytes[0x1b0 + i] = uint8_t(0xa0 + i);
        ByteWriter followup;
        followup.u32(CREATURE);
        followup.bytes(compressed(encodeDelta(19, remote, (uint64_t(1) << 27) | (uint64_t(1) << 40))));
        followup.u32(END_OF_BATCH);
        if (!socket.send(followup.data())) return;
        while (!finalDeltaRequested && !finish) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (finish) return;
        remote.setPosition(2, 654321);
        ByteWriter finalDelta;
        finalDelta.u32(CREATURE);
        finalDelta.bytes(compressed(encodeDelta(19, remote, 1)));
        finalDelta.u32(END_OF_BATCH);
        if (!socket.send(finalDelta.data())) return;
        while (!finish) {
            if (repeatClock.exchange(false)) {
                ByteWriter clock;
                clock.u32(CLOCK);
                clock.u32(clockDay);
                clock.u32(clockTime);
                if (!socket.send(clock.data())) return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    Client client;
    const bool connected = client.connect("127.0.0.1", listener.boundPort());
    check(connected, "buff fixture completes the actual client handshake");
    if (connected) {
        auto frame = client.lockFrame();
        frameHeld = true;
        auto localBuff = creatures::BuffEvent::parse(expectedSend.data());
        auto foreignBuff = localBuff;
        foreignBuff.source |= uint64_t(1) << 32;
        const std::array frameHits{foreignHit, outboundHit, laterHit};
        const std::array frameBuffs{foreignBuff, localBuff};
        const std::array frameShots{firstShot, foreignShot, secondShot};
        client.queueCombatFrame(frameHits, frameBuffs, frameShots);
        check(waitFor([&] { return worldSent.load(); }), "peer supplies input while the frame gate is held");
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        const auto blocked = client.takeCombatReceipts();
        check(blocked.hits.empty() && blocked.buffs.empty() && blocked.shots.empty() && blocked.missions.empty() &&
              client.takeRelationships().empty() &&
              client.takeStaticStates().empty() &&
              client.takeGifts().empty() && client.takeCredits().empty() &&
              client.takeMissionRecords().empty() &&
              client.others().empty() && !client.takeClock(),
              "incoming queue mutation and creature insertion wait until the current frame completes");
        creatures::Combatant actor;
        actor.health = 137.5f;
        CreatureRecord published;
        published.setFighter(actor);
        client.setOwn(published);
        frame.unlock();
        CombatReceipts combat;
        const auto &events = combat.buffs;
        const auto &hits = combat.hits;
        const auto &shots = combat.shots;
        std::vector<creatures::RelationshipRecord> receivedRelationships;
        std::vector<std::vector<uint8_t>> receivedStatics;
        const auto &receivedMissions = combat.missions;
        std::vector<creatures::ItemRecord> receivedGifts;
        std::vector<creatures::KillRecord> receivedCredits;
        check(waitFor([&] {
            if (shots.empty()) {
                combat = client.takeCombatReceipts();
                if (!shots.empty())
                    check(!hits.empty() && !events.empty() && !receivedMissions.empty(),
                          "related combat and objective sections become visible as one frame receipt");
            }
            for (const auto &event : client.takeRelationships()) receivedRelationships.push_back(event);
            for (const auto &event : client.takeStaticStates()) receivedStatics.emplace_back(event.begin(), event.end());
            for (const auto &item : client.takeGifts()) receivedGifts.push_back(item);
            for (const auto &credit : client.takeCredits()) receivedCredits.push_back(credit);
            return events.size() >= 2 && receivedRelationships.size() >= relationships.size() &&
                   receivedStatics.size() >= expectedStaticPayloads.size() && receivedGifts.size() >= expectedGifts.size() &&
                   receivedCredits.size() >= expectedCredits.size() &&
                   receivedMissions.size() >= expectedMissionPayloads.size() &&
                   !hits.empty() && shots.size() >= inboundShots.size() && received && receivedHit && receivedShots;
        }), "actual client exchanges ordered hit, buff and projectile records");
        check(coherentHit && packetOrder && receivedShots,
              "sender queues exact local-source IDs and sends ordered hits, buffs and uncompressed shots after the state");
        const auto drained = client.takeCombatReceipts();
        check(shots.size() == inboundShots.size() && std::equal(shots.begin(), shots.end(), inboundShots.begin()) &&
                  drained.shots.empty(), "section-five shot receipt preserves duplicate payloads and opaque bytes until one drain");
        check(events.size() == 2 && events[0].serialize() == first.serialize() && events[1].serialize() == second.serialize(),
              "incoming buff order, nonlocal target, embedded sources and opaque bytes survive transport");
        check(drained.buffs.empty(), "received buff queue drains exactly once");
        check(receivedRelationships.size() == relationships.size() &&
              receivedRelationships[0].serialize() == relationships[0].serialize() &&
              receivedRelationships[1].serialize() == relationships[1].serialize() &&
              receivedRelationships[2].serialize() == relationships[2].serialize() && client.takeRelationships().empty(),
              "section-eleven relationships retain order, zero/self IDs, float bits and all opaque bytes before one frame drain");
        check(receivedStatics == expectedStaticPayloads && client.takeStaticStates().empty(),
              "ordered section-six snapshots preserve all 88 bytes through frame-protected publication and one drain");
        bool sameMissions = receivedMissions.size() == expectedMissionPayloads.size();
        for (size_t i = 0; sameMissions && i < receivedMissions.size(); i++)
            sameMissions = std::equal(receivedMissions[i].begin(), receivedMissions[i].end(), expectedMissionPayloads[i].begin());
        check(sameMissions && drained.missions.empty() && client.takeMissionRecords().empty(),
              "section-thirteen retains intermediate regional snapshots, opaque received bytes and order through one frame drain");
        check(receivedGifts == expectedGifts && client.takeGifts().empty(),
              "positive, zero and negative Platinum Coins grants retain item bytes without becoming or consuming kill experience");
        bool sameCredits = receivedCredits.size() == expectedCredits.size();
        for (size_t i = 0; sameCredits && i < expectedCredits.size(); i++)
            sameCredits = receivedCredits[i].serialize() == expectedCredits[i].serialize();
        check(sameCredits && client.takeCredits().empty(),
              "complete credit records retain foreign, zero and self IDs, signed amounts, unused type bits and order without aggregation");
        auto wallet = creatures::Inventory::fresh();
        wallet.platinum = 5;
        for (const auto &item : receivedGifts)
            wallet.add(item);
        check(wallet.platinum == 7 && wallet.money == 0,
              "actual loopback category-thirteen grants credit their signed quantities to the inventory balance");
        const auto clock = client.takeClock();
        uint32_t observedDay = 0, observedTime = 0;
        check(clock && clock->first == clockDay && clock->second == clockTime && !client.takeClock() &&
              client.clock(observedDay, observedTime) && observedDay == clockDay && observedTime == clockTime,
              "fresh clock publication retains both raw words, drains once and keeps historical lookup separate");
        check(hits.size() == 1 && hits[0].record().serialize() == inboundHit.serialize() && drained.hits.empty(),
              "packet-7 and compressed-world hits retain lock timer, raw position, flags and opaque bytes");
        const auto peers = client.others();
        const auto remote = peers.find(19);
        check(remote != peers.end() && peers.count(1) == 0,
              "partial remote delta publishes a new actor while controlled state stays separate");
        if (remote != peers.end()) {
            const auto actor = remote->second.fighter();
            const float expectedMultipliers[] = {100.f, 1.f, 1.f, 1.f, 1.f};
            check(actor.controller == 3 && actor.health == 500.f && actor.hitStun == -3000 && actor.level == 1 &&
                      std::equal(actor.multipliers, actor.multipliers + 5, expectedMultipliers),
                  "first position-only delta retains native constructor role, health, stun, level and multipliers");
            check(remote->second.position(0) == 123456789 && remote->second.position(1) == -987654321 &&
                      remote->second.name().empty() && actor.equipment[0].level == 1 && actor.equipment[12].level == 1,
                  "first delta decodes its positions without dropping absent name and item defaults");
            uint32_t fractionBits = 0, negativeBits = 0;
            std::memcpy(&fractionBits, &remote->second.bytes[0x68 + 0x24], 4);
            std::memcpy(&negativeBits, &remote->second.bytes[0x68 + 0x90], 4);
            check(fractionBits == 0x3f8147ae && negativeBits == 0xc1280000 &&
                      remote->second.bytes[0x68 + 2] == 0xff && remote->second.bytes[0x68 + 0x14] == 0xff,
                  "absent appearance keeps exact fixed fractions, negative offsets and selector defaults");
            auto frame = client.lockFrame();
            auto expected = remote->second;
            expected.setHealth(0);
            client.setReceivedHealth(19, 0);
            client.setReceivedHealth(777, 0);
            const auto changed = client.others();
            check(changed.at(19).bytes == expected.bytes && !changed.contains(777),
                  "frame credit cleanup changes only received health and never creates a missing actor");
        }
        followupRequested = true;
        check(waitFor([&] {
            const auto peers = client.others();
            const auto it = peers.find(19);
            return it != peers.end() && it->second.health() == 123.25f;
        }), "existing creature accepts a health-only update with explicit opaque storage");
        {
            auto frame = client.lockFrame();
            client.setReceivedHealth(19, 0);
        }
        finalDeltaRequested = true;
        check(waitFor([&] {
            const auto peers = client.others();
            const auto it = peers.find(19);
            return it != peers.end() && it->second.position(2) == 654321;
        }), "presence reconciliation retains a listed actor across successive batches");
        const auto finalPeers = client.others();
        const auto finalRemote = finalPeers.find(19);
        if (finalRemote != finalPeers.end()) {
            const auto &record = finalRemote->second;
            bool opaquePreserved = true;
            for (size_t i = 0; i < 24; ++i) opaquePreserved &= record.bytes[0x1b0 + i] == uint8_t(0xa0 + i);
            check(record.health() == 0 && record.controller() == 3 &&
                      record.position(0) == 123456789 && opaquePreserved,
                  "position-only delta retains frame credit health and prior opaque fields; a fresh health delta can replace it");
        }
        repeatClock = true;
        std::optional<std::pair<uint32_t, uint32_t>> repeated;
        check(waitFor([&] { repeated = client.takeClock(); return repeated.has_value(); }) && repeated == clock,
              "an identical later clock packet is a fresh publication rather than a cached-value comparison");
    }
    finish = true;
    client.disconnect();
    listener.close();
    peer.join();
}

// One loopback connection for the receiver checks: the peer completes the handshake as
// `local`, runs `script`, then drains the client's output until `observe` has finished.
static void withReceiverPeer(uint64_t local, const std::function<void(Socket &)> &script,
                             const std::function<void(Client &)> &observe) {
    Socket listener;
    if (!listener.listen(0, nullptr, "127.0.0.1")) {
        check(false, "receiver fixture listens");
        return;
    }
    std::atomic<bool> finish{false};
    std::thread peer([&] {
        Socket socket = listener.accept();
        if (!socket.valid())
            return;
        socket.setReadTimeout(3000);
        uint32_t packet = 0, version = 0;
        if (!socket.readU32(packet) || packet != HELLO || !socket.readU32(version))
            return;
        const auto initial = CreatureRecord::nativeDefaults();
        ByteWriter w;
        w.u32(ACCEPTED), w.u32(CREATURE), w.u64(local);
        w.write(initial.bytes.data(), initial.bytes.size());
        w.u32(SEED), w.u32(41);
        if (!socket.send(w.data()))
            return;
        script(socket);
        uint8_t sink = 0;
        while (!finish && socket.readUpTo(&sink, 1) == Socket::ReadEnd::complete) {
        }
        while (!finish)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
    });
    Client client;
    if (client.connect("127.0.0.1", listener.boundPort()))
        observe(client);
    else
        check(false, "receiver fixture connects");
    finish = true;
    client.disconnect();
    listener.close();
    peer.join();
}

// Receiver publication locks, re-certification 2026-11-21.
static void checkReceiverPublicationLocks() {
    constexpr uint64_t local = 5, remoteKey = 21, failedKey = 31;
    const auto takeClockDay = [](Client &client, uint32_t day) {
        return waitFor([&] {
            const auto clock = client.takeClock();
            return clock && clock->first == day;
        });
    };
    // Unread kinds, nonpositive chat lengths, kind-2 timing and presence.
    std::atomic<int> stage{0};
    withReceiverPeer(local, [&](Socket &socket) {
        ByteWriter w;
        for (const uint32_t kind : {6u, 7u, 8u, 9u, 11u, 12u, 13u, 14u, 16u, 17u, 18u, 99u, 0xffffffffu})
            w.u32(kind);
        w.u32(CHAT), w.u64(19), w.i32(0);
        w.u32(CHAT), w.u64(19), w.i32(-5);
        const std::array<uint16_t, 2> text{'h', 'i'};
        w.u32(CHAT), w.u64(19), w.i32(2), w.write(text.data(), sizeof text);
        auto remote = CreatureRecord::nativeDefaults();
        w.u32(CREATURE), w.bytes(compressed(encodeDelta(remoteKey, remote, uint64_t(1) << 27)));
        w.u32(CREATURE), w.bytes(compressed(encodeDelta(local, remote, uint64_t(1) << 27)));
        w.u32(END_OF_BATCH);
        w.u32(CLOCK), w.u32(1), w.u32(100);
        if (!socket.send(w.data()) || !waitFor([&] { return stage.load() >= 1; }))
            return;
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        w.data().clear();
        w.u32(END_OF_BATCH);
        w.u32(CLOCK), w.u32(2), w.u32(200);
        if (socket.send(w.data()))
            waitFor([&] { return stage.load() >= 2; });
    }, [&](Client &client) {
        check(takeClockDay(client, 1), "kinds 6-9, 11-14 and above 15 read no payload and keep the receiver running");
        const auto chat = client.takeChat();
        check(client.connected() && chat.size() == 1 && chat[0].first == 19 && chat[0].second == "hi",
              "zero and negative kind-10 lengths publish nothing and continue without stopping");
        check(client.others().contains(remoteKey) && !client.others().contains(local),
              "a kind-0 registration survives the next kind 2; the controlled update is not published as remote");
        check(!client.batchInterval(), "the first kind 2 after the receiver starts publishes no timing difference");
        stage = 1;
        check(takeClockDay(client, 2), "second kind-2 batch arrives");
        const auto interval = client.batchInterval();
        check(interval && *interval >= 40 && *interval < 3000,
              "a later kind 2 publishes the millisecond difference from the previous kind 2");
        check(client.others().empty(), "kind 2 removes an identifier not registered since the last kind 2");
        stage = 2;
    });

    // Kind-0 decoder failure: the inserted creature stays, the receiver stops.
    withReceiverPeer(local, [&](Socket &socket) {
        ByteWriter plain, w;
        plain.u64(failedKey), plain.u64(uint64_t(1) << 63);
        w.u32(CREATURE), w.bytes(compressed(plain.data()));
        w.u32(CLOCK), w.u32(3), w.u32(300);
        socket.send(w.data());
    }, [&](Client &client) {
        check(waitFor([&] { return !client.connected(); }), "a kind-0 decoder failure sets the stop flag");
        const auto others = client.others();
        check(others.size() == 1 && others.contains(failedKey) &&
                  others.at(failedKey).bytes == CreatureRecord::nativeDefaults().bytes,
              "a creature inserted before the failed decode stays with its constructed state");
        check(client.takeReceipts().empty() && !client.takeClock(),
              "a failed decode skips the second interval and every later message");
    });

    // Zero encoded length stops before any lock.
    withReceiverPeer(local, [&](Socket &socket) {
        ByteWriter w;
        w.u32(CREATURE), w.u32(0);
        w.u32(CLOCK), w.u32(4), w.u32(400);
        socket.send(w.data());
    }, [&](Client &client) {
        check(waitFor([&] { return !client.connected(); }) && client.others().empty() && !client.takeClock(),
              "a zero kind-0 encoded length sets the stop flag without inserting a creature");
    });

    // End of stream mid-value: the message completes with the partial value.
    withReceiverPeer(local, [&](Socket &socket) {
        ByteWriter w;
        w.u32(CLOCK), w.u32(7);
        const std::array<uint8_t, 2> half{0x34, 0x12};
        w.write(half.data(), half.size());
        if (socket.send(w.data()))
            socket.finishSending();
    }, [&](Client &client) {
        std::optional<std::pair<uint32_t, uint32_t>> clock;
        check(waitFor([&] { return (clock = client.takeClock()).has_value(); }) && clock->first == 7 &&
                  clock->second == 0x1234,
              "a zero receive result publishes the partly filled kind-5 value without setting the stop flag");
        check(waitFor([&] { return !client.connected(); }), "engine adapter: a closed stream at the next header ends the receiver");
    });
}

// Packet-8 receipt distinguishes end of stream (partial bytes kept) from an error.
static void checkPartialRead() {
    startNetworking();
    Socket listener;
    if (!listener.listen(0, nullptr, "127.0.0.1")) {
        check(false, "partial-read fixture listens");
        return;
    }
    std::thread peer([&] {
        Socket socket;
        if (!socket.connect("127.0.0.1", listener.boundPort()))
            return;
        const std::array<uint8_t, 10> partial{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        socket.send(partial.data(), partial.size());
        socket.close();
    });
    Socket accepted = listener.accept();
    accepted.setReadTimeout(3000);
    std::array<uint8_t, 40> event{};
    const auto end = accepted.readUpTo(event.data(), event.size());
    peer.join();
    check(end == Socket::ReadEnd::closed && event[0] == 1 && event[9] == 10 && event[10] == 0 && event[39] == 0,
          "end of stream mid-payload is distinct from an error and keeps the bytes received");
}

int main() {
    checkPartialRead();
    {
        auto record = CreatureRecord::nativeDefaults();
        auto actor = record.fighter();
        check(actor.blueprintRotation == 0 && actor.blueprintModel == 0, "fresh blueprint fields are zero");
        actor.blueprintRotation = -7;
        actor.blueprintModel = -128;
        record.setFighter(actor);
        const auto decoded = record.fighter();
        check(record.bytes[0x17c] == 249 && record.bytes[0x17d] == 128 &&
                  decoded.blueprintRotation == -7 && decoded.blueprintModel == -128,
              "the two signed blueprint fields round-trip independently through the core record");
    }
    checkIdentityAdoption();
    checkWorldRequests();
    checkClientPackets();
    checkCreatureReceiptPoses();
    checkCombatTransport();
    checkReceiverPublicationLocks();
    // Server buff-update admission: the day clock factor is 10, or 100 when the scan found
    // a living role-zero creature in action 0x54.
    {
        uint32_t day = 3, ms = 1000;
        advanceServerClock(day, ms, 20, false);
        check(day == 3 && ms == 1200, "server day clock runs at 10x");
        advanceServerClock(day, ms, 20, true);
        check(day == 3 && ms == 3200, "server day clock runs at 100x while accelerated");
        ms = 86400000u - 500u;
        advanceServerClock(day, ms, 5, true);
        check(day == 3 && ms == 86400000u, "net server §2.1: exactly 86,400,000 does not roll over");
        advanceServerClock(day, ms, 1, false);
        check(day == 4 && ms == 10, "above 86,400,000 the day advances");
        // §2.1 item 4: `time` at each 10,000 ms boundary of the accumulated dt.
        int32_t accumulator = 0;
        check(!clockSaveDue(accumulator, 9999) && accumulator == 9999, "no save before 10 s");
        check(clockSaveDue(accumulator, 1) && accumulator == 10000, "the boundary saves");
        check(!clockSaveDue(accumulator, 9999), "the next window waits");
        check(clockSaveDue(accumulator, 25000), "a long pass saves once");
        CreatureRecord record;
        record.bytes[0x58] = 84;
        check(record.action() == 84, "record action byte is creature +0x68");
    }
    // Transport §3.3: the stored stamp is the pre-sleep clock, so a lone sender alternates.
    {
        check(paceSleepMs(1000, 0) == 0, "first pass does not sleep");
        check(paceSleepMs(1005, 1000) == 15, "a pass 5 ms after the stamp sleeps 15 ms");
        check(paceSleepMs(1020, 1005) == 5, "the next pass is measured from the pre-sleep stamp");
        check(paceSleepMs(1025, 1000) == 0, "20 ms or more: no sleep");
        check(paceSleepMs(5, 0xfffffff0u) == 0, "the 32-bit clock wraps");
    }
    // Transport N-8: the main loop's zone index truncates toward zero at both divisions.
    {
        check(zoneForRawPosition(int64_t(300) * 65536, int64_t(511) * 65536) == std::pair(1, 1), "zone of a positive position");
        check(zoneForRawPosition(-1, -65537) == std::pair(0, 0), "negative positions truncate toward zero");
        check(zoneForRawPosition(int64_t(-257) * 65536, 0) == std::pair(-1, 0), "zone -1 only below block -256");
        constexpr int64_t wrap = int64_t{1} << 48;
        check(zoneForRawPosition(wrap + 256 * 65536, wrap - 257 * 65536) == std::pair(1, -1),
              "request zones use the signed low 32 bits of the block quotient");
        constexpr uint32_t start = 0xfffffff0u;
        uint32_t stamp = start;
        check(!requestSnapshotDue(start + 1000u, stamp) && stamp == start,
              "an elapsed 1000 ms across clock wrap does not admit a request turn");
        check(requestSnapshotDue(start + 1001u, stamp) && stamp == start + 1001u,
              "an admitted request turn advances the timestamp whether entries exist or not");
        const auto advanced = stamp;
        check(!requestSnapshotDue(advanced + 0x80000000u, stamp) && stamp == advanced,
              "the elapsed comparison interprets the wrapping difference as signed");
    }
    // Net server §2.2: one zone per pass, nearest first; ties by request order, x, then y.
    {
        using Z = std::pair<int32_t, int32_t>;
        std::set<Z> held;
        const auto loaded = [&](int32_t x, int32_t y) { return held.count({x, y}) > 0; };
        check(nextZoneToGenerate({{10, 10}}, loaded) == Z(10, 10), "the requested zone comes first");
        held.insert({10, 10});
        check(nextZoneToGenerate({{10, 10}}, loaded) == Z(9, 10), "then distance 1, x ascending first");
        check(nextZoneToGenerate({{10, 10}, {9, 10}}, loaded) == Z(9, 10), "a nearer zone of a later request wins");
        for (int dx = -2; dx <= 2; dx++)
            for (int dy = -2; dy <= 2; dy++)
                if (dx * dx + dy * dy < 4)
                    held.insert({10 + dx, 10 + dy});
        check(nextZoneToGenerate({{10, 10}}, loaded) == Z(8, 10), "distance 4 before the corners");
        for (int dx = -2; dx <= 2; dx++)
            for (int dy = -2; dy <= 2; dy++)
                if (dx * dx + dy * dy < 9)
                    held.insert({10 + dx, 10 + dy});
        check(!nextZoneToGenerate({{10, 10}}, loaded), "squared distance 8 is the limit");
        check(nextZoneToGenerate({{0, 0}}, [](int32_t x, int32_t y) { return x == 0 && y == 0; }) == Z(0, 1),
              "zones below 0 are never generated");
        check(!nextZoneToGenerate({}, loaded), "no players, no generation");
    }
    // Net server §4.1: the NPC radius in f32; exactly 200 blocks is kept.
    {
        const std::array<int64_t, 3> origin{int64_t(5000) << 16, 0, 0};
        check(creatureDistanceSquared({(int64_t(5000) + 200) << 16, 0, 0}, origin) == 40000.f, "200 blocks is 40,000");
        check(creatureDistanceSquared({0, int64_t(-120) << 16, int64_t(160) << 16}, {0, 0, 0}) == 40000.f,
              "axes add up");
    }
    // Net server §3.1: the joining record.
    {
        worldgen::Random joinStream{1}, reference{1};
        const CreatureRecord join = newPlayerRecord([&] { return joinStream.draw(); });
        for (int k = 0; k < 5; k++)
            reference.draw();
        check(joinStream.count() == 5 && joinStream.draw() == reference.draw(),
              "the join's appearance generator takes exactly five draws from the given stream");
        check(join.position(0) == 550301073408LL && join.position(1) == 550301073408LL && join.position(2) == 0,
              "join spawn (8,396,928, 8,396,928, 0) blocks");
        int64_t home[3];
        std::memcpy(home, &join.bytes[0x1b0], 24);
        check(home[0] == join.position(0) && home[1] == join.position(1) && home[2] == 0, "home copies the position");
        check(join.controller() == 0 && join.fighter().type == 44 && join.fighter().level == 1 &&
                  join.fighter().characterClass == 1,
              "player, type 44, level 1, class 1");
        bool items = true;
        for (const size_t at : {size_t(0x980), size_t(0xa98)})
            items &= join.bytes[at] == 3 && join.bytes[at + 1] == 2 && join.bytes[at + 0x0d] == 1;
        check(items, "two type-3 subtype-2 item records at 0x980 and 0xA98");
        bool zeroed = join.bytes[0x4c] == 0 && join.bytes[0x4d] == 0 && join.bytes[0x4e] == 0 && join.bytes[0x4f] == 0;
        for (size_t at = 0x18; at < 0x30; at++)
            zeroed &= join.bytes[at] == 0;
        check(zeroed, "record 0x18..0x2F and 0x4C are zero");

        // Creature-state defaults contract, join overrides over the construction defaults.
        const auto u32At = [&](size_t at) {
            uint32_t v;
            std::memcpy(&v, &join.bytes[at], 4);
            return v;
        };
        // Ruling 2 step 12: the generator replaces the 1.5 products and sets the type-44 models.
        check(u32At(0x70) == 0x3f75c290 && u32At(0x74) == 0x3f75c290 && u32At(0x78) == 0x400a3d71,
              "join box is the generator's 0.96, 0.96, 2.16 bits");
        const auto u16At = [&](size_t at) {
            uint16_t v;
            std::memcpy(&v, &join.bytes[0x68 + at], 2);
            return v;
        };
        check(u16At(0x14) == 0x000a && u16At(0x16) == 0xffff && u16At(0x18) == 0x01ae && u16At(0x1a) == 0x01b0 &&
                  u16At(0x1c) == 0x0000 && u16At(0x1e) == 0xffff && u16At(0x20) == 0xffff && u16At(0x22) == 0xffff,
              "type-44 appearance models; the later selectors keep their defaults");
        check(std::bit_cast<uint32_t>(std::bit_cast<float>(0x3fe66666u) * 1.5f) == 0x402ccccc,
              "the dead 1.5 height product rounds to even");
        bool blockLevels = true;
        for (size_t slot = 0; slot < 13; slot++) {
            uint16_t level;
            std::memcpy(&level, &join.bytes[0x2f0 + 0x118 * slot + 0x10], 2);
            blockLevels &= level == 0;
        }
        check(blockLevels && join.fighter().held.level == 1,
              "the join reset zeroes every block item's level; the standalone item keeps level 1");
        check(join.health() == 100.f && u32At(0x54) == 0x2c, "join health and type 0x2C");
        // Everything outside the join's assignments keeps the construction defaults.
        const CreatureRecord base = CreatureRecord::nativeDefaults();
        const auto joinWritten = [](size_t at) {
            return at < 0x18 || (at >= 0x50 && at < 0x58) || (at >= 0x70 && at < 0x86) ||
                   (at >= 0x130 && at < 0x131) || (at >= 0x15c && at < 0x160) || (at >= 0x1b0 && at < 0x1c8) ||
                   (at >= 0x2f0 && at < 0x1128);
        };
        bool kept = true;
        for (size_t at = 0; at < CreatureRecord::SIZE; at++)
            kept &= joinWritten(at) || join.bytes[at] == base.bytes[at];
        check(kept, "join keeps construction defaults (-3000, multipliers, -1 words, appearance) elsewhere");
        bool block = true;
        for (size_t at = 0x2f0; at < 0x1128; at++) {
            const size_t item = (at - 0x2f0) % 0x118, slot = (at - 0x2f0) / 0x118;
            const bool selected = (slot == 6 || slot == 7) && (item == 0 || item == 1 || item == 0x0d);
            block &= selected || join.bytes[at] == 0;
        }
        check(block, "only the two selected block items carry nonzero bytes");
        // The health query sees the construction controller 3 and class 0 with a zeroed block.
        CreatureRecord queried = base;
        std::memset(&queried.bytes[0x2f0], 0, 0x1128 - 0x2f0);
        const auto queryFighter = queried.fighter();
        check(queryFighter.controller == 3 && queryFighter.characterClass == 0 &&
                  creatures::maxHealth(queryFighter) == join.health(),
              "the join health is the engine query under the controller-3 state");
        check(std::bit_cast<uint32_t>(join.health()) == 0x42c80000u,
              "controller 3, level 1, rank 0, class 0 and multiplier 100 give exactly 100");
    }
    // Creature-state defaults contract: construction defaults and codec groups.
    {
        const CreatureRecord d = CreatureRecord::nativeDefaults();
        const auto u32At = [&](size_t at) {
            uint32_t v;
            std::memcpy(&v, &d.bytes[at], 4);
            return v;
        };
        check(d.bytes[0x50] == 3 && d.bytes[0x51] == 0 && d.bytes[0x52] == 0 && d.bytes[0x53] == 0,
              "one-byte controller default 3");
        check(u32At(0x15c) == 0x43fa0000 && u32At(0x11c) == 0xfffff448 && u32At(0x168) == 0x42c80000 &&
                  u32At(0x180) == 1 && u32At(0x19c) == 0xffffffff,
              "health 500, -3000, multiplier 100, level 1, -1");
        check(u32At(0x1a0) == 0xffffffff && u32At(0x1a4) == 0xffffffff && u32At(0x1a8) == 0 &&
                  u32At(0x1cc) == 0xffffffff && u32At(0x1d0) == 0xffffffff && u32At(0x1d4) == 0,
              "two three-word groups -1, -1, 0");
        check(u32At(0x68) == 0xffff0000 && d.bytes[0x6c] == 0xff && u32At(0x68 + 0x24) == 0x3f8147ae,
              "appearance leading bytes 00 00 FF FF FF");
        bool items = true;
        for (size_t base : {size_t(0x1d8), size_t(0x2f0)})
            for (size_t slot = 0; slot < (base == 0x1d8 ? 1u : 13u); slot++) {
                uint16_t level;
                std::memcpy(&level, &d.bytes[base + 0x118 * slot + 0x10], 2);
                items &= level == 1;
            }
        check(items, "all fourteen constructed items have level 1");
        // Shared writer/reader order: ascending, but 0x1CC before 0x1C8 and trailing name,
        // eleven words, 0x1154; the 0x1B0 group is coded like the position.
        check(CREATURE_FIELDS[0].offset == 0x000 && CREATURE_FIELDS[40].offset == 0x1b0 &&
                  CREATURE_FIELDS[40].size == CREATURE_FIELDS[0].size &&
                  CREATURE_FIELDS[40].kind == CREATURE_FIELDS[0].kind && CREATURE_FIELDS[41].offset == 0x1cc &&
                  CREATURE_FIELDS[42].offset == 0x1c8 && CREATURE_FIELDS[44].size == 0xe38 &&
                  CREATURE_FIELDS[45].offset == 0x1158 && CREATURE_FIELDS[46].offset == 0x1128 &&
                  CREATURE_FIELDS[47].offset == 0x1154,
              "codec group order");
        // One mask bit per group; an absent group keeps the seeded value.
        CreatureRecord sent = d;
        sent.bytes[0x1c8] = 7;
        const int32_t talkX = 9;
        std::memcpy(&sent.bytes[0x1cc], &talkX, 4);
        CreatureRecord seeded = d;
        const auto id = applyDelta(encodeDelta(5, sent, uint64_t(1) << 42), seeded);
        check(id && *id == 5 && seeded.bytes[0x1c8] == 7 && std::memcmp(&seeded.bytes[0x1cc], &d.bytes[0x1cc], 12) == 0,
              "bit 42 carries the 0x1C8 byte only");
    }
    // Transport §4: a failed inflate keeps the output produced before the failure.
    {
        std::vector<uint8_t> plain(5000);
        for (size_t i = 0; i < plain.size(); i++)
            plain[i] = uint8_t(i * 7);
        std::vector<uint8_t> packed = zlibCompress(plain);
        check(zlibInflatePartial(packed) == plain, "a whole stream inflates fully");
        packed.resize(packed.size() / 2);
        const auto partial = zlibInflatePartial(packed);
        check(partial.size() < plain.size() && std::equal(partial.begin(), partial.end(), plain.begin()),
              "a cut stream keeps its leading output");
        packed[0] = 0;
        check(zlibInflatePartial(packed).empty(), "a bad header yields no output and no error");
    }
    // §6.2: position and health of entity 1. (The plain bytes printed in the spec are shifted
    // by one byte in the position; these are what its wire bytes inflate to.)
    const auto plain62 = hex("01 00 00 00 00 00 00 00 01 00 00 08 00 00 00 00 00 80 42 26 80 00 00 00 "
                             "00 80 a0 22 80 00 00 00 00 00 40 00 00 00 00 00 00 80 08 43");
    CreatureRecord r;
    const auto id = applyDelta(plain62, r);
    check(id && *id == 1, "6.2 decodes, id 1");
    check(r.position(0) == 550397706240LL && r.position(1) == 550336757760LL && r.position(2) == 4194304,
          "6.2 position");
    check(r.health() == 136.5f, "6.2 health");
    {
        // The fighting state round-trips through the record (behaviour spec §1).
        creatures::Combatant c;
        c.action = 17;
        c.actionMs = 250;
        c.characterClass = 3;
        c.level = 12;
        c.health = 321.5f;
        c.equipment[6] = creatures::Item{3, 10, 2, 1, 12, 77};
        CreatureRecord rec;
        rec.setFighter(c);
        const creatures::Combatant back = rec.fighter();
        check(back.action == 17 && back.actionMs == 250 && back.characterClass == 3 && back.level == 12 &&
                  back.health == 321.5f,
              "fighter fields round-trip");
        check(back.equipment[6].type == 3 && back.equipment[6].subtype == 10 && back.equipment[6].material == 2 &&
                  back.equipment[6].rarity == 1 && back.equipment[6].level == 12 && back.equipment[6].seed == 77,
              "equipment round-trips");
    }
    check(encodeDelta(1, r, changedFields(r, &(const CreatureRecord &)CreatureRecord{})) == plain62,
          "6.2 encodes the same plain bytes from a zeroed record");
    const auto wire62 = hex("78 9c 63 64 80 00 46 06 06 0e 30 a3 c1 49 ad 01 4c 2f 50 02 d3 0c 0e 10 05 0d 1c ce 00 48 c3 04 40");
    check(zlibInflate(wire62) == plain62, "6.2 wire bytes inflate to the plain payload");
    check(zlibCompress(plain62) == wire62, "6.2 compresses to the spec's bytes");

    // §6.3: an empty delta.
    CreatureRecord same;
    check(encodeDelta(1, same, changedFields(same, &same)) == hex("01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"),
          "6.3 empty delta");
    check(compressed(hex("01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")) ==
              hex("0b 00 00 00 78 9c 63 64 40 05 00 00 20 00 02"),
          "6.3 on the wire");
    // Floats compare as floats: -0 equals +0.
    CreatureRecord negativeZero;
    const float minus = -0.f;
    std::memcpy(&negativeZero.bytes[0x15c], &minus, 4);
    check(changedFields(negativeZero, &same) == 0, "-0 health is not a change");
    {
        // Packets §2.2: the items and the appearance use their typed tests, so bytes those tests
        // skip never set a mask bit.
        CreatureRecord base;
        const int32_t four = 4;
        std::memcpy(&base.bytes[0x2f0 + 0x114], &four, 4); // equipment slot 0 uses 4 sub-entries
        CreatureRecord other = base;
        other.bytes[0x1d8 + 0x02] = other.bytes[0x1d8 + 0x0f] = other.bytes[0x1d8 + 0x12] = 9;
        other.bytes[0x2f0 + 0x118 * 5 + 0x03] = 9;
        other.bytes[0x2f0 + 0x14 + 8 * 4] = 9; // the fifth sub-entry, beyond the count
        other.bytes[0x68 + 0x05] = 9;
        std::memcpy(&other.bytes[0x68 + 0x24], &minus, 4);
        check(changedFields(other, &base) == 0, "uncompared item and appearance bytes never set a bit");
        other.bytes[0x2f0 + 0x14 + 8 * 3 + 7] = 9; // the fourth sub-entry's last byte
        check(changedFields(other, &base) == uint64_t(1) << 44, "a used sub-entry is compared whole");
        other = base;
        other.bytes[0x1d8 + 0x0e] = 1;
        check(changedFields(other, &base) == uint64_t(1) << 43, "item byte 0x0e is compared");
        other = base;
        other.bytes[0x68 + 0x06] = 1;
        check(changedFields(other, &base) == uint64_t(1) << 13, "appearance word 0x06 is compared");
    }
    {
        CreatureRecord motion, response;
        const glm::vec3 velocity{1, 2, 3}, intent{4, 5, 6}, impulse{-7, 8, -9};
        motion.setMotion(velocity, 45, {}, intent);
        response = motion;
        response.setMotion(velocity, 45, impulse, intent);
        const uint64_t mask = changedFields(response, &motion);
        const auto delta = encodeDelta(72, response, mask);
        check(mask == (uint64_t(1) << 4) && delta.size() == 28,
              "response alone enables presence bit four and carries all twelve component bytes");
        const auto identity = applyDelta(delta, motion);
        check(identity && *identity == 72 && motion.velocity() == velocity && motion.intent() == intent &&
              motion.response() == impulse && motion.yaw() == 45,
              "response delta replaces its own vector and preserves velocity, intent and rotation");
        motion.setMotion({}, 0, {-0.f, 0.f, -0.f});
        check(changedFields(motion, &same) == 0, "response compares signed zeros numerically without a bytewise change");
    }

    // §6.5: chat "hi".
    check(toUtf16("hi") == std::vector<uint16_t>{'h', 'i'}, "chat text");
    check(fromUtf16(toUtf16("héllo ✓")) == "héllo ✓", "chat round trip");

    // §6.6, §6.7: world updates.
    WorldUpdate empty;
    check(empty.encode() == std::vector<uint8_t>(52, 0), "6.6 empty world update is 52 zero bytes");
    check(compressed(empty.encode()) == hex("0c 00 00 00 78 9c 63 60 20 1d 00 00 00 34 00 01"), "6.6 on the wire");
    WorldUpdate edit;
    edit.sections[0].push_back(hex("42 26 80 00 a0 22 80 00 40 00 00 00 80 40 20 01 00 00 00 00"));
    // (zlib versions may choose different, equally valid streams: compare what they inflate to.)
    check(zlibInflate(hex("78 9c 63 64 60 60 70 52 6b 60 58 a0 d4 c0 e0 c0 c0 c0 d0 e0 a0 c0 c8 40 06 00 00 cc 1d 03 4d")) ==
              edit.encode(),
          "6.7 the spec's wire bytes are this update");
    const auto ours = compressed(edit.encode());
    check(zlibInflate(std::vector<uint8_t>(ours.begin() + 4, ours.end())) == edit.encode(), "6.7 our stream inflates back");
    const auto decoded = WorldUpdate::decode(edit.encode());
    check(decoded && decoded->sections[0].size() == 1, "6.7 decodes");
    WorldUpdate keyed;
    keyed.keyed[0].push_back({0x0000802000008026ull, {std::vector<uint8_t>(328, 7)}});
    const auto keyedBack = WorldUpdate::decode(keyed.encode());
    check(keyedBack && keyedBack->keyed[0].size() == 1 && keyedBack->keyed[0][0].records[0][5] == 7, "keyed section");

    // A real server and two clients on the loopback interface.
    ServerOptions options;
    options.extendedQuests = true; // also exercise the built-in quest-book feature
    options.seed = 2412123;
    options.port = 0; // any free port
    options.folder = std::filesystem::temp_directory_path() / "AlphaVulkan-NetTests";
    // Block edits §6.2: blueprint 0 with a protected interior layer.
    options.prefabs = [](const std::string &key) -> std::shared_ptr<const worldgen::Prefab> {
        if (key != "framework-floor-wood.cub")
            return nullptr;
        auto slab = std::make_shared<worldgen::Prefab>();
        slab->sx = slab->sy = 10, slab->sz = 6;
        slab->rgb.assign(10 * 10 * 6 * 3, 165);
        std::fill_n(slab->rgb.begin() + 2 * 10 * 10 * 3, 10 * 10 * 3, uint8_t(0));
        return slab;
    };
    std::filesystem::remove_all(options.folder);
    std::filesystem::create_directories(options.folder);
    {
        std::filesystem::create_directories(options.folder / "Save");
        SaveDb db;
        MissionArea mission;
        mission.kind = 1;
        mission.seed = 0x31415926;
        mission.level = 5;
        mission.targetType = 19;
        mission.targetZoneX = mission.targetZoneY = 32800;
        MonsterArea monster{19, 1, 0, 32800, 32800};
        check(db.open(options.folder / "Save" / "world_server_2412123.db", SaveDb::Mode::ReadWrite) &&
              db.put("mission4100_4100", mission.save()) && db.put("monster4100_4100", monster.save()),
              "real server fixture stores a synthetic regional target without owner saves");
    }
    options.bindAddress = "127.0.0.1";
    // Mod SDK v10 on the server: events arrive without the lock, so a hook may call back in.
    Server *modServer = nullptr;
    std::mutex modMutex;
    std::vector<std::string> modSeen;
    std::atomic<int> modFrames{0};
    options.modEvents.frame = [&](double) { modFrames++; };
    options.modEvents.playerJoin = [&](uint64_t id) {
        std::lock_guard guard(modMutex);
        modSeen.push_back("join " + std::to_string(id));
    };
    options.modEvents.chat = [&](uint64_t sender, const std::string &text) {
        const std::string name = modServer->playerName(sender).value_or("?");
        if (text == "hi")
            modServer->tellPlayer(sender, "mod heard " + name);
        std::lock_guard guard(modMutex);
        modSeen.push_back("chat " + name + " " + text + " creatures " + std::to_string(modServer->creatureList().size() > 0));
    };
    Server server(options);
    modServer = &server;
    std::string error;
    check(server.start(&error), "server starts: " + error);
    const uint16_t port = server.port();

    Client a, b;
    check(a.connect("127.0.0.1", port, &error), "client a joins: " + error);
    check(b.connect("127.0.0.1", port, &error), "client b joins: " + error);
    check(a.id() == 1 && b.id() == 2, "ids 1 and 2");
    check(a.seed() == 2412123 && b.seed() == 2412123, "seed sent");
    CreatureRecord ra, rb;
    creatures::Combatant alive;
    alive.health = 100;
    ra.setFighter(alive);
    rb.setFighter(alive);
    ra.setName("Alice");
    ra.setPosition(0, (int64_t)8396928 << 16);
    rb.setName("Bob");
    rb.setPosition(0, (int64_t)8396928 << 16);
    a.setOwn(ra);
    b.setOwn(rb);
    check(waitFor([&] { auto o = a.others(); return o.count(2) && o[2].name() == "Bob"; }), "a sees Bob");
    check(waitFor([&] { auto o = b.others(); return o.count(1) && o[1].name() == "Alice" && o[1].position(0) == ((int64_t)8396928 << 16); }),
          "b sees Alice where she is");
    {
        auto state = ra.fighter();
        state.mode = 0xa620;
        ra.setFighter(state);
        a.setOwn(ra);
        check(waitFor([&] {
            const auto peers = b.others();
            return peers.contains(a.id()) && peers.at(a.id()).fighter().mode == 0xa620;
        }), "actual server publishes incoming mode 0x20 with all other word bits intact");
        ra.setName("Alice partial");
        a.setOwn(ra);
        check(waitFor([&] {
            const auto peers = b.others();
            return peers.contains(a.id()) && peers.at(a.id()).name() == "Alice partial" &&
                   peers.at(a.id()).fighter().mode == 0xa620;
        }), "a subsequent name-only delta keeps the server's snapshotted mode word");
        state.mode = 0;
        ra.setFighter(state);
        ra.setName("Alice");
        a.setOwn(ra);
        check(waitFor([&] {
            const auto peers = b.others();
            return peers.contains(a.id()) && peers.at(a.id()).name() == "Alice" && peers.at(a.id()).fighter().mode == 0;
        }), "a present zero word clears all mode bits through the actual server");
    }
    {
        const Hit self = Hit::make(a.id(), a.id(), -.1f, 0, {}, 0);
        a.sendHit(self);
        a.sendHit(self);
        std::vector<Hit> echoed;
        check(waitFor([&] {
            for (const auto &record : a.takeHits()) echoed.push_back(record);
            return echoed.size() >= 2;
        }), "server relays repeated self-source hits back to their sending player");
        check(echoed.size() == 2 && echoed[0].bytes == self.bytes && echoed[1].bytes == self.bytes,
              "self relay keeps both ordered records without identity rewriting or duplicate suppression");
        std::vector<Hit> observed;
        check(waitFor([&] {
            for (const auto &record : b.takeHits()) observed.push_back(record);
            return observed.size() >= 2;
        }) && observed.size() == 2 && observed[0].bytes == self.bytes && observed[1].bytes == self.bytes,
              "a nontarget recipient retains both relayed records for later foreign-source presentation");
        const Hit foreign = Hit::make(0xfedcba9876543210, a.id(), 3, 0, {}, 0);
        b.sendHit(foreign);
        echoed.clear();
        check(waitFor([&] {
            for (const auto &record : a.takeHits()) echoed.push_back(record);
            return !echoed.empty();
        }) && echoed.size() == 1 && echoed[0].bytes == foreign.bytes,
              "role-zero relay precedes source lookup and does not replace the embedded source with the sending player");
    }
    {
        a.takeHits();
        b.takeHits();
        const creatures::BuffEvent periodic{a.id(), a.id(), {4, 2, 650, 0x123456789abcdef0ULL}};
        a.sendEvent40(periodic.serialize());
        std::vector<Hit> generated, observed;
        const auto collect = [&](Client &peer, std::vector<Hit> &records) {
            for (const auto &hit : peer.takeHits())
                if (hit.attacker() == 0 && hit.target() == a.id() && hit.record().amount == 2)
                    records.push_back(hit);
        };
        check(waitFor([&] {
            collect(a, generated);
            collect(b, observed);
            return !generated.empty() && !observed.empty();
        }, 10000), "actual server countdown turns packet-8 resident damage into periodic hit output for both clients");
        check(generated.size() == 1 && observed.size() == 1 && generated[0].bytes == observed[0].bytes &&
              generated[0].record().kind == creatures::HIT_DAMAGE && generated[0].record().stun == 0 &&
              generated[0].record().knockBack == glm::vec3(0),
              "server-generated null-source hit retains category/vector/timer and reaches its target once");
    }
    ra.setPosition(0, ra.position(0) + (5 << 16));
    a.setOwn(ra);
    check(waitFor([&] { auto o = b.others(); return o.count(1) && o[1].position(0) == ra.position(0); }), "b sees Alice move");
    a.say("hi");
    std::vector<std::pair<uint64_t, std::string>> heard;
    check(waitFor([&] { for (auto &l : b.takeChat()) heard.push_back(l); return !heard.empty(); }), "b hears chat");
    check(!heard.empty() && heard[0].first == 1 && heard[0].second == "hi", "chat from Alice says hi");
    {
        std::vector<std::pair<uint64_t, std::string>> told;
        const auto fromServer = [&] {
            return std::find_if(told.begin(), told.end(), [](const auto &l) { return l.first == 0; });
        };
        check(waitFor([&] { for (auto &l : a.takeChat()) told.push_back(l); return fromServer() != told.end(); }),
              "mod SDK v10: the server mod's tell reaches Alice");
        check(fromServer() != told.end() && fromServer()->second == "mod heard Alice", "told from 0 with her name");
        std::lock_guard guard(modMutex);
        check(std::count(modSeen.begin(), modSeen.end(), "join 1") == 1 && std::count(modSeen.begin(), modSeen.end(), "join 2") == 1,
              "server mod events: both joins, once each");
        check(std::count(modSeen.begin(), modSeen.end(), "chat Alice hi creatures 1") == 1, "server mod chat event, list read back");
        check(modFrames > 0, "server mod frames run per world step");
    }
    // Quests (docs/design/quests.md): an AlphaVulkan client's hello is not passed on as chat, and
    // the server answers it with the player's quest book.
    a.say("\x01" "AV1");
    check(waitFor([&] { return a.takeQuestBook().has_value(); }), "the quest book arrives");
    a.say("after");
    heard.clear();
    check(waitFor([&] { for (auto &l : b.takeChat()) heard.push_back(l); return !heard.empty(); }), "b hears the next line");
    check(heard.size() == 1 && heard[0].second == "after", "and not the hello");
    uint32_t day, ms;
    check(waitFor([&] { return a.clock(day, ms); }), "clock arrives");
    // Server simulation §2.1, §5: the server makes the zones round Alice and the creatures of
    // their spawns come alive near her, listed with the spawn ids of §5.3.
    ra.setPosition(1, (int64_t)8396928 << 16);
    ra.setPosition(2, (int64_t)80 << 16);
    a.setOwn(ra);
    const int32_t aliceX = int32_t(ra.position(0) / 65536), aliceY = int32_t(ra.position(1) / 65536);
    check(waitFor([&] { return server.block(aliceX, aliceY, 80).has_value(); }, 120000),
          "the server holds Alice's destination zone before its drop fixture");
    uint64_t target = 0;
    check(waitFor([&] {
        for (const auto &[id, record] : a.others()) {
            if (id >= (uint64_t(32700) << 24) && record.controller() != 0 && record.health() > 0) {
                target = id;
                return true;
            }
        }
        return false;
    }, 120000), "a living zone spawn reaches Alice with its spawn id");
    check(waitFor([&] {
        for (const auto &[id, record] : a.others())
            if (record.controller() != 0 && record.contactFlags() != 0)
                return true;
        return false;
    }), "an NPC's simulated terrain contact reaches the peer");
    {
        if (target) {
            a.takeRelationships();
            b.takeRelationships();
            const Hit contact = Hit::make(a.id(), target, 0, 0, {}, 0);
            a.sendHit(contact);
            std::optional<creatures::RelationshipRecord> first, second;
            const auto collect = [&](Client &client, auto &result) {
                for (const auto &record : client.takeRelationships())
                    if (record.target == target && record.source == a.id())
                        result = record;
            };
            check(waitFor([&] {
                collect(a, first);
                collect(b, second);
                return first.has_value() && second.has_value();
            }), "real world contact publishes section-eleven relationship records to both peers");
            check(first && second && first->value > 0 && first->serialize() == second->serialize(),
                  "real server preserves the same complete relationship payload for target and other peers");
        }
    }
    // Server simulation §8, world items §3: Alice drops a sword, sees it in her zone's list
    // (section 7), picks it up and gets it back as a gift (section 9).
    std::pair<int32_t, int32_t> aliceZone;
    {
        const int32_t zx = (int32_t)(ra.position(0) >> 24), zy = (int32_t)(ra.position(1) >> 24);
        aliceZone = {zx, zy};
        std::array<uint8_t, 300> drop{};
        drop[0] = 3, drop[0x0d] = 1;
        drop[0x128] = 6;
        a.sendItemRequest(drop);
        a.requestZones(std::array{aliceZone});
        int32_t index = -1;
        check(waitFor([&] {
                  for (const auto &[key, records] : a.takeZoneItems())
                      if (key == std::make_pair(zx, zy))
                          for (size_t k = 0; k < records.size(); k++)
                              if (records[k][0] == 3 && records[k][0x0d] == 1)
                                  index = (int32_t)k;
                  return index >= 0;
              }),
              "the dropped sword lies in Alice's zone");
        a.requestZones({});
        std::array<uint8_t, 300> pick{};
        const int32_t target[3] = {zx, zy, index};
        std::memcpy(&pick[0x118], target, 12);
        pick[0x128] = 5;
        a.sendItemRequest(pick);
        std::vector<std::array<uint8_t, 280>> gifts;
        check(waitFor([&] {
                  for (auto &g : a.takeGifts())
                      gifts.push_back(g);
                  return !gifts.empty();
              }),
              "picking it up gives it back");
        check(!gifts.empty() && gifts[0][0] == 3 && gifts[0][0x0d] == 1, "the gift is the sword");
        // Mixed requests leave the sender FIFO intact across the connection.
        {
            creatures::ItemRecord potion{};
            potion[0] = 1, potion[1] = 1;
            creatures::setItemLevel(potion, 5);
            std::array<uint8_t, 300> drink{};
            std::memcpy(drink.data(), potion.data(), potion.size());
            drink[0x128] = 1;
            a.takeHits();
            a.takeSounds();
            a.sendItemRequest(drink);
            auto rejected = pick;
            const int32_t invalidIndex = -1;
            std::memcpy(&rejected[0x120], &invalidIndex, sizeof invalidIndex);
            a.sendItemRequest(rejected);
            auto secondDrop = drop;
            secondDrop[0x0d] = 121;
            a.sendItemRequest(secondDrop);
            creatures::setItemLevel(potion, 9);
            std::memcpy(drink.data(), potion.data(), potion.size());
            a.sendItemRequest(drink);
            std::vector<Hit> heals;
            std::vector<int32_t> sounds;
            const glm::vec3 senderPosition((float)ra.position(0) / 65536.f, (float)ra.position(1) / 65536.f,
                                           (float)ra.position(2) / 65536.f);
            check(waitFor([&] {
                      for (const Hit &h : a.takeHits())
                          if (h.target() == a.id() && h.amount() < 0)
                              heals.push_back(h);
                      for (const auto &sound : a.takeSounds())
                          if (sound.position == senderPosition && (sound.id == 44 || sound.id == 45 || sound.id == 57))
                              sounds.push_back(sound.id);
                      return heals.size() >= 2 && sounds.size() >= 3;
                  }),
                  "mixed requests deliver the heal hits and ordered sounds");
            check(heals.size() == 2 && heals[0].attacker() == 0 && heals[1].attacker() == 0 &&
                  heals[0].kind() == 0 && heals[1].kind() == 0 && heals[0].amount() > heals[1].amount(),
                  "the two potions emit ordered negative heals with attacker id zero");
            check(sounds == std::vector<int32_t>{44, 57, 44}, "rejected pickup makes no sound between two heals and a drop");
        }
        // Block edits §6: Alice places a slab. Bob sees the broadcast and requests its zone once.
        CreatureRecord placing = ra;
        const glm::vec3 aim(0, 6, 0);
        std::memcpy(&placing.bytes[0x150], &aim, 12);
        placing.bytes[0x17c] = 1, placing.bytes[0x17d] = 0;
        const int64_t position[3] = {placing.position(0), placing.position(1), placing.position(2)};
        const auto base = placementBase(position, aim);
        check(waitFor([&] { return server.block(base[0], base[1], base[2]).has_value(); }, 15000),
              "the placement zone is resident");
        a.setOwn(placing);
        b.takeBlockEdits();
        std::array<uint8_t, 300> build{};
        build[0x128] = 7;
        a.sendItemRequest(build);
        constexpr size_t solidCount = 10 * 10 * 5;
        const auto collectSlab = [&](std::vector<std::array<uint8_t, 20>> &records) {
            for (const auto &record : b.takeBlockEdits()) {
                const auto e = BlockEdit::parse(record.data());
                if (e.x >= base[0] - 5 && e.x < base[0] + 5 && e.y >= base[1] - 5 && e.y < base[1] + 5 &&
                    e.z >= base[2] && e.z < base[2] + 6)
                    records.push_back(record);
            }
            return records.size() >= solidCount;
        };
        std::vector<std::array<uint8_t, 20>> placed, replayed;
        check(waitFor([&] { return collectSlab(placed); }, 15000),
              "the placed slab arrives in section 1");
        const auto air = server.block(base[0] + 4, base[1] - 5, base[2] + 2);
        check(air && air->typeByte == worldgen::DRY_AIR, "blueprint writes protected interior air to the live world");
        bool orderedSolids = placed.size() == solidCount;
        if (orderedSolids) {
            size_t index = 0;
            for (int i = 0; i < 10; i++)
                for (int j = 0; j < 10; j++)
                    for (const int k : {0, 1, 3, 4, 5}) {
                        const auto e = BlockEdit::parse(placed[index++].data());
                        orderedSolids &= e.x == base[0] + 4 - j && e.y == base[1] - 5 + i && e.z == base[2] + k &&
                                         e.r == 165 && e.g == 165 && e.b == 165 && e.typeByte == 0x46;
                    }
        }
        check(orderedSolids, "section 1 contains the 500 solid voxels in model traversal order");
        const Client::ZonePlacements expectedSnapshot{{zx, zy}, {{0, base[0], base[1], base[2]}}};
        const auto hasPlacement = [&](Client &peer) {
            const auto snapshots = peer.takeZonePlacements();
            return std::find(snapshots.begin(), snapshots.end(), expectedSnapshot) != snapshots.end();
        };
        bool placerSaw = false, distantPeerSaw = false;
        check(waitFor([&] {
                  placerSaw |= hasPlacement(a);
                  distantPeerSaw |= hasPlacement(b);
                  return placerSaw && distantPeerSaw;
              }), "the dedicated placement broadcasts the section-8 list to both peers, including the distant peer");
        const std::pair<int32_t, int32_t> neighbour{zx + 1, zy}, outsideWorld{65536, zy};
        // Use the spectator's ordinary interest point to load and hold the
        // separate zone before testing request order.
        const int32_t neighbourX = neighbour.first * 256 + 128, neighbourY = zy * 256 + 128;
        CreatureRecord nearby = rb;
        nearby.setPosition(0, int64_t(neighbourX) * 65536);
        nearby.setPosition(1, int64_t(neighbourY) * 65536);
        b.setOwn(nearby);
        check(waitFor([&] { return server.block(neighbourX, neighbourY, 0).has_value(); }, 15000),
              "the adjacent request zone is resident");
        b.takeZoneItems();
        b.requestZones(std::array{neighbour, outsideWorld, aliceZone, neighbour});
        std::vector<std::pair<int32_t, int32_t>> zoneReplies;
        check(waitFor([&] {
                  for (const auto &[key, records] : b.takeZoneItems())
                      zoneReplies.push_back(key);
                  return collectSlab(replayed) && zoneReplies.size() >= 3;
              }, 15000) && replayed == placed,
              "the zone request replays the same ordered placement records");
        b.requestZones({});
        b.setOwn(rb);
        check(zoneReplies == std::vector{neighbour, aliceZone, neighbour},
              "zone item replies keep request order and duplicates and omit an invalid zone");
        check(b.takeZonePlacements().empty(), "packet 11 replays block edits without a placed-object snapshot");
        // Missions §6.2: packet 12 brings the region's 64 cell records in section 13.
        std::vector<std::array<uint8_t, 56>> cells;
        const std::pair<int32_t, int32_t> region{zx / 64, zy / 64};
        a.requestRegions(std::array{region});
        check(waitFor([&] {
                  for (auto &r : a.takeMissionRecords())
                      cells.push_back(r);
                  return cells.size() >= 64;
              }),
              "a region request brings its 64 cell records");
        a.requestRegions({});
        int32_t missionCount = 0;
        for (const auto &r : cells)
            missionCount += r[24] != 0;
        check(missionCount > 0, "the region response contains its restored or generated missions");
        // Packets §4.4: duplicate requests are not merged; each copy is answered.
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        a.takeMissionRecords();
        a.requestRegions(std::array{region, region});
        size_t answered = 0;
        check(waitFor([&] {
                  for (auto &r : a.takeMissionRecords()) {
                      int32_t x = 0, y = 0;
                      MissionArea::fromRecord(r.data(), x, y);
                      answered += std::pair(x / 8, y / 8) == region;
                  }
                  return answered >= 128;
              }),
              "a repeated region request is answered once per copy");
        a.requestRegions({});
    }
    {
        uint64_t target = 0;
        int32_t expectedCredit = 0, expectedType = 0;
        check(waitFor([&] {
            for (const auto &[id, record] : a.others()) {
                const auto actor = record.fighter();
                if (actor.controller == 1 && actor.health > 0 && (actor.appearanceFlags & 0x2000) &&
                    actor.backing.x == 32800 && actor.backing.y == 32800) {
                    target = id;
                    int32_t field38 = 0;
                    std::memcpy(&field38, record.bytes.data() + 0x19c, sizeof field38);
                    check(field38 == -1, "the server-sent NPC keeps the signed construction default in field 38");
                    constexpr int64_t centre = int64_t(32800 * 256 + 128) * 65536;
                    check(actor.behaviorReference[0] == centre && actor.behaviorReference[1] == centre &&
                          id == creatures::Npcs::spawnId(32800, 32800, actor.backing.index),
                          "the NPC publishes the regional descriptor ordinal and the separate fixed-point spawn centre");
                    expectedType = actor.type;
                    expectedCredit = (int32_t)creatures::fatalCreditAmount(actor.level, actor.powerTier, actor.type, actor.appearanceFlags);
                }
            }
            return target != 0;
        }, 10000), "the real server activates the stored regional descriptor with its cached origin");
        if (target) {
            a.takeMissionRecords(); b.takeMissionRecords(); a.takeGifts(); b.takeGifts(); a.takeCredits(); b.takeCredits();
            a.sendHit(Hit::make(a.id(), target, 1000000, 0, {}, 0));
            std::optional<std::array<uint8_t, 56>> first, second;
            const auto collect = [&](Client &client, auto &result) {
                for (const auto &record : client.takeMissionRecords()) {
                    int32_t x = 0, y = 0;
                    const auto mission = MissionArea::fromRecord(record.data(), x, y);
                    if (x == 4100 && y == 4100 && mission.seed == 0x31415926 && mission.state == 2)
                        result = record;
                }
            };
            check(waitFor([&] {
                collect(a, first); collect(b, second);
                return first.has_value() && second.has_value();
            }) && first == second, "actual fatal application publishes the same completed regional snapshot to both peers");
            std::vector<creatures::ItemRecord> rewards, otherRewards;
            check(waitFor([&] {
                for (const auto &item : a.takeGifts()) rewards.push_back(item);
                for (const auto &item : b.takeGifts()) otherRewards.push_back(item);
                return rewards.size() >= 2 && otherRewards.size() >= 2;
            }) && rewards.size() == 2 && otherRewards.size() == 2 &&
                  rewards[0][0] == 13 && creatures::itemLevel(rewards[0]) == 9 && rewards[1][0] >= 3 && rewards[1][0] <= 9 &&
                  otherRewards[0][0] == 13 && creatures::itemLevel(otherRewards[0]) == 9 && otherRewards[1][0] >= 3 && otherRewards[1][0] <= 9,
                  "the synchronous recipient sequence emits Platinum then gear once for both peers, including the peer that did not send the kill hit");
            std::vector<std::array<uint8_t, 24>> credits, otherCredits;
            const auto collectCredits = [&](Client &client, auto &records) {
                for (const auto &credit : client.takeCredits())
                    if (credit.victim == target) records.push_back(credit.serialize());
            };
            check(waitFor([&] {
                collectCredits(a, credits); collectCredits(b, otherCredits);
                return !credits.empty() && credits == otherCredits;
            }), "both peers receive identical ordered complete credit records from actual fatal application");
            size_t ownCredits = 0;
            for (const auto &bytes : credits) {
                const auto credit = creatures::KillRecord::parse(bytes.data());
                ownCredits += credit.player == a.id();
                check(credit.victimType == expectedType && credit.experience == expectedCredit,
                      "actual server credit retains the victim kind and prepared amount");
            }
            check(ownCredits == 1, "the later corpse adapter cannot duplicate the actual killer's synchronous credit");
            SaveDb saved;
            check(saved.open(options.folder / "Save" / "world_server_2412123.db", SaveDb::Mode::ReadOnly),
                  "completed synthetic server state opens read-only");
            const auto value = saved.get("mission4100_4100");
            const auto mission = value ? MissionArea::parse(*value) : std::nullopt;
            check(mission && mission->state == 2 && mission->seed == 0x31415926,
                  "the actual server saves its fatal regional mutation in the synthetic world");
        }
    }
    {
        auto state = ra.fighter();
        state.mode = 0xa610;
        state.equipment[12].type = 19;
        state.equipment[12].subtype = 19;
        state.equipment[12].level = 1;
        ra.setFighter(state);
        a.setOwn(ra);
        check(waitFor([&] {
            const auto peers = b.others();
            return peers.contains(a.id()) && peers.at(a.id()).fighter().mode == 0xa610;
        }), "the second peer receives the owner's full gliding mode word");
        const auto receivedPet = [&]() -> std::optional<CreatureRecord> {
            for (const auto &[key, record] : a.others()) {
                if (record.controller() == 5 && record.fighter().ownerId == a.id())
                    return record;
            }
            return std::nullopt;
        };
        bool glidingPet = receivedPet().has_value();
        uint32_t initialDay = 0, initialMs = 0;
        check(a.clock(initialDay, initialMs), "the glider fixture has a server clock baseline");
        check(waitFor([&] {
            glidingPet |= receivedPet().has_value();
            uint32_t currentDay = 0, currentMs = 0;
            return a.clock(currentDay, currentMs) &&
                (currentDay != initialDay || uint32_t(currentMs - initialMs) >= 5000);
        }), "the real server advances while the owner remains gliding");
        check(!glidingPet, "decoded glider mode prevents actual server pet construction");
        state.mode &= ~0x10u;
        ra.setFighter(state);
        a.setOwn(ra);
        std::optional<CreatureRecord> pet;
        check(waitFor([&] { pet = receivedPet(); return pet.has_value(); }),
              "clearing only glider admits the same equipped pet on the actual server");
        if (pet) {
            check(pet->homeSpawn() == std::array<int32_t, 3>{-1, -1, 0} &&
                  pet->fighter().behaviorReference == std::array<int64_t, 3>{} && pet->talkKind() == 0 &&
                  pet->talkTarget() == glm::ivec2(-1, -1),
                  "the new pet keeps the engine's zero field 40 and the reset backing and dialogue coordinates");
        }
        check(waitFor([&] {
            const auto peers = b.others();
            return peers.contains(a.id()) && peers.at(a.id()).fighter().mode == 0xa600;
        }), "the nonmovement high-byte flags remain unchanged after pet admission");
    }
    a.disconnect();
    check(waitFor([&] { return !b.others().count(1); }), "b sees Alice leave");
    check(waitFor([&] { return server.players() == 1; }), "server is down to one player");

    // An eleventh player is told the server is full.
    std::vector<std::unique_ptr<Client>> crowd;
    for (int i = 0; i < 9; i++) {
        crowd.push_back(std::make_unique<Client>());
        crowd.back()->connect("127.0.0.1", port);
    }
    Client late;
    check(!late.connect("127.0.0.1", port, &error) && error == "Server is full.", "eleventh player: server is full");
    crowd.clear();
    b.disconnect();
    server.stop();
    {
        // Net server §1.2: the step saves no zones; Alice's zone, with its placed slab's block
        // edits, is written when it is unloaded after she leaves (§2.2) or at shutdown (§6).
        SaveDb saved;
        const auto record = saved.open(options.folder / "Save" / "world_server_2412123.db", SaveDb::Mode::ReadOnly)
                                ? saved.get(zoneKey(aliceZone.first, aliceZone.second))
                                : std::nullopt;
        const auto zone = record ? ZoneSave::parse(*record) : std::nullopt;
        check(zone && !zone->blockEdits.empty(), "shutdown saves the held zone with its block edits");
    }
    std::filesystem::remove_all(options.folder);

    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
