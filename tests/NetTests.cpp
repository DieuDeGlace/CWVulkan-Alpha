// Checks the network code: first the byte-level test vectors of docs/specs/alpha-net-packets.md
// §6, then a real server on a local port with two clients that see each other, chat, and see
// each other leave. Uses only the loopback interface.
#include "Bytes.h"
#include "ZlibStream.h"
#include "net/Client.h"
#include "net/Server.h"
#include <chrono>
#include <atomic>
#include <cstdio>
#include <cstring>
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
    const auto expectedHit = outboundHit.serialize();
    std::atomic<bool> finish{false}, received{false}, receivedHit{false};
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
        for (const auto &event : {first, second}) {
            const auto bytes = event.serialize();
            update.sections[11].emplace_back(bytes.begin(), bytes.end());
        }
        for (const auto &event : relationships) {
            const auto bytes = event.serialize();
            update.sections[10].emplace_back(bytes.begin(), bytes.end());
        }
        update.sections[5] = expectedStaticPayloads;
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
                packetOrder = received && receivedHit;
                if (received && receivedHit) break;
            } else if (packet == HIT) {
                std::array<uint8_t, 72> bytes{};
                if (!socket.readExact(bytes.data(), bytes.size())) return;
                receivedHit = bytes == expectedHit;
                coherentHit = receivedHit && latestOwn.health() == 137.5f;
                if (received && receivedHit) break;
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
        client.sendEvent40(expectedSend);
        client.sendHit(Hit::from(outboundHit));
        check(waitFor([&] { return worldSent.load(); }), "peer supplies input while the frame gate is held");
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        check(client.takeHits().empty() && client.takeBuffs().empty() && client.takeRelationships().empty() &&
              client.takeStaticStates().empty() &&
              client.others().empty() && !client.takeClock(),
              "incoming queue mutation and creature insertion wait until the current frame completes");
        creatures::Combatant actor;
        actor.health = 137.5f;
        CreatureRecord published;
        published.setFighter(actor);
        client.setOwn(published);
        frame.unlock();
        std::vector<creatures::BuffEvent> events;
        std::vector<creatures::RelationshipRecord> receivedRelationships;
        std::vector<std::vector<uint8_t>> receivedStatics;
        std::vector<Hit> hits;
        check(waitFor([&] {
            for (const auto &event : client.takeBuffs()) events.push_back(event);
            for (const auto &event : client.takeRelationships()) receivedRelationships.push_back(event);
            for (const auto &event : client.takeStaticStates()) receivedStatics.emplace_back(event.begin(), event.end());
            for (const auto &hit : client.takeHits()) hits.push_back(hit);
            return events.size() >= 2 && receivedRelationships.size() >= relationships.size() &&
                   receivedStatics.size() >= expectedStaticPayloads.size() && !hits.empty() && received && receivedHit;
        }), "actual client exchanges packet-8 and section-12 buff records");
        check(coherentHit && packetOrder,
              "sender captures completed frame state before hit records and sends hits before buff events");
        check(events.size() == 2 && events[0].serialize() == first.serialize() && events[1].serialize() == second.serialize(),
              "incoming buff order, nonlocal target, embedded sources and opaque bytes survive transport");
        check(client.takeBuffs().empty(), "received buff queue drains exactly once");
        check(receivedRelationships.size() == relationships.size() &&
              receivedRelationships[0].serialize() == relationships[0].serialize() &&
              receivedRelationships[1].serialize() == relationships[1].serialize() &&
              receivedRelationships[2].serialize() == relationships[2].serialize() && client.takeRelationships().empty(),
              "section-eleven relationships retain order, zero/self IDs, float bits and all opaque bytes before one frame drain");
        check(receivedStatics == expectedStaticPayloads && client.takeStaticStates().empty(),
              "ordered section-six snapshots preserve all 88 bytes through frame-protected publication and one drain");
        const auto clock = client.takeClock();
        uint32_t observedDay = 0, observedTime = 0;
        check(clock && clock->first == clockDay && clock->second == clockTime && !client.takeClock() &&
              client.clock(observedDay, observedTime) && observedDay == clockDay && observedTime == clockTime,
              "fresh clock publication retains both raw words, drains once and keeps historical lookup separate");
        check(hits.size() == 1 && hits[0].record().serialize() == inboundHit.serialize() && client.takeHits().empty(),
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
        }
        followupRequested = true;
        check(waitFor([&] {
            const auto peers = client.others();
            const auto it = peers.find(19);
            return it != peers.end() && it->second.health() == 123.25f;
        }), "existing creature accepts a health-only update with explicit opaque storage");
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
            check(record.health() == 123.25f && record.controller() == 3 &&
                      record.position(0) == 123456789 && opaquePreserved,
                  "later partial delta preserves prior semantic and opaque fields without reapplying defaults");
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

int main() {
    checkCombatTransport();
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
    // Block edits §6.2: blueprint 0 as a plain 10 x 10 x 6 slab.
    options.prefabs = [](const std::string &key) -> std::shared_ptr<const worldgen::Prefab> {
        if (key != "framework-floor-wood.cub")
            return nullptr;
        auto slab = std::make_shared<worldgen::Prefab>();
        slab->sx = slab->sy = 10, slab->sz = 6;
        slab->rgb.assign(10 * 10 * 6 * 3, 165);
        return slab;
    };
    std::filesystem::remove_all(options.folder);
    std::filesystem::create_directories(options.folder);
    options.bindAddress = "127.0.0.1";
    Server server(options);
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
    uint64_t npc = 0;
    check(waitFor(
              [&] {
                  for (const auto &[id, r] : a.others())
                      if (r.controller() != 0)
                          npc = id;
                  return npc != 0;
              },
              120000),
          "creatures come alive round Alice");
    check(npc >= ((uint64_t)32700 << 24), "with a spawn id");
    {
        uint64_t target = 0;
        for (const auto &[id, record] : a.others())
            if (record.controller() != 0 && record.health() > 0) {
                target = id;
                break;
            }
        check(target != 0, "relationship fixture finds a living mapped NPC");
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
    {
        const int32_t zx = (int32_t)(ra.position(0) >> 24), zy = (int32_t)(ra.position(1) >> 24);
        std::array<uint8_t, 300> drop{};
        drop[0] = 3, drop[0x0d] = 1;
        drop[0x128] = 6;
        a.sendItemRequest(drop);
        int32_t index = -1;
        check(waitFor([&] {
                  a.requestZones({{zx, zy}});
                  for (const auto &[key, records] : a.takeZoneItems())
                      if (key == std::make_pair(zx, zy))
                          for (size_t k = 0; k < records.size(); k++)
                              if (records[k][0] == 3 && records[k][0x0d] == 1)
                                  index = (int32_t)k;
                  return index >= 0;
              }),
              "the dropped sword lies in Alice's zone");
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
        // Block edits §6: a kind-7 request stamps the slab in front of Alice; its records come
        // in section 1 as the step makes them, and again whole when her zone is asked for.
        CreatureRecord placing = ra;
        const glm::vec3 aim(0, 6, 0);
        std::memcpy(&placing.bytes[0x150], &aim, 12);
        placing.bytes[0x17c] = 1, placing.bytes[0x17d] = 0;
        a.setOwn(placing);
        std::array<uint8_t, 300> build{};
        build[0x128] = 7;
        std::vector<std::array<uint8_t, 20>> placed;
        int polls = 0;
        check(waitFor(
                  [&] {
                      if (placed.empty() && polls++ % 200 == 0) // (asked again every 2 s until it lands)
                          a.sendItemRequest(build);
                      for (auto &e : a.takeBlockEdits())
                          placed.push_back(e);
                      return placed.size() >= 300;
                  },
                  15000),
              "the placed slab arrives in section 1");
        check(!placed.empty() && placed[0][0x0f] == 0x46 && placed[0][0x0c] == 165, "placed blocks are grey, type byte 0x46");
        size_t again = 0;
        check(waitFor([&] {
                  a.requestZones({{zx, zy}});
                  again += a.takeBlockEdits().size();
                  return again >= placed.size();
              }),
              "a zone request sends its whole edit list");
        // Missions §6.2: packet 12 brings the region's 64 cell records in section 13.
        std::vector<std::array<uint8_t, 56>> cells;
        check(waitFor([&] {
                  a.requestRegions({{zx / 64, zy / 64}});
                  for (auto &r : a.takeMissionRecords())
                      cells.push_back(r);
                  return cells.size() >= 64;
              }),
              "a region request brings its 64 cell records");
        int32_t missionCount = 0;
        for (const auto &r : cells)
            missionCount += r[24] != 0;
        check(missionCount > 0, "the daily pass gave Alice's region missions");
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
    std::filesystem::remove_all(options.folder);

    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
