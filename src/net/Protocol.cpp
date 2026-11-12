#include "net/Protocol.h"
#include "Bytes.h"
#include "ZlibStream.h"
#include <cstring>
#include <utility>

namespace net {

// Packets §2.1. Bits 41 and 42 are in the opposite order to their offsets.
const std::array<CreatureField, 48> CREATURE_FIELDS = {{
    {0x000, 24, CreatureField::BYTES},  {0x018, 12, CreatureField::FLOATS}, {0x024, 12, CreatureField::FLOATS},
    {0x030, 12, CreatureField::FLOATS}, {0x03c, 12, CreatureField::FLOATS}, {0x048, 4, CreatureField::FLOATS},
    {0x04c, 4, CreatureField::BYTES},   {0x050, 1, CreatureField::BYTES},   {0x054, 4, CreatureField::BYTES},
    {0x058, 1, CreatureField::BYTES},   {0x05c, 4, CreatureField::BYTES},   {0x060, 4, CreatureField::BYTES},
    {0x064, 4, CreatureField::BYTES},   {0x068, 172, CreatureField::BYTES}, {0x114, 2, CreatureField::BYTES},
    {0x118, 4, CreatureField::BYTES},   {0x11c, 4, CreatureField::BYTES},   {0x120, 4, CreatureField::BYTES},
    {0x124, 4, CreatureField::BYTES},   {0x128, 4, CreatureField::BYTES},   {0x12c, 4, CreatureField::FLOATS},
    {0x130, 1, CreatureField::BYTES},   {0x131, 1, CreatureField::BYTES},   {0x134, 4, CreatureField::FLOATS},
    {0x138, 12, CreatureField::FLOATS}, {0x144, 12, CreatureField::FLOATS}, {0x150, 12, CreatureField::FLOATS},
    {0x15c, 4, CreatureField::FLOATS},  {0x160, 4, CreatureField::FLOATS},  {0x164, 4, CreatureField::FLOATS},
    {0x168, 20, CreatureField::FLOATS}, {0x17c, 1, CreatureField::BYTES},   {0x17d, 1, CreatureField::BYTES},
    {0x180, 4, CreatureField::BYTES},   {0x184, 4, CreatureField::BYTES},   {0x188, 8, CreatureField::BYTES},
    {0x190, 8, CreatureField::BYTES},   {0x198, 1, CreatureField::BYTES},   {0x19c, 4, CreatureField::BYTES},
    {0x1a0, 12, CreatureField::BYTES},  {0x1b0, 24, CreatureField::BYTES},  {0x1cc, 12, CreatureField::BYTES},
    {0x1c8, 1, CreatureField::BYTES},   {0x1d8, 280, CreatureField::BYTES}, {0x2f0, 3640, CreatureField::BYTES},
    {0x1158, 16, CreatureField::NAME},  {0x1128, 44, CreatureField::BYTES}, {0x1154, 4, CreatureField::BYTES},
}};

int64_t CreatureRecord::position(int axis) const {
    int64_t v;
    std::memcpy(&v, &bytes[8 * axis], 8);
    return v;
}

void CreatureRecord::setPosition(int axis, int64_t value) { std::memcpy(&bytes[8 * axis], &value, 8); }

float CreatureRecord::health() const {
    float v;
    std::memcpy(&v, &bytes[0x15c], 4);
    return v;
}

namespace {

template <class T> T get(const std::array<uint8_t, CreatureRecord::SIZE> &b, size_t at) {
    T v;
    std::memcpy(&v, &b[at], sizeof v);
    return v;
}
template <class T> void put(std::array<uint8_t, CreatureRecord::SIZE> &b, size_t at, T v) { std::memcpy(&b[at], &v, sizeof v); }

constexpr size_t EQUIPMENT = 0x2f0, ITEM_SIZE = 0x118;

} // namespace

CreatureRecord CreatureRecord::nativeDefaults() {
    // alpha-network-creature-state-defaults.md. No stat queries or random draws.
    // Zeroing unspecified bytes is an engine choice, not a native byte-parity claim.
    CreatureRecord record;
    auto &b = record.bytes;
    b[0x50] = 3;
    put<int32_t>(b, 0x11c, -3000);
    put<float>(b, 0x15c, 500.f);
    put<float>(b, 0x168, 100.f);
    for (size_t at = 0x16c; at <= 0x178; at += 4)
        put<float>(b, at, 1.f);
    put<int32_t>(b, 0x180, 1);
    put<int32_t>(b, 0x19c, -1);
    for (size_t at : {size_t(0x1a0), size_t(0x1cc)}) {
        put<uint32_t>(b, at, UINT32_MAX);
        put<uint32_t>(b, at + 4, UINT32_MAX);
    }

    constexpr size_t appearance = 0x68;
    b[appearance + 2] = b[appearance + 3] = b[appearance + 4] = 0xff;
    for (size_t at = 0x14; at <= 0x22; at += 2)
        put<uint16_t>(b, appearance + at, 0xffff);
    // Preserve fixed binary32 constants directly, including the rounded fractions.
    constexpr std::pair<size_t, uint32_t> appearanceValues[] = {
        {0x08, 0x3f800000}, {0x0c, 0x3f800000}, {0x10, 0x3f800000},
        {0x24, 0x3f8147ae}, {0x28, 0x3f800000}, {0x2c, 0x3f800000},
        {0x30, 0x3f7ae148}, {0x34, 0x3f800000}, {0x38, 0x3f733333},
        {0x3c, 0x3f4ccccd}, {0x40, 0x3f800000}, {0x44, 0x3f800000},
        {0x6c, 0xc0a00000}, {0x74, 0x3f000000}, {0x78, 0x40a00000},
        {0x7c, 0x40c00000}, {0x88, 0x40400000}, {0x8c, 0x3f800000},
        {0x90, 0xc1280000}, {0x98, 0xc1000000}, {0x9c, 0x40000000},
    };
    for (const auto &[at, bits] : appearanceValues)
        put<uint32_t>(b, appearance + at, bits);

    put<uint16_t>(b, 0x1d8 + 0x10, 1);
    for (size_t slot = 0; slot < 13; ++slot)
        put<uint16_t>(b, EQUIPMENT + ITEM_SIZE * slot + 0x10, 1);
    return record;
}

creatures::Combatant CreatureRecord::fighter() const {
    creatures::Combatant c;
    c.appearanceFlags = get<uint16_t>(bytes, 0x06e); // appearance field13, flags at +6
    c.mode = get<uint16_t>(bytes, 0x114);
    c.controller = bytes[0x050];
    c.type = get<int32_t>(bytes, 0x054);
    c.action = bytes[0x058];
    c.actionMs = get<int32_t>(bytes, 0x05c);
    c.combo = get<int32_t>(bytes, 0x060);
    c.comboElapsedMs = get<int32_t>(bytes, 0x064);
    c.dodgeMs = (float)get<int32_t>(bytes, 0x118);
    c.hitStun = get<int32_t>(bytes, 0x11c);
    c.actionLockMs = get<int32_t>(bytes, 0x120);
    c.slowMs = get<int32_t>(bytes, 0x124);
    c.hasteMs = get<int32_t>(bytes, 0x128);
    c.recoveryPenalty = get<float>(bytes, 0x12c);
    c.characterClass = bytes[0x130];
    c.specialization = bytes[0x131];
    c.charge = get<float>(bytes, 0x134);
    c.health = get<float>(bytes, 0x15c);
    c.mana = get<float>(bytes, 0x160);
    c.blockGauge = get<float>(bytes, 0x164);
    for (int i = 0; i < 5; i++)
        c.multipliers[i] = get<float>(bytes, 0x168 + 4 * i);
    c.level = get<int32_t>(bytes, 0x180);
    c.experience = get<int32_t>(bytes, 0x184);
    c.ownerId = get<uint64_t>(bytes, 0x188);
    c.healTarget = get<uint64_t>(bytes, 0x190);
    c.powerTier = bytes[0x198];
    c.backing = {get<int32_t>(bytes, 0x1a0), get<int32_t>(bytes, 0x1a4), get<int32_t>(bytes, 0x1a8)};
    for (size_t axis = 0; axis < c.behaviorReference.size(); axis++)
        c.behaviorReference[axis] = get<int64_t>(bytes, 0x1b0 + 8 * axis);
    creatures::ItemRecord held;
    std::memcpy(held.data(), bytes.data() + 0x1d8, ITEM_SIZE);
    c.setHeld(held);
    for (size_t slot = 0; slot < c.equipment.size(); slot++) {
        creatures::ItemRecord record;
        std::memcpy(record.data(), bytes.data() + EQUIPMENT + ITEM_SIZE * slot, ITEM_SIZE);
        c.setEquipment(slot, record);
    }
    for (size_t i = 0; i < c.skills.size(); i++)
        c.skills[i] = get<int32_t>(bytes, 0x1128 + 4 * i);
    return c;
}

void CreatureRecord::setLinkedItem(const creatures::ItemRecord &item) {
    std::memcpy(bytes.data() + EQUIPMENT + ITEM_SIZE * 12, item.data(), ITEM_SIZE);
}

void CreatureRecord::setFighter(const creatures::Combatant &c) {
    put<uint16_t>(bytes, 0x114, uint16_t(c.mode));
    bytes[0x050] = c.controller;
    put<int32_t>(bytes, 0x054, c.type);
    bytes[0x058] = (uint8_t)c.action;
    put<int32_t>(bytes, 0x05c, c.actionMs);
    put<int32_t>(bytes, 0x060, c.combo);
    put<int32_t>(bytes, 0x064, c.comboElapsedMs);
    put<int32_t>(bytes, 0x118, (int32_t)c.dodgeMs);
    put<int32_t>(bytes, 0x11c, c.hitStun);
    put<int32_t>(bytes, 0x120, c.actionLockMs);
    put<int32_t>(bytes, 0x124, (int32_t)c.slowMs);
    put<int32_t>(bytes, 0x128, (int32_t)c.hasteMs);
    put<float>(bytes, 0x12c, c.recoveryPenalty);
    bytes[0x130] = c.characterClass;
    bytes[0x131] = c.specialization;
    put<float>(bytes, 0x134, c.charge);
    put<float>(bytes, 0x15c, c.health);
    put<float>(bytes, 0x160, c.mana);
    put<float>(bytes, 0x164, c.blockGauge);
    for (int i = 0; i < 5; i++)
        put<float>(bytes, 0x168 + 4 * i, c.multipliers[i]);
    put<int32_t>(bytes, 0x180, c.level);
    put<int32_t>(bytes, 0x184, c.experience);
    put<uint64_t>(bytes, 0x188, c.ownerId);
    put<uint64_t>(bytes, 0x190, c.healTarget);
    bytes[0x198] = c.powerTier;
    put<int32_t>(bytes, 0x1a0, c.backing.x);
    put<int32_t>(bytes, 0x1a4, c.backing.y);
    put<int32_t>(bytes, 0x1a8, c.backing.index);
    for (size_t axis = 0; axis < c.behaviorReference.size(); axis++)
        put<int64_t>(bytes, 0x1b0 + 8 * axis, c.behaviorReference[axis]);
    const auto held = c.heldRecord();
    std::memcpy(bytes.data() + 0x1d8, held.data(), ITEM_SIZE);
    for (size_t slot = 0; slot < c.equipment.size(); slot++) {
        uint8_t *r = &bytes[EQUIPMENT + ITEM_SIZE * slot];
        const auto record = c.equipmentRecord(slot);
        std::memcpy(r, record.data(), ITEM_SIZE);
    }
    for (size_t i = 0; i < c.skills.size(); i++)
        put<int32_t>(bytes, 0x1128 + 4 * i, c.skills[i]);
}

std::string CreatureRecord::name() const {
    const char *p = (const char *)&bytes[0x1158];
    return std::string(p, strnlen(p, 16));
}

void CreatureRecord::setName(const std::string &name) {
    std::memset(&bytes[0x1158], 0, 16);
    std::memcpy(&bytes[0x1158], name.data(), std::min<size_t>(name.size(), 15));
}

uint64_t changedFields(const CreatureRecord &now, const CreatureRecord *before) {
    if (!before)
        return ALL_FIELDS;
    uint64_t mask = 0;
    for (size_t n = 0; n < CREATURE_FIELDS.size(); n++) {
        const CreatureField &f = CREATURE_FIELDS[n];
        const uint8_t *a = &now.bytes[f.offset], *b = &before->bytes[f.offset];
        bool differs = false;
        switch (f.kind) {
        case CreatureField::BYTES:
            differs = std::memcmp(a, b, f.size) != 0;
            break;
        case CreatureField::FLOATS: // compared as floats: +0 equals -0, NaN always differs
            for (uint16_t i = 0; i < f.size && !differs; i += 4) {
                float x, y;
                std::memcpy(&x, a + i, 4);
                std::memcpy(&y, b + i, 4);
                differs = !(x == y);
            }
            break;
        case CreatureField::NAME:
            differs = std::strncmp((const char *)a, (const char *)b, f.size) != 0;
            break;
        }
        if (differs)
            mask |= 1ull << n;
    }
    return mask;
}

std::vector<uint8_t> encodeDelta(uint64_t id, const CreatureRecord &now, uint64_t mask) {
    ByteWriter w;
    w.u64(id);
    w.u64(mask);
    for (size_t n = 0; n < CREATURE_FIELDS.size(); n++)
        if (mask & (1ull << n))
            w.write(&now.bytes[CREATURE_FIELDS[n].offset], CREATURE_FIELDS[n].size);
    return std::move(w.data());
}

std::optional<uint64_t> applyDelta(const std::vector<uint8_t> &plain, CreatureRecord &record) {
    ByteReader r(plain);
    const uint64_t id = r.u64(), mask = r.u64();
    if (!r.ok() || (mask >> 48) != 0)
        return std::nullopt;
    CreatureRecord next = record;
    for (size_t n = 0; n < CREATURE_FIELDS.size(); n++)
        if (mask & (1ull << n))
            r.read(&next.bytes[CREATURE_FIELDS[n].offset], CREATURE_FIELDS[n].size);
    if (!r.atEnd())
        return std::nullopt;
    record = next;
    return id;
}

std::vector<uint8_t> compressed(const std::vector<uint8_t> &plain) {
    const std::vector<uint8_t> packed = zlibCompress(plain);
    ByteWriter w;
    w.u32((uint32_t)packed.size());
    w.bytes(packed);
    return std::move(w.data());
}

bool WorldUpdate::empty() const {
    for (int s = 0; s < 13; s++)
        if (s != 6 && s != 7 && !sections[s].empty())
            return false;
    return keyed[0].empty() && keyed[1].empty();
}

std::vector<uint8_t> WorldUpdate::encode() const {
    ByteWriter w;
    for (int s = 0; s < 13; s++) {
        if (s == 6 || s == 7) {
            const auto &lists = keyed[s - 6];
            w.u32((uint32_t)lists.size());
            for (const auto &list : lists) {
                w.u64(list.key);
                w.u32((uint32_t)list.records.size());
                for (const auto &rec : list.records)
                    w.bytes(rec);
            }
            continue;
        }
        w.u32((uint32_t)sections[s].size());
        for (const auto &rec : sections[s])
            w.bytes(rec);
    }
    return std::move(w.data());
}

std::optional<WorldUpdate> WorldUpdate::decode(const std::vector<uint8_t> &plain) {
    ByteReader r(plain);
    WorldUpdate u;
    for (int s = 0; s < 13; s++) {
        const uint32_t count = r.u32();
        if (s == 6 || s == 7) {
            for (uint32_t i = 0; i < count && r.ok(); i++) {
                KeyedList list;
                list.key = r.u64();
                const uint32_t n = r.u32();
                if ((size_t)n * RECORD_SIZE[s] > r.left())
                    return std::nullopt;
                for (uint32_t k = 0; k < n; k++)
                    list.records.push_back(r.bytes(RECORD_SIZE[s]));
                u.keyed[s - 6].push_back(std::move(list));
            }
            continue;
        }
        if ((size_t)count * RECORD_SIZE[s] > r.left())
            return std::nullopt;
        for (uint32_t i = 0; i < count; i++)
            u.sections[s].push_back(r.bytes(RECORD_SIZE[s]));
    }
    if (!r.atEnd())
        return std::nullopt;
    return u;
}

Hit Hit::make(uint64_t attacker, uint64_t target, float amount, int32_t stun, glm::vec3 knockBack, uint8_t kind) {
    Hit h;
    std::memcpy(&h.bytes[0], &attacker, 8);
    std::memcpy(&h.bytes[8], &target, 8);
    std::memcpy(&h.bytes[0x10], &amount, 4);
    std::memcpy(&h.bytes[0x18], &stun, 4);
    std::memcpy(&h.bytes[0x38], &knockBack, sizeof knockBack);
    h.bytes[0x45] = kind;
    return h;
}

uint64_t Hit::attacker() const {
    uint64_t v;
    std::memcpy(&v, &bytes[0], 8);
    return v;
}

uint64_t Hit::target() const {
    uint64_t v;
    std::memcpy(&v, &bytes[8], 8);
    return v;
}

int32_t Hit::stun() const {
    int32_t v;
    std::memcpy(&v, &bytes[0x18], 4);
    return v;
}

int32_t Hit::lockMs() const {
    int32_t v;
    std::memcpy(&v, &bytes[0x1c], sizeof(v));
    return v;
}

glm::vec3 Hit::knockBack() const {
    glm::vec3 v;
    std::memcpy(&v, &bytes[0x38], sizeof v);
    return v;
}

float Hit::amount() const {
    float v;
    std::memcpy(&v, &bytes[0x10], 4);
    return v;
}

std::vector<uint16_t> toUtf16(const std::string &text) {
    std::vector<uint16_t> out;
    for (size_t i = 0; i < text.size();) {
        const uint8_t c = (uint8_t)text[i];
        uint32_t cp = c;
        int extra = c >= 0xf0 ? 3 : c >= 0xe0 ? 2 : c >= 0xc0 ? 1 : 0;
        if (extra)
            cp = c & (0x3f >> extra);
        i++;
        for (int k = 0; k < extra && i < text.size(); k++, i++)
            cp = (cp << 6) | ((uint8_t)text[i] & 0x3f);
        if (cp >= 0x10000) {
            cp -= 0x10000;
            out.push_back((uint16_t)(0xd800 + (cp >> 10)));
            out.push_back((uint16_t)(0xdc00 + (cp & 0x3ff)));
        } else
            out.push_back((uint16_t)cp);
    }
    return out;
}

std::string fromUtf16(const std::vector<uint16_t> &units) {
    std::string out;
    for (size_t i = 0; i < units.size(); i++) {
        uint32_t cp = units[i];
        if (cp >= 0xd800 && cp < 0xdc00 && i + 1 < units.size())
            cp = 0x10000 + ((cp - 0xd800) << 10) + (units[++i] - 0xdc00);
        if (cp < 0x80)
            out += (char)cp;
        else if (cp < 0x800) {
            out += (char)(0xc0 | (cp >> 6));
            out += (char)(0x80 | (cp & 0x3f));
        } else if (cp < 0x10000) {
            out += (char)(0xe0 | (cp >> 12));
            out += (char)(0x80 | ((cp >> 6) & 0x3f));
            out += (char)(0x80 | (cp & 0x3f));
        } else {
            out += (char)(0xf0 | (cp >> 18));
            out += (char)(0x80 | ((cp >> 12) & 0x3f));
            out += (char)(0x80 | ((cp >> 6) & 0x3f));
            out += (char)(0x80 | (cp & 0x3f));
        }
    }
    return out;
}

} // namespace net
