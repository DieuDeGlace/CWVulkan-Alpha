// The dungeon builder (docs/specs/alpha-worldgen-structures.md §1): a 3 x 3 x 3 grid of rooms
// joined by corridors, carved into a 22 x 22 x 22 grid of 10-block cells, turned to fit the
// ground, then built out of blocks with its furniture, decorations, loot and creatures.
// Section numbers point at that spec; the loot comes from alpha-items-generator.md.
#include "worldgen/Builders.h"
#include "worldgen/Items.h"
#include "worldgen/Prefab.h"
#include "worldgen/Terrain.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>

namespace worldgen {
namespace {

constexpr int64_t ONE = 65536;
constexpr int64_t HALF = 32768;
constexpr int32_t N = 22; // cells per side
constexpr double PI = 3.141592653589793;


// §1.5: cell kinds and flags.
enum CellKind : uint8_t { OUTSIDE = 0, SCRATCH = 1, PILLAR = 2, OPEN = 3, ENTRANCE = 4 };
enum CellFlag : uint8_t { STAIR = 1, FLOOR_PATCH = 2, MARK = 4 };

// Three draws d1, d2, d3; the third lands in red (§0).
Colour drawColour(Random &random, int32_t modR, int32_t addR, int32_t modG, int32_t addG, int32_t modB, int32_t addB) {
    const int32_t d1 = random.draw(), d2 = random.draw(), d3 = random.draw();
    return {d3 % modR + addR, d2 % modG + addG, d1 % modB + addB};
}

// §1.10.2: a creature kind list split into leaders and escorts.
struct Group {
    std::vector<int32_t> leaders, escorts;
};

struct Room {
    bool present = false, marked = false, quiet = false, fixed = false;
    int32_t p[6] = {};
    glm::ivec3 P{0}, S{0}; // box, after the fixed override
};

struct Link {
    glm::ivec3 from, to;
};

// The cell grid with its view transform (0x5c38c0, 0x5fa3c0).
class Grid {
  public:
    int32_t rot = 0;
    bool mir = false;

    uint8_t kind(int32_t a, int32_t b, int32_t c) const { return inside(a, b, c) ? at(a, b, c)[0] : SCRATCH; }
    uint8_t flags(int32_t a, int32_t b, int32_t c) const { return inside(a, b, c) ? at(a, b, c)[1] : 0; }
    uint8_t kind(glm::ivec3 q) const { return kind(q.x, q.y, q.z); }
    void setKind(int32_t a, int32_t b, int32_t c, uint8_t k) {
        if (inside(a, b, c))
            at(a, b, c)[0] = k;
    }
    void addFlag(int32_t a, int32_t b, int32_t c, uint8_t flag) {
        if (inside(a, b, c))
            at(a, b, c)[1] |= flag;
    }
    // 0x5fa440: a stored cell's logical coordinates.
    glm::ivec3 logical(glm::ivec3 s) const {
        int32_t A = s.x, B = s.y;
        if (mir)
            B = N - 1 - B;
        switch (rot) {
        case 1: return {B, N - 1 - A, s.z};
        case 2: return {N - 1 - A, N - 1 - B, s.z};
        case 3: return {N - 1 - B, A, s.z};
        default: return {A, B, s.z};
        }
    }

  private:
    static bool inside(int32_t a, int32_t b, int32_t c) { return a >= 0 && b >= 0 && c >= 0 && a < N && b < N && c < N; }
    std::array<uint8_t, 2> &at(int32_t a, int32_t b, int32_t c) { return cells[index(a, b, c)]; }
    const std::array<uint8_t, 2> &at(int32_t a, int32_t b, int32_t c) const { return cells[index(a, b, c)]; }
    size_t index(int32_t a, int32_t b, int32_t c) const {
        int32_t A, B;
        switch (rot) {
        case 1: A = N - 1 - b, B = a; break;
        case 2: A = N - 1 - a, B = N - 1 - b; break;
        case 3: A = b, B = N - 1 - a; break;
        default: A = a, B = b; break;
        }
        if (mir)
            B = N - 1 - B;
        return ((size_t)c * N + B) * N + A;
    }
    std::vector<std::array<uint8_t, 2>> cells = std::vector<std::array<uint8_t, 2>>(N * N * N, {0, 0});
};

class Builder : StructureWriter {
  public:
    Builder(const Terrain &terrain, Zone &zone, Random &random, const DungeonCall &call,
            const std::function<Block(int32_t, int32_t, int32_t)> &gapFill)
        : StructureWriter{terrain, zone, random, gapFill}, x0(call.x0), y0(call.y0), v(call.variant), flag(call.flag),
          lv(call.level), sl(call.subLevel) {}

    void run() {
        styles();
        roomParameters();
        selectRooms();
        carveRooms();
        corridors();
        entranceAndPalette();
        if (v <= 3)
            raisedFloors();
        if (!fit())
            return;
        palette2();
        shell();
        if (v == 5)
            mound();
        interior();
        if (v == 4 || v == 5)
            shafts();
        if (v == 1 || v == 2)
            erosion();
        roomSpawns();
    }

  private:
    // ---- Items and creatures ------------------------------------------------------------

    // §1.10.2 item 6.
    int32_t rarity(int32_t r, bool force) {
        int32_t q = random.draw() % (r + 1);
        if (random.draw() % 100 == 0)
            q++;
        if (random.draw() % 1000 == 0)
            q++;
        if (random.draw() % 10000 == 0)
            q++;
        if (force)
            q = r + 1;
        return std::min(q, 4);
    }

    // §1.10.2 item 7: a bag of coins (2 draws).
    ItemRecord coins(uint8_t subtype, uint8_t material) {
        const int32_t r = random.draw();
        const float exponent = f(f(f(f((float)(r * 2) / 32767.f) + 1.f)) * f(lv * 0.25));
        const float F = f(std::pow(2.0, (double)exponent));
        ItemRecord item;
        item.type = 12;
        item.subtype = subtype;
        item.material = material;
        item.level = (int16_t)(random.draw() % (2 * (int32_t)F + 2));
        return item;
    }

    // §1.10.2 item 7: the ten inventory records of a guard (0x5c9f90, 0x5f8750).
    std::vector<ItemRecord> guardInventory() {
        std::vector<ItemRecord> list;
        for (int32_t n = 0; n < 10; n++) {
            ItemRecord item;
            if (random.draw() & 1) {
                item = coins(0, 10);
            } else {
                item.level = (int16_t)lv;
                if (random.draw() % 6 == 0) {
                    switch (random.draw() % 5) {
                    case 0: item.type = 1, item.subtype = 1; break;
                    case 1: item = coins(4, 12); break;
                    case 2: item = coins(0, 12); break;
                    case 3: item.type = 1, item.subtype = 7; break;
                    default: item.type = 1, item.subtype = 5; break;
                    }
                } else if (random.draw() & 1) {
                    item.type = 18;
                    item.subtype = 0;
                    item.seed = (uint32_t)(random.draw() % 3);
                    item.material = 0;
                }
            }
            list.push_back(item);
        }
        return list;
    }

    ItemRecord stack(uint8_t type, uint8_t subtype) const {
        ItemRecord item;
        item.type = type;
        item.subtype = subtype;
        item.level = (int16_t)lv;
        return item;
    }

    Spawn creatureAt(int64_t x, int64_t y, int64_t z) const {
        Spawn s;
        s.x = x;
        s.y = y;
        s.z = z;
        s.level = lv;
        return s;
    }

