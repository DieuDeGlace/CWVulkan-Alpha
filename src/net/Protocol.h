#pragma once
// Cube World Alpha's network protocol (docs/specs/alpha-net-transport.md, alpha-net-packets.md):
// one TCP stream per player on port 12345, each message a u32 packet id and its payload. Only
// creature updates (0) and the world update (4) are zlib-compressed. This file holds the
// records and the encoding; sockets and threads are elsewhere.
#include "creatures/Appearance.h"
#include "creatures/Combat.h"
#include <cstring>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace net {

constexpr uint16_t PORT = 12345;
constexpr uint32_t VERSION = 3;
constexpr int MAX_PLAYERS = 10; // player ids are 1..10
constexpr int TICK_MS = 20;

// Packet ids (packets §0, transport §2).
enum Packet : uint32_t {
    CREATURE = 0,
    END_OF_BATCH = 2,
    WORLD_UPDATE = 4,
    CLOCK = 5,
    ITEM_RECORD = 6,
    HIT = 7,
    EVENT40 = 8,
    SHOOT = 9,
    CHAT = 10,
    ZONE_REQUEST = 11,
    REGION_REQUEST = 12,
    SEED = 15,
    ACCEPTED = 16,
    HELLO = 17,
    FULL = 18,
    WRONG_VERSION = 3, // what an Alpha server sends a wrong version (transport §2.2)
};

// Packets §2.1: a creature as the network sees it, 4456 bytes, cut into 48 fields.
struct CreatureRecord {
    static constexpr size_t SIZE = 0x1168;
    std::array<uint8_t, SIZE> bytes{};

    // Semantic seed for a newly received creature, before its first delta.
    // Native unwritten storage is unspecified; this engine initializes it to zero.
    static CreatureRecord nativeDefaults();

    // A few fields by name; everything else stays as bytes and is relayed unchanged.
    int64_t position(int axis) const;
    void setPosition(int axis, int64_t value);
    uint8_t controller() const { return bytes[0x50]; } // 0 = player
    glm::vec3 orientation() const { return floats(0x18); } // field 1; z is the facing
    glm::vec3 velocity() const { return floats(0x24); }    // field 2, blocks per second
    glm::vec3 intent() const { return floats(0x30); }      // field 3
    glm::vec3 response() const { return floats(0x3c); }    // field 4, separate from velocity
    float yaw() const { return orientation().z; }
    void setMotion(glm::vec3 velocity, float yaw, glm::vec3 response = {}, glm::vec3 intent = {}) {
        glm::vec3 o = orientation();
        o.z = yaw;
        std::memcpy(&bytes[0x18], &o, sizeof o);
        std::memcpy(&bytes[0x24], &velocity, sizeof velocity);
        std::memcpy(&bytes[0x30], &intent, sizeof intent);
        std::memcpy(&bytes[0x3c], &response, sizeof response);
    }
    // Field 13 (offset 0x68): the 172-byte appearance (alpha-creatures-models.md §3).
    creatures::Appearance appearance() const { return creatures::Appearance::parse(&bytes[0x68]); }
    void setAppearance(const creatures::Appearance &a) { a.serialize(&bytes[0x68]); }
    float health() const;
    // The fighting state other clients draw and the server relays (behaviour spec §1): type,
    // controller, action and its time, combo, class, charge, health, mana, level, experience,
    // the stat multipliers and the equipment (models spec §6.4).
    creatures::Combatant fighter() const;
    void setFighter(const creatures::Combatant &c);
    std::string name() const;
    void setName(const std::string &name);

  private:
    glm::vec3 floats(size_t at) const {
        glm::vec3 v;
        std::memcpy(&v, &bytes[at], sizeof v);
        return v;
    }
};

// Packets §2.1: one field of the record.
struct CreatureField {
    uint16_t offset, size;
    enum Kind : uint8_t { BYTES, FLOATS, NAME } kind; // how a change is detected (§2.2)
};
extern const std::array<CreatureField, 48> CREATURE_FIELDS;
constexpr uint64_t ALL_FIELDS = 0x0000FFFFFFFFFFFFull;

// §2.2: the fields of `now` that differ from `before` (all of them when `before` is null).
uint64_t changedFields(const CreatureRecord &now, const CreatureRecord *before);
// The plain payload: u64 id, u64 mask, the masked fields' new bytes.
std::vector<uint8_t> encodeDelta(uint64_t id, const CreatureRecord &now, uint64_t mask);
// Applies a plain payload to `record`; returns the id, or nothing if it is malformed.
std::optional<uint64_t> applyDelta(const std::vector<uint8_t> &plain, CreatureRecord &record);

// Transport §4: u32 compressed length + zlib stream (never empty).
std::vector<uint8_t> compressed(const std::vector<uint8_t> &plain);

// Packets §3.4: the thirteen lists of a world update. Sections 7 and 8 are keyed lists.
struct WorldUpdate {
    static constexpr uint32_t RECORD_SIZE[13] = {20, 72, 72, 24, 112, 88, 328, 16, 288, 24, 24, 40, 56};
    struct KeyedList {
        uint64_t key = 0;
        std::vector<std::vector<uint8_t>> records;
    };
    std::vector<std::vector<uint8_t>> sections[13]; // for keyed sections (index 6, 7): unused
    std::vector<KeyedList> keyed[2];                 // sections 7 and 8

    bool empty() const;
    std::vector<uint8_t> encode() const; // plain
    static std::optional<WorldUpdate> decode(const std::vector<uint8_t> &plain);
};

// Packets §5.1: the fields of a hit this code reads.
struct Hit {
    std::array<uint8_t, 72> bytes{};
    uint64_t attacker() const;
    uint64_t target() const;
    float amount() const;
    int32_t stun() const;
    int32_t lockMs() const;
    glm::vec3 knockBack() const;
    uint8_t kind() const { return bytes[0x45]; }
    creatures::HitRecord record() const { return creatures::HitRecord::parse(bytes.data()); }
    static Hit from(const creatures::HitRecord &record) { return {record.serialize()}; }
    // A record from the fields the engine knows (the unknown bytes stay 0).
    static Hit make(uint64_t attacker, uint64_t target, float amount, int32_t stun, glm::vec3 knockBack, uint8_t kind);
};

// UTF-16 chat text (packets §3.9, §4.3) to and from UTF-8.
// AlphaVulkan's own messages, riding on chat lines whose first unit is 1 (no Alpha client sends
// or shows such a line, docs/design/quests.md): a client's hello, which tells the server it
// may send the rest, and the server's copy of that player's quest book (one byte per unit).
inline bool avMessage(const std::vector<uint16_t> &units, char kind) {
    return units.size() >= 4 && units[0] == 1 && units[1] == 'A' && units[2] == 'V' && units[3] == (uint16_t)kind;
}
inline std::vector<uint16_t> avPacket(char kind, const std::vector<uint8_t> &bytes = {}) {
    std::vector<uint16_t> units{1, 'A', 'V', (uint16_t)kind};
    units.insert(units.end(), bytes.begin(), bytes.end());
    return units;
}
std::vector<uint16_t> toUtf16(const std::string &text);
std::string fromUtf16(const std::vector<uint16_t> &units);

} // namespace net
