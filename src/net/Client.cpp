#include "net/Client.h"
#include "ItemRequest.h"
#include "Bytes.h"
#include "ZlibStream.h"
#include <bit>
#include <cstring>
#include <set>
#include <utility>

namespace net {

Client::~Client() { disconnect(); }

bool Client::connect(const std::string &host, uint16_t port, std::string *error) {
    disconnect();
    auto fail = [&](const std::string &why) {
        if (error)
            *error = why;
        socket.close();
        return false;
    };
    if (!socket.connect(host, port, error))
        return false;
    // Transport §2.4.
    uint32_t reply = 0;
    if (!socket.sendU32(HELLO) || !socket.sendU32(VERSION) || !socket.readU32(reply))
        return fail("Connection error.");
    if (reply == HELLO) {
        uint32_t theirs = 0;
        socket.readU32(theirs);
        return fail("Server has different version (" + std::to_string(theirs) + ") than client (" +
                    std::to_string(VERSION) + ").");
    }
    if (reply == FULL)
        return fail("Server is full.");
    if (reply != ACCEPTED)
        return fail("Connection error."); // (an Alpha server answers a wrong version with 3)

    // Our id and the server's 4456-byte record of us, which Alpha reads into a buffer and
    // discards (transport §2.4): the client keeps its own character. Then the seed (engine
    // choice: Alpha's receiver thread reads packet 15; here the connect routine expects it).
    uint32_t packet = 0;
    uint64_t id = 0;
    CreatureRecord record;
    if (!socket.readU32(packet) || packet != CREATURE || !socket.readExact(&id, 8) ||
        !socket.readExact(record.bytes.data(), record.bytes.size()))
        return fail("Connection error.");
    if (!socket.readU32(packet) || packet != SEED || !socket.readU32(worldSeed))
        return fail("Connection error.");
    // The normal server supplies1..10; the native adoption boundary also admits
    // zero and preserves the prior actor identity for a signed-negative reply.
    if (std::bit_cast<int64_t>(id) >= 0)
        ownId = id;
    online = true;
    receiver = std::thread([this] { receiveLoop(); });
    sender = std::thread([this] { sendLoop(); });
    return true;
}

void Client::disconnect() {
    online = false;
    socket.shutdown();
    if (receiver.joinable())
        receiver.join();
    if (sender.joinable())
        sender.join();
    socket.close();
    std::lock_guard guard(lock);
    creatures.clear();
    buffEvents.clear();
    relationshipEvents.clear();
    credits.clear();
    outgoingEvents.clear();
    outgoingItems.clear();
    outgoingZones.clear();
    outgoingRegions.clear();
    missionRecords.clear();
    zoneItems.clear();
    zonePlacements.clear();
    staticStates.clear();
    blockEdits.clear();
    haveClock = false;
    pendingClock.reset();
    sentOnce = false;
}

void Client::setOwn(const CreatureRecord &record) {
    std::lock_guard guard(lock);
    own = record;
}

void Client::say(const std::string &text) {
    std::lock_guard guard(lock);
    outgoingChat.push_back(text);
}

void Client::sendEvent40(const std::array<uint8_t, 40> &record) {
    std::lock_guard guard(lock);
    outgoingEvents.push_back(record);
}

void Client::sendHit(const Hit &hit) {
    std::lock_guard guard(lock);
    outgoingHits.push_back(hit);
}

void Client::sendShoot(const std::array<uint8_t, 112> &record) {
    std::lock_guard guard(lock);
    outgoingShoots.push_back(record);
}

void Client::sendItemRequest(const std::array<uint8_t, 300> &record) {
    std::lock_guard guard(lock);
    outgoingItems.push_back(record);
}

std::vector<std::array<int32_t, 3>> Client::pendingStaticRequests() const {
    std::lock_guard guard(lock);
    std::vector<std::array<int32_t, 3>> targets;
    for (const auto &packet : outgoingItems)
        if (packet[0x128] == ItemRequest::UseStatic)
            targets.push_back(ItemRequest::parse(packet).target);
    return targets;
}

void Client::requestZones(std::span<const std::pair<int32_t, int32_t>> zones) {
    std::lock_guard guard(lock);
    outgoingZones.assign(zones.begin(), zones.end());
}

void Client::requestRegions(std::span<const std::pair<int32_t, int32_t>> regions) {
    std::lock_guard guard(lock);
    outgoingRegions.assign(regions.begin(), regions.end());
}

std::vector<std::array<uint8_t, 56>> Client::takeMissionRecords() {
    std::lock_guard guard(lock);
    return std::exchange(missionRecords, {});
}

std::vector<Client::ZoneItems> Client::takeZoneItems() {
    std::lock_guard guard(lock);
    return std::exchange(zoneItems, {});
}

std::vector<Client::ZonePlacements> Client::takeZonePlacements() {
    std::lock_guard guard(lock);
    return std::exchange(zonePlacements, {});
}

std::vector<std::array<uint8_t, 88>> Client::takeStaticStates() {
    std::lock_guard guard(lock);
    return std::exchange(staticStates, {});
}

std::vector<std::array<uint8_t, 20>> Client::takeBlockEdits() {
    std::lock_guard guard(lock);
    return std::exchange(blockEdits, {});
}

std::vector<std::array<uint8_t, 280>> Client::takeGifts() {
    std::lock_guard guard(lock);
    return std::exchange(gifts, {});
}

std::vector<std::array<uint8_t, 112>> Client::takeShoots() {
    std::lock_guard guard(lock);
    return std::exchange(shoots, {});
}

std::map<uint64_t, CreatureRecord> Client::others() const {
    std::lock_guard guard(lock);
    return creatures;
}

bool Client::clock(uint32_t &day, uint32_t &timeOfDayMs) const {
    std::lock_guard guard(lock);
    day = clockDay;
    timeOfDayMs = clockMs;
    return haveClock;
}

std::optional<std::pair<uint32_t, uint32_t>> Client::takeClock() {
    std::lock_guard guard(lock);
    return std::exchange(pendingClock, std::nullopt);
}

std::optional<std::vector<uint8_t>> Client::takeQuestBook() {
    std::lock_guard guard(lock);
    return std::exchange(questBook, std::nullopt);
}

std::vector<std::pair<uint64_t, std::string>> Client::takeChat() {
    std::lock_guard guard(lock);
    return std::exchange(chat, {});
}

std::vector<std::array<uint8_t, 72>> Client::takeBursts() {
    std::lock_guard guard(lock);
    return std::exchange(bursts, {});
}

std::vector<Client::Sound> Client::takeSounds() {
    std::lock_guard guard(lock);
    return std::exchange(sounds, {});
}

std::vector<::creatures::KillRecord> Client::takeCredits() {
    std::lock_guard guard(lock);
    return std::exchange(credits, {});
}

std::vector<Client::Receipt> Client::takeReceipts() {
    std::lock_guard guard(lock);
    return std::exchange(receipts, {});
}

void Client::setReceivedHealth(uint64_t key, float health) {
    std::lock_guard guard(lock);
    if (const auto actor = creatures.find(key); actor != creatures.end())
        actor->second.setHealth(health);
}

std::vector<Hit> Client::takeHits() {
    std::lock_guard guard(lock);
    return std::exchange(hits, {});
}

std::vector<::creatures::BuffEvent> Client::takeBuffs() {
    std::lock_guard guard(lock);
    return std::exchange(buffEvents, {});
}

std::vector<::creatures::RelationshipRecord> Client::takeRelationships() {
    std::lock_guard guard(lock);
    return std::exchange(relationshipEvents, {});
}

namespace {
SenderPacing clientPacing; // transport §3.3: the client image's one pacing stamp
} // namespace

void Client::sendLoop() {
    uint32_t requestStamp = networkTimeMs();
    std::vector<std::pair<int32_t, int32_t>> zones, regions;
    while (online) {
        // The native header precedes the locked queue/state snapshot. Encoding
        // and socket writes remain outside the frame gate.
        if (!socket.sendU32(CREATURE))
            break;
        CreatureRecord current, previous;
        bool first = false;
        std::optional<std::string> chatLine;
        std::vector<Hit> batchHits;
        std::vector<std::array<uint8_t, 40>> batchEvents;
        std::vector<std::array<uint8_t, 112>> batchShoots;
        std::optional<std::array<uint8_t, 300>> item;
        {
            auto frame = lockFrame();
            std::lock_guard guard(lock);
            if (!online)
                break;
            if (!outgoingChat.empty()) {
                chatLine = std::move(outgoingChat.front());
                outgoingChat.pop_front();
            }
            batchHits = std::exchange(outgoingHits, {});
            batchEvents = std::exchange(outgoingEvents, {});
            batchShoots = std::exchange(outgoingShoots, {});
            if (!outgoingItems.empty()) {
                item = outgoingItems.front();
                outgoingItems.pop_front();
            }
            current = own;
            previous = ownSent;
            first = !sentOnce;
        }
        // Packets §0.1: packet 0, at most one chat line, at most one item record, then every
        // hit, event-40 and shoot, then the requests.
        ByteWriter w;
        const uint64_t mask = changedFields(current, first ? nullptr : &previous);
        w.bytes(compressed(encodeDelta(ownId, current, mask)));
        if (chatLine) {
            const auto units = toUtf16(*chatLine);
            w.u32(CHAT);
            w.u32((uint32_t)units.size());
            w.write(units.data(), units.size() * 2);
        }
        if (item) {
            w.u32(ITEM_RECORD);
            w.write(item->data(), item->size());
        }
        for (const Hit &hit : batchHits) {
            w.u32(HIT);
            w.write(hit.bytes.data(), hit.bytes.size());
        }
        for (const auto &event : batchEvents) {
            w.u32(EVENT40);
            w.write(event.data(), event.size());
        }
        for (const auto &shot : batchShoots) {
            w.u32(SHOOT);
            w.write(shot.data(), shot.size());
        }
        if (!socket.send(w.data()))
            break;
        {
            std::lock_guard guard(lock);
            ownSent = current;
            sentOnce = true;
        }
        // Blueprint zone delivery §5. Take both live snapshots after the gameplay
        // batch. Retain the live lists and keep the socket writes outside the locks.
        {
            auto frame = lockFrame();
            std::lock_guard guard(lock);
            zones = outgoingZones;
            regions = outgoingRegions;
        }
        if (requestSnapshotDue(networkTimeMs(), requestStamp) && (!zones.empty() || !regions.empty())) {
            w.data().clear();
            for (const auto &[zx, zy] : zones) {
                w.u32(ZONE_REQUEST);
                w.i32(zx);
                w.i32(zy);
            }
            for (const auto &[rx, ry] : regions) {
                w.u32(REGION_REQUEST);
                w.i32(rx);
                w.i32(ry);
            }
            if (!socket.send(w.data()))
                break;
        }
        clientPacing.endPass();
    }
    // Engine choice: Alpha's sender closes the socket but leaves "online" set and prints
    // nothing (transport §5.2); here a failed connection returns the client to offline play.
    online = false;
    socket.shutdown();
}

void Client::commit(uint64_t id, CreatureRecord &&decoded, bool created) {
    // Remote smoothing §1.3 step 7, against the creature's old record (packets §2.3: created,
    // field 0x1B0 changed, or health <= 0 snaps; otherwise the creature interpolates).
    constexpr size_t SNAP_FIELD = 0x1B0, SNAP_BYTES = 24;
    CreatureRecord &held = creatures[id];
    const bool fieldChanged = std::memcmp(held.bytes.data() + SNAP_FIELD, decoded.bytes.data() + SNAP_FIELD, SNAP_BYTES) != 0;
    receipts.push_back({id, created || fieldChanged || 0.f >= held.health()});
    held = std::move(decoded);
}

void Client::receiveLoop() {
    // Receiver-private presence is consumed after the guarded batch reconciliation.
    std::set<uint64_t> updatedThisBatch;
    // Transport §4, §5.3: a zero length ends the receiver; a failed inflate keeps its partial
    // output. (The 16 MiB length bound is an engine choice.)
    auto readCompressed = [&](std::vector<uint8_t> &plain) {
        uint32_t length = 0;
        if (!socket.readU32(length) || length == 0 || length > (16u << 20))
            return false;
        std::vector<uint8_t> packed(length);
        if (!socket.readExact(packed.data(), length))
            return false;
        plain = zlibInflatePartial(packed);
        return true;
    };
    while (online) {
        uint32_t packet;
        if (!socket.readU32(packet))
            break;
        bool ok = true;
        switch (packet) {
        case CREATURE: {
            std::vector<uint8_t> plain;
            ok = readCompressed(plain) && plain.size() >= 16;
            if (!ok)
                break;
            uint64_t id;
            std::memcpy(&id, plain.data(), 8);
            CreatureRecord decoded;
            bool created = false;
            {
                auto frame = lockFrame();
                std::lock_guard guard(lock);
                if (id == ownId) {
                    decoded = own;
                } else {
                    auto [it, inserted] = creatures.try_emplace(id);
                    if (inserted)
                        it->second = CreatureRecord::nativeDefaults();
                    created = inserted;
                    decoded = it->second;
                }
            }
            // Native insertion/baseline copy precedes this unlocked decode. It is
            // deliberately a separate publication from the final state assignment.
            ok = applyDelta(plain, decoded).has_value();
            if (!ok)
                break;
            updatedThisBatch.insert(id);
            {
                auto frame = lockFrame();
                std::lock_guard guard(lock);
                if (id != ownId && creatures.contains(id))
                    commit(id, std::move(decoded), created);
            }
            break;
        }
        case CREATURE_BATCH: {
            // Packets §3.2: i32 count, then per creature u64 id, u64 mask and the masked fields,
            // uncompressed. Each entry is gathered as a packet-0 plain payload and decoded below.
            int32_t count = 0;
            ok = socket.readExact(&count, 4);
            std::vector<std::vector<uint8_t>> entries;
            for (int32_t k = 0; ok && k < count; k++) {
                std::vector<uint8_t> plain(16);
                ok = socket.readExact(plain.data(), 16);
                uint64_t mask = 0;
                std::memcpy(&mask, plain.data() + 8, 8);
                ok = ok && (mask >> 48) == 0; // (engine choice: bits 48-63 are never set)
                for (size_t n = 0; ok && n < CREATURE_FIELDS.size(); n++)
                    if (mask & (1ull << n)) {
                        const size_t at = plain.size();
                        plain.resize(at + CREATURE_FIELDS[n].size);
                        ok = socket.readExact(plain.data() + at, CREATURE_FIELDS[n].size);
                    }
                entries.push_back(std::move(plain));
            }
            if (!ok)
                break;
            auto frame = lockFrame();
            std::lock_guard guard(lock);
            // Decoded onto copies of the current records; then every creature not in the batch is
            // deleted (all of them for a count <= 0); then the others' records are committed.
            struct Decoded {
                uint64_t id;
                CreatureRecord record;
                bool created;
            };
            std::vector<Decoded> decoded;
            std::set<uint64_t> listed;
            for (const auto &plain : entries) {
                uint64_t id;
                std::memcpy(&id, plain.data(), 8);
                const auto held = creatures.find(id);
                const bool created = id != ownId && held == creatures.end();
                Decoded d{id, id == ownId ? own : created ? CreatureRecord::nativeDefaults() : held->second, created};
                applyDelta(plain, d.record);
                listed.insert(id);
                if (id != ownId)
                    decoded.push_back(std::move(d));
            }
            std::erase_if(creatures, [&](const auto &entry) { return !listed.contains(entry.first); });
            for (Decoded &d : decoded)
                commit(d.id, std::move(d.record), d.created);
            updatedThisBatch.insert(listed.begin(), listed.end());
            break;
        }
        case STATIC_OBJECTS: {
            // Packets §3.3: i32 count; for count <= 0 nothing else is read. The engine keeps no
            // static-object map (Alpha's server never sends this), so the records are read and
            // dropped to keep the stream in step.
            int32_t count = 0;
            ok = socket.readExact(&count, 4);
            std::array<uint8_t, 120> record;
            for (int32_t k = 0; ok && k < count; k++)
                ok = socket.readExact(record.data(), record.size());
            break;
        }
        case END_OF_BATCH: { // packets §2.4: whoever was not listed is gone
            {
                auto frame = lockFrame();
                std::lock_guard guard(lock);
                for (auto it = creatures.begin(); it != creatures.end();)
                    it = updatedThisBatch.count(it->first) ? std::next(it) : creatures.erase(it);
            }
            updatedThisBatch.clear();
            break;
        }
        case CLOCK: {
            uint32_t day = 0, ms = 0;
            ok = socket.readU32(day) && socket.readU32(ms);
            if (!ok)
                break;
            auto frame = lockFrame();
            std::lock_guard guard(lock);
            clockDay = day;
            clockMs = ms;
            haveClock = true;
            pendingClock = {day, ms};
            break;
        }
        case WORLD_UPDATE: {
            std::vector<uint8_t> plain;
            ok = readCompressed(plain);
            const auto update = ok ? WorldUpdate::decode(plain) : std::nullopt;
            ok = update.has_value();
            if (!ok)
                break;
            auto frame = lockFrame();
            std::lock_guard guard(lock);
            // Section 2 only: target application and foreign-source presentation are separate
            // frame decisions. Section 3 is not hit handling (packets §3.4); it is the bursts below.
            for (const auto &bytes : update->sections[1]) {
                Hit h;
                std::memcpy(h.bytes.data(), bytes.data(), h.bytes.size());
                hits.push_back(h);
            }
            for (const auto &bytes : update->sections[12]) // section 13: map-cell missions
                if (bytes.size() == 56) {
                    std::array<uint8_t, 56> record;
                    std::memcpy(record.data(), bytes.data(), 56);
                    missionRecords.push_back(record);
                }
            for (const auto &bytes : update->sections[0]) // section 1: block edits
                if (bytes.size() == 20) {
                    std::array<uint8_t, 20> record;
                    std::memcpy(record.data(), bytes.data(), 20);
                    blockEdits.push_back(record);
                }
            for (const auto &bytes : update->sections[5]) // section 6: static-object states
                if (bytes.size() == 88) {
                    std::array<uint8_t, 88> record;
                    std::memcpy(record.data(), bytes.data(), 88);
                    staticStates.push_back(record);
                }
            for (const auto &list : update->keyed[0]) { // section 7: a zone's world items
                ZoneItems z{{(int32_t)(uint32_t)list.key, (int32_t)(uint32_t)(list.key >> 32)}, {}};
                for (const auto &bytes : list.records)
                    if (bytes.size() == 328) {
                        std::array<uint8_t, 328> record;
                        std::memcpy(record.data(), bytes.data(), 328);
                        z.second.push_back(record);
                    }
                zoneItems.push_back(std::move(z));
            }
            for (const auto &list : update->keyed[1]) { // section 8: changed placed-object lists
                ZonePlacements snapshot{{(int32_t)(uint32_t)list.key, (int32_t)(uint32_t)(list.key >> 32)}, {}};
                snapshot.second.reserve(list.records.size());
                for (const auto &bytes : list.records) {
                    std::array<int32_t, 4> placement;
                    std::memcpy(placement.data(), bytes.data(), sizeof placement);
                    snapshot.second.push_back(placement);
                }
                zonePlacements.push_back(std::move(snapshot));
            }
            for (const auto &bytes : update->sections[4]) // section 5: shoot records
                if (bytes.size() == 112) {
                    std::array<uint8_t, 112> record;
                    std::memcpy(record.data(), bytes.data(), 112);
                    shoots.push_back(record);
                }
            for (const auto &bytes : update->sections[2]) { // section 3: particle bursts
                std::array<uint8_t, 72> record;
                std::memcpy(record.data(), bytes.data(), 72);
                bursts.push_back(record);
            }
            for (const auto &bytes : update->sections[3]) { // section 4: 24-byte sounds
                Sound s;
                std::memcpy(&s.position, bytes.data(), 12);
                std::memcpy(&s.id, &bytes[12], 4);
                std::memcpy(&s.pitch, &bytes[16], 4);
                std::memcpy(&s.volume, &bytes[20], 4);
                sounds.push_back(s);
            }
            for (const auto &bytes : update->sections[10])
                relationshipEvents.push_back(::creatures::RelationshipRecord::parse(bytes.data()));
            for (const auto &bytes : update->sections[11])
                buffEvents.push_back(::creatures::BuffEvent::parse(bytes.data()));
            // Section 9 grants inventory items. Category 13 credits Platinum
            // Coins through the same item route; kill experience is section 10.
            for (const auto &bytes : update->sections[8]) {
                if (bytes.size() < 0x1a)
                    continue;
                uint64_t who;
                std::memcpy(&who, bytes.data(), 8);
                if (who == ownId && bytes.size() >= 288) {
                    std::array<uint8_t, 280> item;
                    std::memcpy(item.data(), &bytes[8], 280);
                    gifts.push_back(item);
                }
            }
            // Preserve each complete record. Even a foreign recipient can have
            // designated-victim cleanup at the later frame consumer.
            for (const auto &bytes : update->sections[9])
                if (bytes.size() >= 24)
                    credits.push_back(::creatures::KillRecord::parse(bytes.data()));
            break;
        }
        case CHAT: {
            // Packets §3.10: for n <= 0 nothing more is read. (The 4096-unit bound is an engine choice.)
            uint64_t from = 0;
            int32_t n = 0;
            ok = socket.readExact(&from, 8) && socket.readExact(&n, 4) && n <= 4096;
            std::vector<uint16_t> units(ok && n > 0 ? size_t(n) : 0);
            ok = ok && socket.readExact(units.data(), units.size() * 2);
            if (ok && n > 0) {
                auto frame = lockFrame();
                std::lock_guard guard(lock);
                if (avMessage(units, 'Q')) { // our quest book, not a line to show
                    std::vector<uint8_t> bytes;
                    for (size_t k = 4; k < units.size(); k++)
                        bytes.push_back((uint8_t)units[k]);
                    questBook = std::move(bytes);
                } else if (units.size() >= 3 && units[0] == 1 && units[1] == 'A' && units[2] == 'V') {
                    // (Another AlphaVulkan message, such as a hello an Alpha server passed on as chat.)
                } else {
                    chat.emplace_back(from, fromUtf16(units));
                }
            }
            break;
        }
        case SEED: {
            uint32_t seed;
            ok = socket.readU32(seed);
            if (ok) {
                auto frame = lockFrame();
                std::lock_guard guard(lock);
                worldSeed = seed;
            }
            break;
        }
        default:
            // Engine choice: Alpha ignores the id and reads on (misreading any payload,
            // transport §3); here the stream cannot be trusted any longer.
            ok = false;
        }
        if (!ok)
            break;
    }
    // Engine choice: Alpha's receiver only ends (transport §5.2); here it also ends the
    // connection. An orderly close at a message boundary ends it in both (§5.3).
    online = false;
    socket.shutdown();
}

} // namespace net