    // §1.10.9: the kind rule.
    void kindRule(Spawn &s, const std::vector<int32_t> &kinds) {
        if (flag) {
            s.controller = 3;
            s.type = random.draw() & 1;
        } else {
            s.controller = 1;
            s.appearanceFlags |= 0x1000;
            s.type = kinds[(uint32_t)random.draw() % kinds.size()];
            s.powerTier = (uint8_t)sl;
        }
    }

    // ---- §1.2 – §1.9: the layout ---------------------------------------------------------

    void styles() {
        switch (v) {
        case 1:
        case 2: {
            K1 = {15, 16};
            K2 = {{{15, 16}, {96}}};
            const int32_t d = random.draw() % 3;
            K2.push_back({{d == 1 ? 94 : d == 2 ? 17 : 97}, {}});
            break;
        }
        case 3:
            K1 = {2, 3};
            K2 = {{{2, 3}, {19}}};
            break;
        case 5:
            K1 = {78, 77};
            K2 = {{{17, 81}, {62, 30}}};
            break;
        default:
            K1 = {11, 12};
            K2 = {{{46}, {19}}};
            break;
        }
    }

    Room &room(int32_t a, int32_t b, int32_t c) { return rooms[9 * a + 3 * b + c]; }
    Room &room(glm::ivec3 q) { return room(q.x, q.y, q.z); }

    void roomParameters() {
        for (int32_t a = 0; a < 3; a++)
            for (int32_t b = 0; b < 3; b++)
                for (int32_t c = 0; c < 3; c++) {
                    Room &r = room(a, b, c);
                    r.p[5] = random.draw() % 3;
                    r.p[4] = random.draw() % 3 - 1;
                    r.p[3] = random.draw() % 3 - 1;
                    r.p[2] = random.draw() % 3 - 1;
                    r.p[1] = random.draw() & 1;
                    r.p[0] = random.draw() & 1;
                }
    }

    void present(int32_t index) { rooms[index].present = true; }
    void presentIfEven(int32_t index) { rooms[index].present = (random.draw() & 1) == 0; }
    void links(std::initializer_list<std::array<int32_t, 6>> list) {
        for (const auto &l : list)
            linkList.push_back({{l[0], l[1], l[2]}, {l[3], l[4], l[5]}});
    }

    // §1.4.
    void selectRooms() {
        if (flag) {
            for (int32_t i : {13, 16, 4, 22, 1, 19})
                present(i);
            links({{1, 1, 1, 1, 2, 1}, {1, 1, 1, 0, 1, 1}, {1, 1, 1, 2, 1, 1}, {0, 1, 1, 0, 0, 1}, {2, 1, 1, 2, 0, 1}});
            return;
        }
        if (v == 2 || v == 4 || v == 5) {
            random.draw(); // (its value is not used)
            if (v == 2) {
                present(11);
            } else {
                present(14);
                rooms[14].quiet = rooms[14].fixed = true;
            }
            for (int32_t i : {16, 25, 22, 19, 7})
                present(i);
            presentIfEven(4);
            present(0);
            present(3);
            presentIfEven(6);
            present(12);
            rooms[12].marked = true;
            if (v == 2)
                links({{1, 0, 2, 1, 2, 1}});
            else
                links({{1, 1, 2, 1, 2, 1}});
            links({{1, 2, 1, 0, 2, 1}, {1, 2, 1, 2, 2, 1}, {0, 2, 1, 0, 1, 1}, {2, 2, 1, 2, 1, 1}, {2, 1, 1, 2, 0, 1},
                   {2, 0, 1, 0, 0, 0}, {0, 0, 0, 0, 1, 0}, {0, 1, 0, 0, 2, 0}, {0, 1, 0, 1, 1, 0}});
            return;
        }
        // v = 0, 1, 3 (and any other).
        const int32_t s = random.draw() % 3;
        present(10);
        if (s == 1) {
            presentIfEven(19);
            present(13);
            present(4);
            presentIfEven(1);
            present(7);
            present(16);
            present(25);
            rooms[25].marked = true;
            presentIfEven(22);
            links({{1, 0, 1, 1, 1, 1}, {1, 0, 1, 2, 0, 1}, {1, 1, 1, 0, 1, 1}, {0, 1, 1, 0, 0, 1}, {0, 1, 1, 0, 2, 1},
                   {0, 2, 1, 1, 2, 1}, {1, 2, 1, 2, 2, 1}, {2, 2, 1, 2, 1, 1}});
        } else if (s == 2) {
            presentIfEven(19);
            present(1);
            present(4);
            presentIfEven(7);
            present(13);
            present(22);
            presentIfEven(25);
            present(16);
            rooms[16].marked = true;
            links({{1, 0, 1, 0, 0, 1}, {1, 0, 1, 2, 0, 1}, {2, 0, 1, 2, 1, 1}, {0, 0, 1, 0, 1, 1}, {0, 1, 1, 1, 1, 1},
                   {0, 1, 1, 0, 2, 1}, {1, 1, 1, 1, 2, 1}, {1, 1, 1, 2, 1, 1}, {2, 1, 1, 2, 2, 1}});
        } else {
            presentIfEven(19);
            present(1);
            present(4);
            presentIfEven(7);
            present(13);
            presentIfEven(16);
            present(22);
            present(25);
            rooms[25].marked = true;
            links({{1, 0, 1, 0, 0, 1}, {1, 0, 1, 2, 0, 1}, {0, 0, 1, 0, 1, 1}, {0, 1, 1, 1, 1, 1}, {0, 1, 1, 0, 2, 1},
                   {1, 1, 1, 1, 2, 1}, {1, 1, 1, 2, 1, 1}, {2, 1, 1, 2, 2, 1}});
        }
    }

    static glm::ivec3 centre(const Room &r) { return r.P + r.S / 2; }

    // §1.5.
    void carveRooms() {
        for (int32_t a = 0; a < 3; a++)
            for (int32_t b = 0; b < 3; b++)
                for (int32_t c = 0; c < 3; c++) {
                    Room &r = room(a, b, c);
                    if (r.fixed) {
                        r.p[0] = r.p[1] = r.p[2] = 0;
                        r.p[3] = r.p[4] = 0;
                        r.p[5] = 1;
                    }
                    r.P = {7 * a + 2 + r.p[0], 7 * b + 2 + r.p[1], 7 * c + 3 + r.p[2]};
                    r.S = {r.p[3] + 3, r.p[4] + 3, r.p[5] + 1};
                    if (!r.present)
                        continue;
                    const bool pit = !r.marked && !r.fixed && random.draw() % 3 == 0;
                    buildRoom(r.P, r.S, pit);
                    if (r.marked) {
                        const glm::ivec3 m = centre(r);
                        grid.addFlag(m.x, m.y, m.z, MARK);
                    }
                }
    }

    // 0x5ef8f0.
    void buildRoom(glm::ivec3 P, glm::ivec3 S, bool pit) {
        for (int32_t a = P.x; a < P.x + S.x; a++)
            for (int32_t b = P.y; b < P.y + S.y; b++)
                for (int32_t c = P.z; c < P.z + S.z; c++)
                    grid.setKind(a, b, c, OPEN);
        if (random.draw() & 1) {
            const int32_t w = random.draw() % S.x + 1;
            const int32_t h = random.draw() % S.y + 1;
            int32_t u = P.x, t = P.y;
            if (S.x - w > 0)
                u += random.draw() % (S.x - w + 1);
            if (S.y - h > 0)
                t += random.draw() % (S.y - h + 1);
            for (int32_t a = u; a < u + w; a++)
                for (int32_t b = t; b < t + h; b++)
                    grid.addFlag(a, b, P.z, FLOOR_PATCH);
        }
        if (pit && S.x > 2 && S.y > 2) {
            const int32_t w = random.draw() % (S.x - 2) + 1;
            const int32_t h = random.draw() % (S.y - 2) + 1;
            int32_t u = P.x + 1, t = P.y + 1;
            if (S.x - w > 1)
                u += random.draw() % (S.x - w - 1);
            if (S.y - h > 1)
                t += random.draw() % (S.y - h - 1);
            for (int32_t a = u; a < u + w; a++)
                for (int32_t b = t; b < t + h; b++)
                    for (int32_t c = P.z; c < P.z + S.z; c++)
                        grid.setKind(a, b, c, PILLAR);
        }
    }

    // 0x5ef6d0.
    void line(glm::ivec3 F, glm::ivec3 T) {
        const glm::ivec3 D = T - F;
        const int32_t n = std::max({std::abs(D.x), std::abs(D.y), std::abs(D.z)});
        if (n <= 0)
            return;
        int32_t previousZ = F.z;
        for (int32_t t = 0; t <= n; t++) {
            const glm::ivec3 q = F + D * t / n;
            grid.setKind(q.x, q.y, q.z, OPEN);
            // On a step the lower of the two cells holds the stairs, the upper is open.
            if (q.z < previousZ) {
                grid.setKind(q.x, q.y, q.z + 1, OPEN);
                grid.addFlag(q.x, q.y, q.z, STAIR);
            } else if (q.z > previousZ) {
                grid.setKind(q.x, q.y, q.z - 1, OPEN);
                grid.addFlag(q.x, q.y, q.z - 1, STAIR);
            }
            previousZ = q.z;
        }
    }

    // §1.6.
    void corridors() {
        for (const Link &l : linkList) {
            const Room &A = room(l.from), &B = room(l.to);
            if (!A.present || !B.present)
                continue;
            // The same rule along y (axis 1) or along x (axis 0): `u` is the axis the corridor
            // crosses, `w` the one it runs along.
            auto corridor = [&](int32_t u, int32_t w) {
                const int32_t m = std::clamp(B.S[u] / 2 + B.P[u], A.P[u], A.P[u] + A.S[u] - 1);
                const bool lower = B.P[w] < A.P[w];
                glm::ivec3 F, T;
                F[u] = T[u] = m;
                F.z = A.P.z, T.z = B.P.z;
                F[w] = lower ? A.P[w] : A.P[w] + A.S[w] - 1; // the last cell inside A
                T[w] = lower ? B.P[w] + B.S[w] : B.P[w] - 1; // the first outside B
                line(F, T);
                if ((random.draw() & 1) && A.P[u] < m && B.P[u] < m) {
                    glm::ivec3 F2 = F, T2 = T;
                    F2[u] = T2[u] = m - 1;
                    line(F2, T2);
                }
                // (As read: along x the third line's test mixes in x extents, §2.3.)
                const bool third = m < B.P.x + B.S.x - 1 && (u == 1 && lower ? m < A.P.y + A.S.y - 1 : m < A.P.x + A.S.x - 1);
                if ((random.draw() & 1) && third) {
                    glm::ivec3 F3 = F, T3 = T;
                    F3[u] = T3[u] = m + 1;
                    line(F3, T3);
                }
            };
            if (B.P.x < A.P.x + A.S.x && A.P.x < B.P.x + B.S.x)
                corridor(0, 1);
            else if (B.P.y < A.P.y + A.S.y && A.P.y < B.P.y + B.S.y)
                corridor(1, 0);
        }
    }

    // §1.7.
    void entranceAndPalette() {
        // The entrance: one cell outside the first room's low-y wall, at its floor and low-x
        // corner.
        const Room &first = room(linkList.front().from);
        grid.setKind(first.P.x, first.P.y - 1, first.P.z, ENTRANCE);
        grid.rot = random.draw() & 3;
        grid.mir = (random.draw() & 1) == 0;
        c1 = drawColour(random, 50, 20, 50, 20, 50, 20);
        int32_t e2 = random.draw() % 50;
        if (v == 3)
            e2 = random.draw() % 50 + 100;
        c2 = drawColour(random, 50, 50 + e2, 50, 50 + e2, 50, 50 + e2);
        c3 = drawColour(random, 50, 50 + e2, 50, 50 + e2, 50, 50 + e2);
        if (v == 1 || v == 2) {
            styleFlag = 1;
            lampTint = {0, 0.5f, 0.1f};
            c2 = drawColour(random, 20, 50, 50, 50, 50, 50);
            c3 = drawColour(random, 20, 50, 50, 50, 50, 50);
            glow = 0.0015f;
        } else if (v == 4) {
            c2 = drawColour(random, 50, 100, 50, 80, 50, 20);
            c3 = drawColour(random, 50, 100, 50, 80, 50, 20);
            glow = 0.0025f;
        } else if (v == 5) {
            c2 = {255, 210, 130};
            c3 = {200, 180, 80};
        }
        if (terrain.prefabs)
            arch = terrain.prefabs(v == 4 || v == 5 ? "temple-arc.cub" : "castle-arc.cub");
    }

    // §1.8.
    void raisedFloors() {
        for (int32_t i = 0; i < N; i++)
            for (int32_t j = 0; j < N; j++) {
                int32_t w = 1;
                if (v != 2 && random.draw() % 6 == 0)
                    w = random.draw() % 3 + 2;
                for (int32_t c = N - 1; c >= 0; c--) {
                    const uint8_t k = grid.kind(i, j, c);
                    if (k == ENTRANCE || k == OUTSIDE)
                        continue;
                    for (int32_t z = c + 1; z <= std::min(c + w, N - 1); z++)
                        grid.setKind(i, j, z, PILLAR);
                    break;
                }
            }
    }

    glm::ivec3 findEntrance() const {
        glm::ivec3 found{0};
        for (int32_t c = N - 1; c >= 0; c--)
            for (int32_t a = 0; a < N; a++)
                for (int32_t b = 0; b < N; b++)
                    if (grid.kind(a, b, c) == ENTRANCE)
                        found = {a, b, c};
        return found;
    }

    // §1.9: turns the grid four times and keeps the turn whose entrance meets the ground best.
    bool fit() {
        int32_t bestScore = 0, bestRot = grid.rot;
        bool bestWet = false, first = true;
        for (int32_t k = 0; k < 4; k++) {
            grid.rot = (grid.rot + 1) & 3;
            const glm::ivec3 e = findEntrance();
            const int32_t X = x0 + 10 * e.x, Y = y0 + 10 * e.y;
            // The ground under the middle of the entrance cell.
            if (!zone.contains(X + 5, Y + 5))
                return false;
            int32_t g0 = zone.column(X + 5, Y + 5).top();
            while (read(X + 5, Y + 5, g0).type() == AIR)
                g0--;
            g0++;
            bool wet = false;
            for (int32_t i = 0; i < 10 && !wet; i++)
                for (int32_t j = 0; j < 10 && !wet; j++)
                    for (int32_t z = 0; z < 10 && !wet; z++) {
                        const uint8_t t = read(X + i, Y + j, g0 - 5 + z).type();
                        wet = t == WATER || t == LAKE_BED;
                    }
            const int32_t score = g0 + (wet ? 100 : 0) - 10 * e.z;
            if (first || score < bestScore) {
                bestScore = score, bestRot = grid.rot, bestWet = wet;
                first = false;
            }
        }
        grid.rot = bestRot;
        entrance = findEntrance();
        base = bestScore;
        if (v == 4 || v == 5)
            base += 50;
        if (bestWet)
            base -= 100;
        if (base + 10 * entrance.z < 0)
            base = -10 * entrance.z;
        return true;
    }

    // ---- §1.10: the building pass ---------------------------------------------------------

    int32_t cellX(int32_t a) const { return x0 + 10 * a; }
    int32_t cellY(int32_t b) const { return y0 + 10 * b; }
    int32_t cellZ(int32_t c) const { return base + 10 * c; }
    static bool closed(uint8_t k) { return k == OUTSIDE || k == SCRATCH || k == ENTRANCE; }
    static bool solidCell(uint8_t k) { return k == OUTSIDE || k == SCRATCH || k == PILLAR; }

    // §1.10.1.
    void palette2() {
        c4 = drawColour(random, 180, 50, 180, 50, 180, 50);
        c5 = drawColour(random, 180, 50, 180, 50, 180, 50);
        if (v == 0 || v == 3)
            random.draw(); // (its value is not used)
    }

    int32_t lowestBottom(int32_t X, int32_t Y) const {
        int32_t bmin = zone.column(X, Y).bottom;
        for (int32_t x = X - 2; x <= X + 11; x++)
            for (int32_t y = Y - 2; y <= Y + 11; y++)
                if (zone.contains(x, y))
                    bmin = std::min(bmin, zone.column(x, y).bottom);
        return bmin;
    }

    // §1.10.3: the shell, parapets, roofs and foundations, column by column.
    void shell() {
        for (int32_t a = 0; a < N; a++)
            for (int32_t b = 0; b < N; b++) {
                const int32_t X = cellX(a), Y = cellY(b);
                int32_t m = INT32_MAX;
                for (int32_t i = 0; i < 10; i++)
                    for (int32_t j = 0; j < 10; j++)
                        if (zone.contains(X + i, Y + j))
                            m = std::min(m, zone.column(X + i, Y + j).top());
                bool found = false, ent = false;
                int32_t top = 0;
                for (int32_t c = N - 1; c >= 0; c--) {
                    const uint8_t k = grid.kind(a, b, c);
                    if (k == OUTSIDE)
                        continue;
                    const int32_t Z = cellZ(c);
                    if (!found)
                        top = base + 10 * (c + 1) - (k == ENTRANCE ? 10 : 0);
                    if (k == ENTRANCE) {
                        ent = found = true;
                        continue;
                    }
                    if (!found && Z + 5 > m)
                        roof(a, b, c);
                    box(X - 2, Y - 2, Z - 2, 14, 14, 14, c2, glow, 0);
                    found = true;
                }
                if (!(found || v == 4 || v == 5) || !zone.contains(X, Y))
                    continue;
                const int32_t bmin = lowestBottom(X, Y);
                if (v <= 3)
                    box(X - 2, Y - 2, bmin, 14, 14, top - bmin, c2, glow, ent ? KEEP : 0);
                else if (v == 4)
                    pyramidStep(a, b, bmin);
            }
    }

    // §1.10.3 step 2.3: what tops the highest cell of a column when it rises above the ground.
    void roof(int32_t a, int32_t b, int32_t c) {
        const int32_t X = cellX(a), Y = cellY(b), Z = cellZ(c);
        if (v == 2) {
            box(X, Y, Z + 11, 10, 10, 3, c2, 0, 0);
            return;
        }
        if (v != 0 && v != 1 && v != 3)
            return;
        auto walled = [&](int32_t da, int32_t db) {
            return closed(grid.kind(a + da, b + db, c)) && closed(grid.kind(a + da, b + db, c + 1));
        };
        const int32_t Zr = Z + 10;
        // A crenel: air where the parapet top is open, then one merlon block beside it.
        auto crenel = [&](int32_t ax, int32_t ay, int32_t mx, int32_t my) {
            if (const uint8_t t = read(ax, ay, Zr + 3).type(); t == AIR || t == WATER)
                write(ax, ay, Zr + 3, {0, 0, 0, DRY_AIR});
            box(mx, my, Zr + 3, 1, 1, 1, c2, 0, 0);
        };
        if (walled(-1, 0)) {
            box(X - 3, Y - 3, Zr, 2, 16, 3, c2, 0, 0);
            for (int32_t t = 0; t < 8; t++) {
                const int32_t q = Y - 2 + 2 * t;
                crenel(X - 3, q, X - 3, q - 1);
            }
        }
        if (walled(1, 0)) {
            box(X + 11, Y - 3, Zr, 2, 16, 3, c2, 0, 0);
            for (int32_t t = 0; t < 8; t++) {
                const int32_t q = Y - 1 + 2 * t;
                crenel(X + 12, q, X + 12, q - 1);
            }
        }
        if (walled(0, -1)) {
            box(X - 3, Y - 3, Zr, 16, 2, 3, c2, 0, 0);
            for (int32_t t = 0; t < 8; t++) {
                const int32_t q = X - 2 + 2 * t;
                crenel(q, Y - 3, q - 1, Y - 3);
            }
        }
        if (walled(0, 1)) {
            box(X - 3, Y + 11, Zr, 16, 2, 3, c2, 0, 0);
            for (int32_t t = 0; t < 8; t++) {
                const int32_t q = X - 1 + 2 * t;
                crenel(q, Y + 12, q - 1, Y + 12);
            }
        }
        if (v == 3 && closed(grid.kind(a - 1, b, c)) && closed(grid.kind(a + 1, b, c)) &&
            closed(grid.kind(a, b - 1, c)) && closed(grid.kind(a, b + 1, c))) {
            box(X - 2, Y - 2, Z + 10, 14, 14, 4, c1, 0, 0);
            for (int32_t t = 0; t < 7; t++)
                box(X - 2 + t, Y - 2 + t, Z + 14 + 2 * t, 14 - 2 * t, 14 - 2 * t, 2, c1, 0, 0);
        }
    }

    // §1.10.4: one column of the temple pyramid.
    void pyramidStep(int32_t a, int32_t b, int32_t bmin) {
        const int32_t X = cellX(a), Y = cellY(b);
        const int32_t m = std::max(std::abs(a - 10), std::abs(b - 10));
        int32_t e = 10 - m;
        if (a == 10 && b == 10)
            e--;
        int32_t H = base + 10 * (entrance.z - 5 + e);
        if (std::abs(a - 10) <= 1 && std::abs(b - 10) <= 1)
            H += 2;
        box(X - 2, Y - 2, bmin, 14, 14, H - bmin - 4, c2, glow, 0);
        if (H - bmin > 0) {
            box(X - 1, Y - 1, H - 4, 12, 12, 3, c2, 0, 0);
            box(X, Y, H - 1, 10, 10, 3, c2, 0, 0);
            for (int32_t i = -2; i <= 11; i++)
                for (int32_t j = -2; j <= 11; j++) {
                    int32_t z = H + 2;
                    if (i == -2 || i == 11 || j == -2 || j == 11)
                        z -= 6;
                    else if (i == -1 || i == 10 || j == -1 || j == 10)
                        z -= 3;
                    const int32_t x = X + i, y = Y + j;
                    if (noise(0.02 * x, 0.02 * y) <= 0 || !zone.contains(x, y))
                        continue;
                    if (const uint8_t t = read(x, y, z).type(); t == AIR || t == WATER)
                        write(x, y, z, terrain.surface(x, y, z, &zone));
                }
        }
        // Stairs up the middle of each face, runs of 11 boxes (open points 10 §2).
        if (a == 10 && std::abs(b - 10) > 1) {
            auto row = [&](int32_t k) { return b < 10 ? Y - 1 + k : Y + 10 - k; };
            for (int32_t k = 0; k <= 10; k++)
                box(X - 2, row(k), H, 14, 1, k + 2, c2, glow, 0);
            for (int32_t k = 0; k <= 10; k++)
                box(X - 2, row(k), H, 2, 1, k + 4, c2, glow, 0);
            for (int32_t k = 0; k <= 10; k++)
                box(X + 10, row(k), H, 2, 1, k + 4, c2, glow, 0);
        } else if (b == 10 && std::abs(a - 10) > 1) {
            auto row = [&](int32_t k) { return a < 10 ? X - 1 + k : X + 10 - k; };
            for (int32_t k = 0; k <= 10; k++)
                box(row(k), Y - 2, H, 1, 14, k + 2, c2, glow, 0);
            for (int32_t k = 0; k <= 10; k++)
                box(row(k), Y - 2, H, 1, 2, k + 4, c2, glow, 0);
            for (int32_t k = 0; k <= 10; k++)
                box(row(k), Y + 10, H, 1, 2, k + 4, c2, glow, 0);
        }
    }

    // §1.10.5: the v = 5 mound, a square pyramid of slope 1.
    void mound() {
        const int32_t peak = base + 10 * entrance.z + 59;
        for (int32_t x = x0; x < x0 + 220; x++)
            for (int32_t y = y0; y < y0 + 220; y++) {
                if (!zone.contains(x, y))
                    continue;
                const int32_t bot = zone.column(x, y).bottom;
                const int32_t d = std::max(std::abs(y - (y0 + 105)), std::abs(x - (x0 + 105)));
                box(x, y, bot, 1, 1, peak - d - bot, c2, glow, 0);
            }
    }

    // §1.10.2 item 5.
    Sprite torch(int32_t style, int64_t x, int64_t y, int64_t z, float angle) {
        Sprite s;
        s.x = x, s.y = y, s.z = z;
        s.angle = angle;
        auto lamp = [&] {
            s.kind = 54;
            s.flags = 1;
            s.tint[0] = 0.8f, s.tint[1] = 0.7f, s.tint[2] = 0.2f;
        };
        switch (style) {
        case 0:
            if (random.draw() & 1) {
                lamp();
            } else {
                s.kind = 50;
                s.z += 2 * ONE;
            }
            break;
        case 1:
        case 2:
            switch (random.draw() % 3) {
            case 0: s.kind = 50; break;
            case 1: s.kind = 49; break;
            default:
                s.kind = 52;
                s.flags = 1;
                s.tint[0] = 0, s.tint[1] = 0.5f, s.tint[2] = 0.1f;
                break;
            }
            break;
        case 3: s.kind = 48; break;
        case 4:
            if (random.draw() & 1)
                s.kind = 44 + (random.draw() & 3);
            else
                lamp();
            break;
        default: lamp(); break;
        }
        return s;
    }

    // §1.10.2 item 4: a chest, shelf, table or stool against the wall on side s.
    StaticObject furniture(int64_t x, int64_t y, int64_t z, int32_t s, int32_t style) {
        StaticObject o;
        o.x = x, o.y = y, o.z = z;
        auto away = [&](int64_t by) {
            switch (s) {
            case 0: o.y -= by; break;
            case 1: o.x += by; break;
            case 2: o.y += by; break;
            default: o.x -= by; break;
            }
        };
        auto scale = [&](float sx, float sy, float sz) { o.scale[0] = sx, o.scale[1] = sy, o.scale[2] = sz; };
        if (random.draw() % 50 == 0) {
            o.kind = 10;
            scale(1.5f, 1, 1);
            o.rotation = (s + 2) & 3;
            away(HALF);
            return o;
        }
        o.rotation = s;
        switch (random.draw() % 3) {
        case 0: {
            const int32_t d = random.draw() % 3;
            o.kind = (style == 3 ? 32 : style == 4 ? 38 : style == 5 ? 41 : 35) + d;
            scale(2, 1, 1.57f);
            break;
        }
        case 1:
            o.kind = style == 4 ? 13 : style == 5 ? 14 : 12;
            scale(3, 3, 1);
            away((int64_t)f(f((float)random.draw() / 32767.f + 1.f) * 65536.f));
            break;
        default:
            o.kind = style == 4 ? 15 : style == 5 ? 17 : 16;
            scale(1, 1, 0.5f);
            away((int64_t)f(f((float)random.draw() / 32767.f) * 65536.f));
            break;
        }
        return o;
    }

    // §1.10.6: everything inside the open cells.
    void interior() {
        for (int32_t a = 0; a < N; a++)
            for (int32_t b = 0; b < N; b++)
                for (int32_t c = 0; c < N; c++) {
                    const uint8_t k = grid.kind(a, b, c);
                    if (k == OPEN || k == ENTRANCE)
                        cell(a, b, c, k);
                }
    }

    void cell(int32_t a, int32_t b, int32_t c, uint8_t k) {
        const int32_t X = cellX(a), Y = cellY(b), Z = cellZ(c);
        const uint8_t flags = grid.flags(a, b, c);
        const bool stair = flags & STAIR;

        if (k == ENTRANCE)
            zone.markers.push_back({5, (int64_t)(X + 5) * ONE, (int64_t)(Y + 5) * ONE, (int64_t)Z * ONE});
        carve(X, Y, Z, 10, 10, 10, v == 1 || v == 2 || v == 4);

        if ((v == 0 || v == 1 || v == 3) && k != ENTRANCE && !stair)
            windows(a, b, c);
        if (stair)
            stairs(a, b, c);
        if (v == 4 && k == ENTRANCE)
            return;

        bool decorate = grid.kind(a, b, c - 1) != OPEN; // otherwise straight to the arches
        if (decorate) {
            box(X, Y, Z, 10, 10, 1, c3, glow, 0);
            if ((v == 0 || v == 3) && (flags & FLOOR_PATCH))
                carpet(a, b, c);
            if (k == ENTRANCE) {
                boss(a, b, c);
                return;
            }
            if (!stair)
                furnish(a, b, c);
        }
        archesAndChandelier(a, b, c, k);
        boss(a, b, c);
    }

    // §1.10.6 step 3.
    void windows(int32_t a, int32_t b, int32_t c) {
        const int32_t X = cellX(a), Y = cellY(b), Z = cellZ(c);
        struct Side {
            int32_t da, db;
            glm::ivec2 test, corner, at;
            int32_t rotation;
        };
        const Side sides[4] = {
            {0, -1, {X + 5, Y - 3}, {X + 4, Y - 2}, {X + 5, Y - 1}, 0},
            {0, 1, {X + 5, Y + 13}, {X + 4, Y + 10}, {X + 5, Y + 11}, 2},
            {-1, 0, {X - 3, Y + 5}, {X - 2, Y + 4}, {X - 1, Y + 5}, 3},
            {1, 0, {X + 13, Y + 5}, {X + 10, Y + 4}, {X + 11, Y + 5}, 1},
        };
        for (const Side &s : sides) {
            if (!(random.draw() & 1) || grid.kind(a + s.da, b + s.db, c) != OUTSIDE || blockSolid(s.test.x, s.test.y, Z + 2))
                continue;
            carve(s.corner.x, s.corner.y, Z + 2, 2, 2, 4, false);
            StaticObject w;
            w.kind = 4;
            w.x = (int64_t)s.at.x * ONE, w.y = (int64_t)s.at.y * ONE, w.z = (int64_t)(Z + 2) * ONE;
            w.rotation = s.rotation;
            w.scale[0] = 2, w.scale[1] = 0.2f, w.scale[2] = 4;
            zone.statics.push_back(w);
        }
    }

    // §1.10.6 step 4.
    void stairs(int32_t a, int32_t b, int32_t c) {
        const int32_t X = cellX(a), Y = cellY(b), Z = cellZ(c);
        auto K = [&](int32_t da, int32_t db, int32_t dc) { return grid.kind(a + da, b + db, c + dc); };
        if (K(1, 0, 0) != OPEN && K(-1, 0, 0) == OPEN && K(1, 0, 1) == OPEN)
            for (int32_t t = 0; t <= 10; t++)
                box(X - 1 + t, Y, Z - 1 + t, 11 - t, 10, 1, c3, glow, 0);
        if (K(-1, 0, 0) != OPEN && K(1, 0, 0) == OPEN && K(-1, 0, 1) == OPEN)
            for (int32_t t = 0; t <= 10; t++)
                box(X, Y, Z - 1 + t, 11 - t, 10, 1, c3, glow, 0);
        if (K(0, 1, 0) != OPEN && K(0, -1, 0) == OPEN && K(0, 1, 1) == OPEN)
            for (int32_t t = 0; t <= 10; t++)
                box(X, Y - 1 + t, Z - 1 + t, 10, 11 - t, 1, c3, glow, 0);
        if (K(0, -1, 0) != OPEN && K(0, 1, 0) == OPEN && K(0, -1, 1) == OPEN)
            for (int32_t t = 0; t <= 10; t++)
                box(X, Y, Z - 1 + t, 10, 11 - t, 1, c3, glow, 0);
    }

    // §1.10.6 step 7.2: a rug over the floor patch, with a border where the patch ends.
    void carpet(int32_t a, int32_t b, int32_t c) {
        const int32_t X = cellX(a), Y = cellY(b), Z = cellZ(c);
        int32_t x1 = X, x2 = X + 10, y1 = Y, y2 = Y + 10;
        const bool bx1 = !(grid.flags(a - 1, b, c) & FLOOR_PATCH), bx2 = !(grid.flags(a + 1, b, c) & FLOOR_PATCH);
        const bool by1 = !(grid.flags(a, b - 1, c) & FLOOR_PATCH), by2 = !(grid.flags(a, b + 1, c) & FLOOR_PATCH);
        if (bx1)
            x1 += 2;
        if (bx2)
            x2 -= 2;
        if (by1)
            y1 += 2;
        if (by2)
            y2 -= 2;
        fill(x1, y1, Z, x2 - x1, y2 - y1, 1, c4);
        if (bx1)
            fill(x1, y1, Z, 1, y2 - y1, 1, c5);
        if (bx2)
            fill(x2 - 1, y1, Z, 1, y2 - y1, 1, c5);
        if (by1)
            fill(x1, y1, Z, x2 - x1, 1, 1, c5);
        if (by2)
            fill(x1, y2 - 1, Z, x2 - x1, 1, 1, c5);
    }

    // §1.10.6 steps 7.4 – 7.8: trap, ceiling and wall decorations, loot.
    void furnish(int32_t a, int32_t b, int32_t c) {
        const int32_t X = cellX(a), Y = cellY(b), Z = cellZ(c);
        if (flag == 0 && random.draw() % 40 == 0) {
            StaticObject trap;
            trap.kind = 7;
            trap.x = (int64_t)(X + 5) * ONE, trap.y = (int64_t)(Y + 5) * ONE, trap.z = (int64_t)(Z + 1) * ONE;
            trap.scale[0] = trap.scale[1] = trap.scale[2] = 10;
            trap.rotation = random.draw() & 3;
            trap.timer = random.draw() % 4000;
            zone.statics.push_back(trap);
            return;
        }
        const size_t n0 = zone.statics.size();

        if (solidCell(grid.kind(a, b, c + 1))) {
            if (random.draw() % 3 == 0 && (v == 2 || v == 4)) {
                Sprite s;
                s.kind = 55;
                s.angle = f(f((float)random.draw() / 32767.f) * 360.f);
                s.size = f(f(f((float)random.draw() * 0.04f) / 32767.f) + 0.08f);
                const float z = f((float)(Z + 10) - 20.f * s.size);
                const int32_t y = Y + 2 + random.draw() % 5;
                const int32_t x = X + 2 + random.draw() % 5;
                s.x = (int64_t)x * ONE, s.y = (int64_t)y * ONE, s.z = fx(z);
                zone.sprites.push_back(s);
            }
            if (v == 1 || v == 2) {
                const int32_t order[4][2] = {{-1, 0}, {1, 0}, {0, 1}, {0, -1}};
                const float angles[4] = {0, 180, 270, 90};
                for (int32_t side = 0; side < 4; side++) {
                    if (random.draw() % 3 != 0 || !solidCell(grid.kind(a + order[side][0], b + order[side][1], c)))
                        continue;
                    Sprite s;
                    s.angle = angles[side];
                    s.size = f(f(f((float)random.draw() * 0.05f) / 32767.f) + 0.08f);
                    const float z = f((float)(Z + 10) - 20.f * s.size);
                    const float q = (float)(2 + random.draw() % 5);
                    float px, py;
                    switch (side) {
                    case 0: px = f((float)X + 10.f * s.size), py = f((float)Y + q); break;
                    case 1: px = f((float)(X + 10) - 10.f * s.size), py = f((float)Y + q); break;
                    case 2: px = f((float)X + q), py = f((float)(Y + 10) - 10.f * s.size); break;
                    default: px = f((float)X + q), py = f((float)Y + 10.f * s.size); break;
                    }
                    s.x = fx(px), s.y = fx(py), s.z = fx(z);
                    s.kind = 57 + (random.draw() & 1);
                    zone.sprites.push_back(s);
                }
            }
        }

        // Wall decorations: torches, furniture and guards, sides -x, +x, -y, +y.
        struct Side {
            int32_t da, db;
            float torchAngle;
            int32_t facing; // the furniture's side s
        };
        const Side sides[4] = {{-1, 0, 270, 1}, {1, 0, 90, 3}, {0, -1, 0, 2}, {0, 1, 180, 0}};
        for (const Side &s : sides) {
            if (random.draw() % 3 != 0 || !solidCell(grid.kind(a + s.da, b + s.db, c)))
                continue;
            // A spot along the wall `r` (0..3) blocks past the third; `inset` is how far in
            // from the wall, in 1/65536 blocks.
            auto spot = [&](int32_t r, int64_t inset, int32_t up) {
                glm::i64vec3 p;
                const int64_t along = (int64_t)(3 + r) * ONE;
                if (s.da)
                    p = {s.da < 0 ? (int64_t)X * ONE + inset : (int64_t)(X + 10) * ONE - inset, (int64_t)Y * ONE + along,
                         (int64_t)(Z + up) * ONE};
                else
                    p = {(int64_t)X * ONE + along, s.db < 0 ? (int64_t)Y * ONE + inset : (int64_t)(Y + 10) * ONE - inset,
                         (int64_t)(Z + up) * ONE};
                return p;
            };
            if (random.draw() & 1) {
                const glm::i64vec3 p = spot(random.draw() & 3, HALF, 2);
                const int32_t wx = blockOfFixed(p.x + s.da * ONE), wy = blockOfFixed(p.y + s.db * ONE);
                                if (blockSolid(wx, wy, blockOfFixed((int64_t)(Z + 2) * ONE)))
                    zone.sprites.push_back(torch(v, p.x, p.y, p.z, s.torchAngle));
                if (s.db > 0) // (open points 8: on the +y wall a torch's chance rules out furniture)
                    continue;
            }
            if (random.draw() & 7) {
                const glm::i64vec3 p = spot(random.draw() & 3, HALF, 1);
                zone.statics.push_back(furniture(p.x, p.y, p.z, s.facing, v));
            } else {
                const glm::i64vec3 p = spot(random.draw() & 3, ONE, 1);
                Spawn guard = creatureAt(p.x, p.y, p.z);
                guard.controller = 6;
                guard.type = 143;
                guard.multipliers[0] = 10;
                guard.radius = 150;
                guard.inventory = guardInventory();
                zone.spawns.push_back(guard);
            }
        }

        // Loot on what was just placed.
        for (size_t n = n0; n < zone.statics.size(); n++) {
            StaticObject &o = zone.statics[n];
            if (o.kind == 10) {
                const int32_t count = (random.draw() & 3) + 1;
                for (int32_t i = 0; i < count; i++)
                    o.contents.push_back(lootItem(random, lv, sl + 1));
                o.state = 2;
            } else if (o.kind == 12 || o.kind == 13 || o.kind == 35 || o.kind == 36 || o.kind == 37) {
                tableLoot(o);
            }
        }
    }


    // §1.10.6 step 7.8: one chance in ten per block of the top of a table or shelf.
    void tableLoot(const StaticObject &o) {
        for (int32_t u = 0; (float)u < o.scale[0]; u++)
            for (int32_t w = 0; (float)w < o.scale[1]; w++) {
                if (random.draw() % 10 != 0)
                    continue;
                ItemDrop drop;
                if (random.draw() % 6 != 0) {
                    switch (random.draw() & 3) {
                    case 0: drop.item.type = 11, drop.item.subtype = 26; break;
                    case 1:
                        drop.item.type = 18, drop.item.subtype = (uint8_t)styleFlag;
                        drop.item.seed = (uint32_t)(random.draw() % 3);
                        break;
                    case 2:
                        drop.item.type = 11, drop.item.subtype = 9;
                        drop.item.material = (uint8_t)(random.draw() % 3 + 25);
                        break;
                    default: drop.item.type = 1, drop.item.subtype = 7; break;
                    }
                } else {
                    switch (random.draw() % 6) {
                    case 0: drop.item.type = 1, drop.item.subtype = 1; break;
                    case 1: drop.item.type = 1, drop.item.subtype = 4; break;
                    case 2: drop.item.type = 1, drop.item.subtype = 5; break;
                    case 3: drop.item = coins(0, 10); break;
                    case 4: drop.item = lootItem(random, lv, sl); break;
                    default: drop.item = recipeItem(random, lv, sl); break;
                    }
                }
                drop.x = o.x + fx(f(f((float)u - f(o.scale[0] * 0.5f)) + 0.5f));
                drop.y = o.y + fx(f(f((float)w - f(o.scale[1] * 0.5f)) + 0.5f));
                drop.z = o.z + fx(o.scale[2]);
                drop.rotation = f((float)random.draw() * 360.f / 32767.f);
                zone.drops.push_back(drop);
            }
    }

    // §1.10.6 step 8.
    void archesAndChandelier(int32_t a, int32_t b, int32_t c, uint8_t k) {
        if (k == ENTRANCE || grid.kind(a, b, c + 1) == OPEN)
            return;
        const int32_t X = cellX(a), Y = cellY(b), Z = cellZ(c);
        if (arch && blockSolid(X, Y, Z + 10)) {
            const int32_t sides[4][3] = {{-1, 0, 0}, {1, 0, 2}, {0, -1, 3}, {0, 1, 1}};
            for (const auto &s : sides) {
                if (grid.kind(a + s[0], b + s[1], c) == OPEN)
                    continue;
                Placement at;
                at.x = X, at.y = Y, at.z = Z;
                at.rotation = s[2];
                at.typeByte = DRY_AIR | ROCK;
                at.mode = 0;
                placePrefab(zone, random, placerWorld(), *arch, at);
            }
        }
        if (v == 3 && random.draw() % 10 == 0) {
            Sprite s;
            s.kind = 56;
            s.x = fx((float)X + 5.5f), s.y = fx((float)Y + 5.5f);
            s.z = (int64_t)(((double)(Z + 10) - 2.7) * 65536.0); // (a double, 0x459c00)
            s.size = 0.1f;
            s.angle = 0;
            s.flags = 1;
            s.tint[0] = lampTint[0], s.tint[1] = lampTint[1], s.tint[2] = lampTint[2];
            zone.sprites.push_back(s);
        }
    }

    PlacerWorld placerWorld() const {
        PlacerWorld p;
        p.warmth = [this](int32_t x, int32_t y) { return terrain.world().warmth(x, y); };
        p.level = [this](int32_t, int32_t) { return lv; };
        p.gapFill = gapFill;
        return p;
    }

    // §1.10.6 step 9: the boss on a marked cell.
    void boss(int32_t a, int32_t b, int32_t c) {
        if (!(grid.flags(a, b, c) & MARK))
            return;
        Spawn s = creatureAt(fx((float)cellX(a) + 4.5f), fx((float)cellY(b) + 4.5f), (int64_t)(cellZ(c) + 1) * ONE);
        s.controller = 1;
        s.appearanceFlags |= 0x1000;
        s.level = std::max(lv, 1);
        s.powerTier = (uint8_t)sl;
        s.type = K1[(uint32_t)random.draw() % K1.size()];
        s.inventory.push_back(lootItem(random, s.level, rarity(sl, true)));
        s.appearanceFlags |= 0x200;
        zone.markers.push_back({6, s.x, s.y, s.z, s.type, (int32_t)zone.spawns.size()});
        const int32_t potions = random.draw() & 3;
        for (int32_t i = 0; i < potions; i++)
            s.inventory.push_back(stack(1, 1));
        s.radius = 150;
        s.bossLoot = true;
        zone.spawns.push_back(s);
    }

    // §1.10.7: the central shaft of the temple and the mound.
    void shafts() {
        const int32_t Xc = x0 + 100, Yc = y0 + 100;
        const int32_t Ze = base + 10 * entrance.z + 8; // (open points 10 §11)
        int32_t Zt;
        if (v == 4) {
            Zt = base + 10 * entrance.z + 44;
            box(Xc - 2, Yc - 2, Zt, 14, 14, 16, c2, glow, 0);
            carve(Xc, Yc - 2, Zt, 10, 14, 10, false);
            carve(Xc - 2, Yc, Zt, 14, 10, 10, false);
            carve(Xc, Yc, Ze, 10, 10, Zt - Ze, false);
        } else {
            Zt = base + 10 * entrance.z + 38;
            box(Xc - 2, Yc - 2, Zt, 14, 14, 14, c2, glow, 0);
            // The corridors, shaft and pillar stand 12 lower than the box (open points 10 §3).
            Zt -= 12;
            carve(Xc + 2, Yc - 32, Zt, 6, 74, 6, false);
            carve(Xc - 32, Yc + 2, Zt, 74, 6, 6, false);
            carve(Xc, Yc, Ze, 10, 10, Zt - Ze + 6, false);
        }
        const int32_t n = Zt - Ze + 30;
        box(Xc + 4, Yc + 4, Ze - 30, 2, 2, n, c2, glow, 0);
        if (n <= 1)
            return;
        // A spiral stair round the pillar, one step per block of height.
        for (int32_t z = Ze - 29; z - Ze + 30 < n; z++) {
            switch ((z - Ze + 30) & 7) {
            case 0: box(Xc, Yc, z, 5, 5, 1, c2, glow, 0); break;
            case 1: box(Xc + 5, Yc, z, 1, 5, 1, c2, glow, 0); break;
            case 2: box(Xc + 6, Yc, z, 5, 5, 1, c2, glow, 0); break;
            case 3: box(Xc + 6, Yc + 5, z, 5, 1, 1, c2, glow, 0); break;
            case 4: box(Xc + 6, Yc + 6, z, 5, 5, 1, c2, glow, 0); break;
            case 5: box(Xc + 5, Yc + 6, z, 1, 5, 1, c2, glow, 0); break;
            case 6: box(Xc, Yc + 6, z, 5, 5, 1, c2, glow, 0); break;
            default: box(Xc, Yc + 5, z, 5, 1, 1, c2, glow, 0); break;
            }
        }
    }

    // §1.10.8: wears the tops of the v = 1, 2 ruins down with noise.
    void erosion() {
        for (int32_t a = 0; a < N; a++)
            for (int32_t b = 0; b < N; b++)
                for (int32_t x = cellX(a); x < cellX(a) + 10; x++)
                    for (int32_t y = cellY(b); y < cellY(b) + 10; y++) {
                        if (!zone.contains(x, y))
                            continue;
                        float e = f(f(noise(0.1 * x, 0.1 * y) * 2.f) + f(noise(0.05 * x, 0.05 * y) * 8.f));
                        if (e > 0)
                            e = 0;
                        const int32_t top = zone.column(x, y).top();
                        const int32_t h = (int32_t)f((float)top + e);
                        for (int32_t z = top; z >= h; z--) {
                            const uint8_t t = read(x, y, z).typeByte;
                            if ((t & DRY_AIR) && !(t & KEEP))
                                write(x, y, z, {255, 255, 255, AIR});
                        }
                    }
    }

    // §1.10.9: the creatures of each room.
    void roomSpawns() {
        for (int32_t a = 0; a < 3; a++)
            for (int32_t b = 0; b < 3; b++)
                for (int32_t c = 0; c < 3; c++) {
                    const Room &r = room(a, b, c);
                    if (r.present && !r.quiet)
                        roomCreatures(r);
                }
    }

    glm::i64vec3 standOn(glm::ivec3 Q) const {
        return {(int64_t)(x0 + 10 * Q.x + 5) * ONE, (int64_t)(y0 + 10 * Q.y + 5) * ONE, (int64_t)(base + 10 * Q.z + 1) * ONE};
    }

    void ring(glm::ivec3 Q) {
        const int32_t m = (random.draw() & 1) + 2;
        for (int32_t j = 0; j < m; j++) {
            const float theta = f((double)(2 * j) * PI / (double)m);
            const float dx = f(2.0 * (double)f(std::cos((double)theta)));
            const float dy = f(2.0 * (double)f(std::sin((double)theta)));
            Spawn s = creatureAt(fx(f((float)(x0 + 10 * Q.x + 5) + dx)), fx(f((float)(y0 + 10 * Q.y + 5) + dy)),
                                 (int64_t)(base + 10 * Q.z + 1) * ONE);
            s.yaw = f((double)theta / PI * 180.0 + 90.0); // (open points 10 §8)
            s.multipliers[2] *= 0.75f;
            kindRule(s, K1);
            const int32_t potions = random.draw() & 1;
            for (int32_t i = 0; i < potions; i++)
                s.inventory.push_back(stack(1, 1));
            zone.spawns.push_back(s);
        }
    }

    void roomCreatures(const Room &r) {
        const glm::ivec3 P = r.P, S = r.S;
        const glm::ivec3 Q[4] = {grid.logical(P), grid.logical({P.x + S.x - 1, P.y, P.z}),
                                 grid.logical({P.x + S.x - 1, P.y + S.y - 1, P.z}), grid.logical({P.x, P.y + S.y - 1, P.z})};
        // 1. A leader with its escorts.
        if (S.x * S.y > 6 && !K2.empty()) {
            const Group &G = K2[(uint32_t)random.draw() % K2.size()];
            if (!G.leaders.empty()) {
                const glm::i64vec3 p = standOn(Q[random.draw() & 3]);
                Spawn leader = creatureAt(p.x, p.y, p.z);
                kindRule(leader, G.leaders);
                random.draw(); // the patrol route (d % 3) over the room's corners
                leader.radius = 150;
                const int32_t potions = random.draw() & 1;
                for (int32_t i = 0; i < potions; i++)
                    leader.inventory.push_back(stack(1, 1));
                zone.spawns.push_back(leader);
                if (flag == 0 && !G.escorts.empty()) {
                    const int32_t n = random.draw() % 3 + 1;
                    for (int32_t i = 0; i < n; i++) {
                        Spawn escort = creatureAt(leader.x, leader.y, leader.z);
                        escort.controller = 1;
                        escort.appearanceFlags |= 0x1000;
                        escort.type = G.escorts[(uint32_t)random.draw() % G.escorts.size()];
                        escort.powerTier = leader.powerTier;
                        escort.multipliers[2] *= 0.5f;
                        zone.spawns.push_back(escort);
                    }
                }
            }
        }
        if (K1.empty() || r.marked)
            return;
        // 2. Rings of creatures at the corners.
        int32_t G = (random.draw() & 1) + 1;
        const int32_t k = random.draw() & 3;
        if (S.x * S.y > 7)
            G++;
        for (int32_t g = 0; g < G; g++)
            ring(Q[(k + g) & 3]);
        // 3. One more ring in the middle.
        if (S.x > 2 && S.y > 2 && (random.draw() & 1)) {
            const glm::ivec3 mid = (Q[0] + Q[2]) / 2;
            if (grid.kind(mid) == OPEN)
                ring(mid);
        }
    }

    const int32_t x0, y0, v, flag, lv, sl;

    std::vector<int32_t> K1;
    std::vector<Group> K2;
    Room rooms[27];
    std::vector<Link> linkList;
    Grid grid;
    Colour c1, c2, c3, c4, c5;
    float glow = 0.005f;
    std::array<float, 3> lampTint{0.6f, 0.5f, 0.1f};
    int32_t styleFlag = 0;
    std::shared_ptr<const Prefab> arch;
    glm::ivec3 entrance{0};
    int32_t base = 0;
};

} // namespace

void Terrain::dungeon(Zone &zone, Random &random, const DungeonCall &call) const {
    Builder(*this, zone, random, call, [&](int32_t x, int32_t y, int32_t z) { return rockBlock(x, y, z, zone); }).run();
}

} // namespace worldgen
