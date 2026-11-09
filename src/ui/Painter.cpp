#include "ui/Painter.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <map>

namespace ui {
namespace {

constexpr float PI = 3.14159265358979f;

glm::vec2 apply(const Transform &t, glm::vec2 p) { return glm::vec2(t * glm::vec3(p, 1)); }

// A vertex of an outline loop with everything the subdivision carries along (§4.2).
struct LoopVertex {
    glm::vec2 position{0}, uv{0};
    glm::vec4 colour{1}, strokeColour{1};
    float strokeWidth = 0;
    uint32_t type = 0; // corner type, §4.2
};

LoopVertex mix(const LoopVertex &a, const LoopVertex &b, float t) {
    LoopVertex m;
    m.position = a.position + (b.position - a.position) * t;
    m.uv = a.uv + (b.uv - a.uv) * t;
    m.colour = a.colour + (b.colour - a.colour) * t;
    m.strokeColour = a.strokeColour + (b.strokeColour - a.strokeColour) * t;
    m.strokeWidth = a.strokeWidth + (b.strokeWidth - a.strokeWidth) * t;
    return m;
}

// §4.2: one refinement pass of a closed loop.
std::vector<LoopVertex> refine(const std::vector<LoopVertex> &loop, float w, bool first) {
    const size_t n = loop.size();
    if (n < 3)
        return loop;
    auto straight = [&](uint32_t type) { return type == 2 || (first && type == 3); };
    // 1. Midpoints.
    std::vector<LoopVertex> mids(n);
    for (size_t i = 0; i < n; i++) {
        const LoopVertex &a = loop[i], &b = loop[(i + 1) % n];
        constexpr float c = 0.414214f; // 1 / (1 + 2 cos 45°)
        if (first && (a.type == 3) != (b.type == 3))
            mids[i] = a.type == 3 ? mix(a, b, 1 - c) : mix(a, b, c);
        else
            mids[i] = mix(a, b, 0.5f);
        mids[i].type = straight(a.type) || straight(b.type) ? 2 : 0;
    }
    // 2. Old vertices move towards their new neighbours.
    std::vector<LoopVertex> moved(loop);
    for (size_t i = 0; i < n; i++) {
        const LoopVertex &before = mids[(i + n - 1) % n], &after = mids[i];
        const LoopVertex half = mix(before, after, 0.5f);
        LoopVertex &v = moved[i];
        const uint32_t type = loop[i].type;
        if (type == 2 || (type == 3 && first)) {
            const uint32_t keep = v.type;
            v = half;
            v.type = keep;
        } else if (type == 0) {
            const uint32_t keep = v.type;
            v = mix(v, half, w);
            v.type = keep;
        }
    }
    // 3. Straight edges stay straight.
    for (size_t i = 0; i < n; i++) {
        const LoopVertex &a = moved[i], &b = moved[(i + 1) % n];
        if (straight(a.type) && straight(b.type)) {
            mids[i] = mix(a, b, 0.5f);
            mids[i].type = 1;
        }
    }
    std::vector<LoopVertex> out;
    out.reserve(2 * n);
    for (size_t i = 0; i < n; i++) {
        out.push_back(moved[i]);
        out.push_back(mids[i]);
    }
    return out;
}

// Weighted sum of vertices (weights add up to 1); the corner type is set by the caller.
LoopVertex blend(std::initializer_list<std::pair<const LoopVertex *, float>> parts) {
    LoopVertex out;
    out.position = glm::vec2(0);
    out.uv = glm::vec2(0);
    out.colour = out.strokeColour = glm::vec4(0);
    for (const auto &[v, k] : parts) {
        out.position += v->position * k;
        out.uv += v->uv * k;
        out.colour += v->colour * k;
        out.strokeColour += v->strokeColour * k;
        out.strokeWidth += v->strokeWidth * k;
    }
    return out;
}

// A polygon mesh: shared vertices and faces as index loops.
struct PolyMesh {
    std::vector<LoopVertex> vertices;
    std::vector<std::vector<int>> faces;
};

// One refinement of a multi-face mesh. The library's own interior rule is not traced (P-8);
// this uses Catmull-Clark inside, and on the outline the curve rule of §4.2 so outlines match
// the single-face shapes: every face becomes quads through its centre.
PolyMesh refineMesh(const PolyMesh &in, float w, bool first) {
    PolyMesh out;
    out.vertices = in.vertices;
    const size_t nv = in.vertices.size();
    auto straight = [&](uint32_t type) { return type == 2 || (first && type == 3); };

    // Face points.
    std::vector<int> facePoint(in.faces.size());
    for (size_t f = 0; f < in.faces.size(); f++) {
        LoopVertex c = blend({});
        const float k = 1.f / (float)in.faces[f].size();
        for (int i : in.faces[f]) {
            const LoopVertex part = blend({{&in.vertices[i], k}});
            c.position += part.position;
            c.uv += part.uv;
            c.colour += part.colour;
            c.strokeColour += part.strokeColour;
            c.strokeWidth += part.strokeWidth;
        }
        facePoint[f] = (int)out.vertices.size();
        out.vertices.push_back(c);
    }
    // Edges and the faces using them.
    struct Edge {
        std::vector<int> faces;
        int point = -1;
    };
    std::map<std::pair<int, int>, Edge> edges;
    auto key = [](int a, int b) { return std::make_pair(std::min(a, b), std::max(a, b)); };
    for (size_t f = 0; f < in.faces.size(); f++)
        for (size_t i = 0; i < in.faces[f].size(); i++)
            edges[key(in.faces[f][i], in.faces[f][(i + 1) % in.faces[f].size()])].faces.push_back((int)f);
    // Edge points.
    for (auto &[ends, e] : edges) {
        const LoopVertex &a = in.vertices[ends.first], &b = in.vertices[ends.second];
        LoopVertex m;
        if (e.faces.size() == 1) {
            constexpr float c = 0.414214f;
            if (first && (a.type == 3) != (b.type == 3))
                m = a.type == 3 ? mix(a, b, 1 - c) : mix(a, b, c);
            else
                m = mix(a, b, 0.5f);
            m.type = straight(a.type) && straight(b.type) ? 1 : (straight(a.type) || straight(b.type) ? 2 : 0);
        } else {
            const LoopVertex &f0 = out.vertices[facePoint[e.faces[0]]], &f1 = out.vertices[facePoint[e.faces[1]]];
            m = blend({{&a, .25f}, {&b, .25f}, {&f0, .25f}, {&f1, .25f}});
            m.type = 0;
        }
        e.point = (int)out.vertices.size();
        out.vertices.push_back(m);
    }
    // Old vertices.
    std::vector<std::vector<int>> boundaryNeighbours(nv), faceNeighbours(nv), edgeNeighbours(nv);
    for (const auto &[ends, e] : edges) {
        for (int v : {ends.first, ends.second}) {
            edgeNeighbours[v].push_back(v == ends.first ? ends.second : ends.first);
            if (e.faces.size() == 1)
                boundaryNeighbours[v].push_back(e.point);
        }
    }
    for (size_t f = 0; f < in.faces.size(); f++)
        for (int i : in.faces[f])
            faceNeighbours[i].push_back(facePoint[f]);
    for (size_t v = 0; v < nv; v++) {
        const LoopVertex &old = in.vertices[v];
        LoopVertex &moved = out.vertices[v];
        const uint32_t type = old.type;
        if (type == 1 || (type == 3 && !first))
            continue;
        if (boundaryNeighbours[v].size() == 2) {
            const LoopVertex half = mix(out.vertices[boundaryNeighbours[v][0]], out.vertices[boundaryNeighbours[v][1]], .5f);
            moved = type == 0 ? mix(old, half, w) : half;
            moved.type = type;
        } else if (boundaryNeighbours[v].empty() && !faceNeighbours[v].empty()) {
            const float n = (float)faceNeighbours[v].size();
            LoopVertex F = blend({}), R = blend({});
            for (int f : faceNeighbours[v])
                F = blend({{&F, 1.f}, {&out.vertices[f], 1 / n}});
            for (int u : edgeNeighbours[v]) {
                const LoopVertex mid = mix(old, in.vertices[u], .5f);
                R = blend({{&R, 1.f}, {&mid, 1 / (float)edgeNeighbours[v].size()}});
            }
            moved = blend({{&F, 1 / n}, {&R, 2 / n}, {&old, (n - 3) / n}});
            moved.type = type;
        }
    }
    // Quads.
    for (size_t f = 0; f < in.faces.size(); f++) {
        const auto &face = in.faces[f];
        const size_t k = face.size();
        for (size_t i = 0; i < k; i++) {
            const int prev = face[(i + k - 1) % k], here = face[i], next = face[(i + 1) % k];
            out.faces.push_back({here, edges[key(here, next)].point, facePoint[f], edges[key(prev, here)].point});
        }
    }
    return out;
}

float signedArea(const std::vector<glm::vec2> &p) {
    float a = 0;
    for (size_t i = 0, j = p.size() - 1; i < p.size(); j = i++)
        a += (p[j].x - p[i].x) * (p[j].y + p[i].y);
    return a * 0.5f;
}

bool inside(glm::vec2 p, glm::vec2 a, glm::vec2 b, glm::vec2 c) {
    auto side = [](glm::vec2 u, glm::vec2 v, glm::vec2 q) { return (v.x - u.x) * (q.y - u.y) - (v.y - u.y) * (q.x - u.x); };
    const float d1 = side(a, b, p), d2 = side(b, c, p), d3 = side(c, a, p);
    const bool negative = d1 < 0 || d2 < 0 || d3 < 0, positive = d1 > 0 || d2 > 0 || d3 > 0;
    return !(negative && positive);
}

// Ear clipping of a simple polygon; returns index triples. Refined outlines carry repeated
// and collinear points and touch themselves at their seams, so: corners that turn by nothing
// are dropped, points lying on a vertex of the candidate ear do not block it, and when no ear
// is clean the sharpest convex corner is clipped anyway rather than giving up.
std::vector<int> triangulate(const std::vector<glm::vec2> &p) {
    std::vector<int> out, left;
    if (p.size() < 3)
        return out;
    for (size_t i = 0; i < p.size(); i++)
        if (left.empty() || p[i] != p[left.back()])
            left.push_back((int)i);
    while (left.size() > 1 && p[left.front()] == p[left.back()])
        left.pop_back();
    const float orientation = signedArea(p) > 0 ? 1.f : -1.f;
    auto turn = [&](size_t i) {
        const int a = left[(i + left.size() - 1) % left.size()], b = left[i], c = left[(i + 1) % left.size()];
        const glm::vec2 u = p[b] - p[a], v = p[c] - p[b];
        return (u.x * v.y - u.y * v.x) * -orientation; // > 0: convex
    };
    while (left.size() > 3) {
        size_t ear = left.size(), fallback = left.size();
        float sharpest = 0;
        for (size_t i = 0; i < left.size() && ear == left.size(); i++) {
            const float t = turn(i);
            if (t == 0) {
                ear = i; // no turn: drop the point, the triangle has no area
                break;
            }
            if (t < 0)
                continue; // a reflex corner
            const int a = left[(i + left.size() - 1) % left.size()], b = left[i], c = left[(i + 1) % left.size()];
            bool empty = true;
            for (int k : left)
                if (k != a && k != b && k != c && p[k] != p[a] && p[k] != p[b] && p[k] != p[c] &&
                    inside(p[k], p[a], p[b], p[c])) {
                    empty = false;
                    break;
                }
            if (empty)
                ear = i;
            else if (t > sharpest)
                sharpest = t, fallback = i;
        }
        if (ear == left.size())
            ear = fallback;
        if (ear == left.size())
            ear = 0; // every corner reflex: the outline is not simple; keep going
        const int a = left[(ear + left.size() - 1) % left.size()], b = left[ear], c = left[(ear + 1) % left.size()];
        if (turn(ear) != 0)
            out.insert(out.end(), {a, b, c});
        left.erase(left.begin() + (ptrdiff_t)ear);
    }
    if (left.size() == 3)
        out.insert(out.end(), {left[0], left[1], left[2]});
    return out;
}

char32_t next(const std::u16string &s, size_t &i) {
    char32_t c = s[i++];
    if (c >= 0xd800 && c < 0xdc00 && i < s.size())
        c = 0x10000 + ((c - 0xd800) << 10) + (s[i++] - 0xdc00);
    return c;
}

} // namespace

bool Fonts::load(const std::filesystem::path &install, std::string *error) {
    return main.load(install / "resource1.dat", error) && banner.load(install / "resource2.dat", error);
}

Transform translate(glm::vec2 t) {
    Transform m(1);
    m[2] = glm::vec3(t, 1);
    return m;
}

// §4.1, as the shipped files need it: x' = t + p + R(θ) · D · (x − p), so the pivot is the
// centre of rotation and deformation and the translation moves it (open question P-7; with
// x' = t + R·D·(x − p) the groups of start.plx land thousands of pixels away).
Transform Painter::nodeTransform(const plasma::Document &doc, const plasma::Node &n) {
    if (n.transformation < 0)
        return Transform(1);
    const plasma::Transformation &tr = doc.transformations[n.transformation];
    const float angle = tr.rotation.value.z * PI / 180.f;
    Transform R(1);
    R[0] = {std::cos(angle), std::sin(angle), 0};
    R[1] = {-std::sin(angle), std::cos(angle), 0};
    const glm::mat4 &d = tr.deformation.value;
    Transform D(1);
    D[0] = {d[0][0], d[0][1], 0};
    D[1] = {d[1][0], d[1][1], 0};
    return translate(tr.translation.value + tr.pivot.value) * R * D * translate(-tr.pivot.value);
}

void Painter::cut(Batch::Op op, uint8_t level) {
    Batch &last = batches.back();
    last.count = (uint32_t)triangles.size() - last.first;
    if (last.count == 0)
        batches.pop_back();
    batches.push_back({op, level, (uint32_t)triangles.size(), 0});
}

Frame Painter::take() {
    cut(Batch::Draw, maskLevel);
    batches.pop_back(); // the empty one cut() opened
    Frame frame{std::move(triangles), std::move(batches), std::move(items)};
    triangles.clear();
    items.clear();
    batches = {Batch{Batch::Draw, maskLevel, 0, 0}};
    return frame;
}

void Painter::item(const ItemDraw &draw) {
    if (draw.mesh == 0)
        return;
    cut(Batch::Item, maskLevel);
    batches.back().first = (uint32_t)items.size();
    batches.back().count = 1;
    batches.push_back({Batch::Draw, maskLevel, (uint32_t)triangles.size(), 0});
    items.push_back(draw);
}

ItemDraw itemDraw(uint32_t mesh, glm::vec3 size, bool ring, glm::vec2 at, glm::vec3 turn, float scale, float depth,
                  glm::vec2 window) {
    // Rows of Alpha's row-vector matrix (v' = x·r0 + y·r1 + z·r2 + t), turned in place.
    glm::vec3 r0(1, 0, 0), r1(0, 1, 0), r2(0, 0, 1);
    const auto aboutX = [&](float degrees) {
        const float c = std::cos(glm::radians(degrees)), s = std::sin(glm::radians(degrees));
        const glm::vec3 a = r1 * c + r2 * s, b = r2 * c - r1 * s;
        r1 = a, r2 = b;
    };
    const auto aboutY = [&](float degrees) {
        const float c = std::cos(glm::radians(degrees)), s = std::sin(glm::radians(degrees));
        const glm::vec3 a = r2 * c + r0 * s, b = r0 * c - r2 * s;
        r2 = a, r0 = b;
    };
    const auto aboutZ = [&](float degrees) {
        const float c = std::cos(glm::radians(degrees)), s = std::sin(glm::radians(degrees));
        const glm::vec3 a = r0 * c + r1 * s, b = r1 * c - r0 * s;
        r0 = a, r1 = b;
    };
    aboutX(turn.x);
    aboutY(turn.y);
    aboutZ(turn.z);
    if (ring) {
        aboutX(90);
        aboutZ(-90);
    }
    const float largest = std::max({size.x, size.y, size.z, 1.f});
    r0 *= scale / largest, r1 *= scale / largest, r2 *= scale / largest;
    // Centred: the mesh is already centred on x and y.
    const glm::vec3 t = glm::vec3(0, 0, 1 + depth) - size.z * 0.5f * r2;
    ItemDraw d;
    d.mesh = mesh;
    d.camera = glm::mat4(glm::vec4(r0, 0), glm::vec4(r1, 0), glm::vec4(r2, 0), glm::vec4(t, 1));
    // Field of view pi/4 with aspect -W/H (mirrored left-right); Vulkan's y points down.
    const float cot = 1.f / std::tan(glm::pi<float>() / 8);
    d.lens = {-cot * window.y / window.x, -cot, 2 * (at.x - window.x / 2) / window.x,
              2 * (at.y - window.y / 2) / window.y};
    return d;
}

ItemDraw modelDraw(uint32_t mesh, const glm::mat4 &model, glm::vec2 at, glm::vec3 turn, float distance, glm::vec2 window) {
    // The view as itemDraw builds it, unscaled, then the model's own placement.
    ItemDraw d = itemDraw(mesh, glm::vec3(1), false, at, turn, 1, 0, window);
    glm::mat4 view = d.camera;
    view[3] = glm::vec4(0, 0, distance, 1);
    d.camera = view * model;
    return d;
}

void Painter::draw(const plasma::Document &doc, int32_t node, const Transform &parent, bool isFileRoot) {
    drawNode(doc, node, parent, glm::vec4(1), glm::vec4(1), isFileRoot, 0);
}

void Painter::drawNode(const plasma::Document &doc, int32_t index, const Transform &parent, glm::vec4 fill,
                       glm::vec4 stroke, bool isFileRoot, int depth) {
    if (index < 0 || index >= (int32_t)doc.nodes.size() || depth > 64)
        return;
    const plasma::Node &n = doc.nodes[index];
    const size_t first = triangles.size();
    Transform t = parent;
    bool mask = false, clips = false, childrenDrawn = false;
    if (!isFileRoot) {
        if (n.display >= 0) {
            const plasma::Display &d = doc.displays[n.display];
            if (d.visibility.value == 0)
                return;
            fill *= d.fillColor.value;
            stroke *= d.strokeColor.value;
            mask = d.clipping.value == 1;
            // Which filter flags 4 selects is open (P-5); clipped through its shape the start
            // logo loses letters, so only flags 2 masks clip and the others draw unclipped.
            clips = mask && (d.flags & 2);
        }
        t = parent * nodeTransform(doc, n);
        if (n.shape >= 0 && clips) {
            // §5.3: the shape is not shown; the subtree is drawn through it.
            const plasma::Shape &s = doc.shapes[n.shape];
            if (s.kind == plasma::Shape::Kind::Mesh && maskLevel < 255) {
                cut(Batch::MaskIn, maskLevel);
                const uint32_t shapeFirst = (uint32_t)triangles.size();
                mesh(doc, s.mesh, t, glm::vec4(1), glm::vec4(1));
                const uint32_t shapeCount = (uint32_t)triangles.size() - shapeFirst;
                maskLevel++;
                cut(Batch::Draw, maskLevel);
                for (int32_t child : n.children)
                    drawNode(doc, child, t, fill, stroke, false, depth + 1);
                maskLevel--;
                cut(Batch::MaskOut, maskLevel);
                // The same shape again, taken away: no new triangles, the batch points back.
                batches.back().first = shapeFirst;
                batches.back().count = shapeCount;
                batches.push_back({Batch::Draw, maskLevel, (uint32_t)triangles.size(), 0});
                childrenDrawn = true;
            }
        }
        if (n.shape >= 0 && !mask) {
            const plasma::Shape &s = doc.shapes[n.shape];
            if (s.kind == plasma::Shape::Kind::Mesh) {
                mesh(doc, s.mesh, t, fill, stroke);
            } else if (s.kind == plasma::Shape::Kind::Text) {
                const plasma::TextShape &ts = s.text;
                TextStyle style;
                style.pixelSize = ts.pixelSize;
                style.flags = ts.flags;
                style.colour = ts.color.value * fill;
                style.outline = ts.strokeRadius;
                style.outlineColour = ts.strokeColor.value * glm::vec4(glm::vec3(stroke), fill.a);
                style.spacing = ts.spacing;
                style.lineSpacing = ts.lineSpacing;
                style.wrapWidth = ts.wrapWidth;
                style.banner = ts.font == "resource2.dat";
                const size_t before = triangles.size();
                text(ts.text.value, glm::vec2(0), style, t);
                textRuns.push_back({before, triangles.size(), apply(t, glm::vec2(0))});
            }
        }
    }
    if (!childrenDrawn)
        for (int32_t child : n.children)
            drawNode(doc, child, t, fill, stroke, false, depth + 1);
    // §5.5: a resized widget stretches what was drawn for it, in its bind space.
    if (!isFileRoot && n.widget >= 0 && doc.widgets[n.widget].deformed) {
        const plasma::Widget &w = doc.widgets[n.widget];
        const glm::mat4 &bind = w.bindMatrix;
        const glm::mat4 unbind = glm::inverse(bind);
        const Transform toLocal = glm::inverse(t);
        const glm::vec2 B = w.innerBindPos, S = w.innerBindSize;
        auto warp = [&](glm::vec2 position) {
            const glm::vec2 local = apply(toLocal, position);
            glm::vec2 p = glm::vec2(bind * glm::vec4(local, 0, 1));
            glm::vec2 u;
            for (int k = 0; k < 2; k++)
                u[k] = p[k] < B[k] ? 0.f : p[k] >= B[k] + S[k] ? 1.f : (p[k] - B[k]) / S[k];
            p += (w.deformedPos - B) + u * (w.deformedSize - S);
            return apply(t, glm::vec2(unbind * glm::vec4(p, 0, 1)));
        };
        size_t i = first;
        for (TextRun &run : textRuns) {
            if (run.end <= first || run.first < first)
                continue;
            for (; i < run.first; i++)
                triangles[i].position = warp(triangles[i].position);
            const glm::vec2 moved = warp(run.origin) - run.origin;
            for (; i < run.end; i++)
                triangles[i].position += moved;
            run.origin += moved;
        }
        for (; i < triangles.size(); i++)
            triangles[i].position = warp(triangles[i].position);
    }
}

const AtlasRect *Painter::texture(const plasma::Document &doc, int32_t id) {
    const plasma::Texture *tex = doc.textureById(id);
    if (!tex || tex->rgba.size() != (size_t)tex->width * tex->height * 4)
        return nullptr;
    if (const AtlasRect *rect = atlas.placed(tex->rgba.data(), tex->width, tex->height))
        return rect;
    AtlasRect rect;
    if (!atlas.add(tex->width, tex->height, tex->rgba.data(), rect))
        return nullptr;
    atlas.remember(tex->rgba.data(), tex->width, tex->height, rect);
    return atlas.placed(tex->rgba.data(), tex->width, tex->height);
}

void Painter::mesh(const plasma::Document &doc, const plasma::MeshShape &m, const Transform &t, glm::vec4 fill,
                   glm::vec4 stroke) {
    const auto &positions = m.positions.value;
    auto at = [&](const auto &list, size_t i, auto fallback) { return i < list.size() ? list[i] : fallback; };
    auto vertex = [&](int32_t i) {
        LoopVertex v;
        const size_t k = (size_t)i;
        v.position = at(positions, k, glm::vec2(0));
        v.uv = at(m.texCoords.value, k, glm::vec2(0));
        v.colour = at(m.colors.value, k, glm::vec4(1));
        v.strokeColour = at(m.strokeColors.value, k, glm::vec4(1));
        v.strokeWidth = at(m.strokeWidths.value, k, 0.f);
        v.type = at(m.vertexFlags, k, 0u) & 0xffff;
        return v;
    };
    const AtlasRect *tex = m.texture.value >= 0 ? texture(doc, m.texture.value) : nullptr;
    const AtlasRect white = atlas.white();
    const float scale = std::sqrt(std::fabs(t[0][0] * t[1][1] - t[0][1] * t[1][0]));

    // Refine: a single outline loop with the curve rule (§4.2), a multi-face mesh as a whole.
    PolyMesh poly;
    for (size_t i = 0; i < positions.size(); i++)
        poly.vertices.push_back(vertex((int32_t)i));
    for (const auto &face : m.faces) {
        std::vector<int> loop;
        for (int32_t i : face)
            if (i >= 0 && (size_t)i < positions.size())
                loop.push_back(i);
        if (loop.size() >= 3)
            poly.faces.push_back(std::move(loop));
    }
    const int passes = std::clamp(m.subdivisions, 0, 6);
    if (poly.faces.size() == 1) {
        std::vector<LoopVertex> loop;
        for (int i : poly.faces[0])
            loop.push_back(poly.vertices[i]);
        for (int pass = 0; pass < passes; pass++)
            loop = refine(loop, m.smoothWeight, pass == 0);
        poly.vertices = loop;
        poly.faces[0].resize(loop.size());
        for (size_t i = 0; i < loop.size(); i++)
            poly.faces[0][i] = (int)i;
    } else {
        for (int pass = 0; pass < passes; pass++)
            poly = refineMesh(poly, m.smoothWeight, pass == 0);
    }

    // Fill, face by face.
    if (!(m.flags & plasma::NO_FILL))
        for (const auto &face : poly.faces) {
            std::vector<glm::vec2> p;
            for (int i : face)
                p.push_back(poly.vertices[i].position);
            const std::vector<int> tris = triangulate(p);
            auto corner = [&](int i) {
                const LoopVertex &v = poly.vertices[face[i]];
                Vertex out;
                out.position = apply(t, v.position);
                out.colour = v.colour * fill;
                out.uv = tex ? tex->uv0 + v.uv * (tex->uv1 - tex->uv0) : white.uv0;
                return out;
            };
            for (size_t i = 0; i + 2 < tris.size(); i += 3)
                triangle(corner(tris[i]), corner(tris[i + 1]), corner(tris[i + 2]));
        }

    // Stroke along the boundary: the edges used by one face only.
    if (!(m.flags & plasma::STROKE))
        return;
    std::map<std::pair<int, int>, int> uses;
    for (const auto &face : poly.faces)
        for (size_t i = 0; i < face.size(); i++) {
            const int a = face[i], b = face[(i + 1) % face.size()];
            uses[{std::min(a, b), std::max(a, b)}]++;
        }
    std::vector<std::pair<LoopVertex, LoopVertex>> edges;
    for (const auto &face : poly.faces)
        for (size_t i = 0; i < face.size(); i++) {
            const int a = face[i], b = face[(i + 1) % face.size()];
            if (uses[{std::min(a, b), std::max(a, b)}] == 1)
                edges.emplace_back(poly.vertices[a], poly.vertices[b]);
        }
    const float side = m.strokeAlignment == 0 ? 0.f : (float)m.strokeAlignment;
    for (const auto &[a, b] : edges) {
        const glm::vec2 pa = apply(t, a.position), pb = apply(t, b.position);
        const glm::vec2 d = pb - pa;
        if (glm::dot(d, d) < 1e-8f)
            continue;
        const glm::vec2 normal = glm::normalize(glm::vec2(-d.y, d.x));
        const float wa = a.strokeWidth * scale, wb = b.strokeWidth * scale;
        // Alignment 0 centres the band on the outline, ±1 puts it to one side (P-4).
        const float lo = side == 0 ? -0.5f : std::min(0.f, side), hi = side == 0 ? 0.5f : std::max(0.f, side);
        Vertex q[4];
        q[0].position = pa + normal * (wa * lo);
        q[1].position = pa + normal * (wa * hi);
        q[2].position = pb + normal * (wb * hi);
        q[3].position = pb + normal * (wb * lo);
        q[0].colour = q[1].colour = a.strokeColour * stroke;
        q[2].colour = q[3].colour = b.strokeColour * stroke;
        for (Vertex &v : q)
            v.uv = white.uv0;
        triangle(q[0], q[1], q[2]);
        triangle(q[0], q[2], q[3]);
        // A round joint, so corners close.
        if (side == 0 && wa > 0) {
            const int segments = 10;
            Vertex centre{pa, white.uv0, a.strokeColour * stroke};
            for (int k = 0; k < segments; k++) {
                const float u = 2 * PI * k / segments, v = 2 * PI * (k + 1) / segments;
                Vertex e1 = centre, e2 = centre;
                e1.position = pa + glm::vec2(std::cos(u), std::sin(u)) * wa * 0.5f;
                e2.position = pa + glm::vec2(std::cos(v), std::sin(v)) * wa * 0.5f;
                triangle(centre, e1, e2);
            }
        }
    }
}

float Painter::textWidth(const std::u16string &s, const TextStyle &style) {
    Font &font = style.banner ? fonts.banner : fonts.main;
    float width = 0, line = 0;
    char32_t previous = 0;
    for (size_t i = 0; i < s.size();) {
        const char32_t c = next(s, i);
        if (c == '\n') {
            width = std::max(width, line);
            line = 0;
            previous = 0;
            continue;
        }
        if (previous)
            line += font.kerning(previous, c, style.pixelSize);
        line += font.glyph(c, style.pixelSize, 0, atlas).advance + style.spacing;
        previous = c;
    }
    return std::max(width, line);
}

void Painter::text(const std::u16string &s, glm::vec2 at, const TextStyle &style, const Transform &t) {
    Font &font = style.banner ? fonts.banner : fonts.main;
    const float px = style.pixelSize;
    // Lines: explicit breaks, and with WRAP breaks at spaces before the wrapping width.
    std::vector<std::u16string> lines(1);
    for (char16_t c : s) {
        if (c == u'\r')
            continue;
        if (c == u'\n') {
            lines.emplace_back();
            continue;
        }
        lines.back() += c;
        if ((style.flags & plasma::WRAP) && c != u' ' && textWidth(lines.back(), style) > style.wrapWidth) {
            const size_t space = lines.back().rfind(u' ');
            if (space != std::u16string::npos) {
                std::u16string rest = lines.back().substr(space + 1);
                lines.back().resize(space);
                lines.push_back(std::move(rest));
            }
        }
    }
    const float lineHeight = font.lineHeight(px) + style.lineSpacing;
    float y = at.y;
    if (style.flags & plasma::MIDDLE) // the block centred on the origin
        y += font.ascent(px) - (lineHeight * (float)lines.size() - (font.lineHeight(px) - font.ascent(px) - font.descent(px))) * 0.5f;
    for (int pass = style.outline > 0 ? 0 : 1; pass < 2; pass++) {
        const float outline = pass == 0 ? style.outline : 0.f;
        const glm::vec4 colour = pass == 0 ? style.outlineColour : style.colour;
        float lineY = y;
        for (const std::u16string &line : lines) {
            const float width = textWidth(line, style);
            const int32_t align = style.flags & plasma::ALIGN_MASK;
            float x = at.x - (align == plasma::CENTRE ? width * 0.5f : align == plasma::RIGHT ? width : 0.f);
            char32_t previous = 0;
            for (size_t i = 0; i < line.size();) {
                const char32_t c = next(line, i);
                if (previous)
                    x += font.kerning(previous, c, px);
                const Glyph &g = font.glyph(c, px, outline, atlas);
                if (g.size.x > 0) {
                    const glm::vec2 p0 = glm::vec2(x, lineY) + g.offset, p1 = p0 + g.size;
                    Vertex q[4];
                    q[0] = {apply(t, p0), g.rect.uv0, colour};
                    q[1] = {apply(t, {p1.x, p0.y}), {g.rect.uv1.x, g.rect.uv0.y}, colour};
                    q[2] = {apply(t, p1), g.rect.uv1, colour};
                    q[3] = {apply(t, {p0.x, p1.y}), {g.rect.uv0.x, g.rect.uv1.y}, colour};
                    triangle(q[0], q[1], q[2]);
                    triangle(q[0], q[2], q[3]);
                }
                x += font.glyph(c, px, 0, atlas).advance + style.spacing;
                previous = c;
            }
            lineY += lineHeight;
        }
    }
}

void Painter::rectangle(glm::vec2 a, glm::vec2 b, glm::vec4 colour) {
    const glm::vec2 uv = atlas.white().uv0;
    const Vertex q[4] = {{a, uv, colour}, {{b.x, a.y}, uv, colour}, {b, uv, colour}, {{a.x, b.y}, uv, colour}};
    triangle(q[0], q[1], q[2]);
    triangle(q[0], q[2], q[3]);
}

void Painter::image(const AtlasRect &picture, glm::vec2 a, glm::vec2 b, glm::vec4 tint) {
    const glm::vec2 u0 = picture.uv0, u1 = picture.uv1;
    const Vertex q[4] = {{a, u0, tint}, {{b.x, a.y}, {u1.x, u0.y}, tint}, {b, u1, tint}, {{a.x, b.y}, {u0.x, u1.y}, tint}};
    triangle(q[0], q[1], q[2]);
    triangle(q[0], q[2], q[3]);
}

plasma::Document clone(const plasma::Document &doc, int32_t node) {
    plasma::Document out;
    out.textures = doc.textures;
    // Breadth-first, so the template itself becomes node 0.
    std::vector<int32_t> order{node};
    std::map<int32_t, int32_t> renumber{{node, 0}};
    for (size_t i = 0; i < order.size(); i++)
        for (int32_t child : doc.nodes[order[i]].children)
            if (!renumber.count(child)) {
                renumber[child] = (int32_t)order.size();
                order.push_back(child);
            }
    for (int32_t old : order) {
        plasma::Node n = doc.nodes[old];
        for (int32_t &child : n.children)
            child = renumber[child];
        if (n.shape >= 0) {
            out.shapes.push_back(doc.shapes[n.shape]);
            n.shape = (int32_t)out.shapes.size() - 1;
        }
        if (n.transformation >= 0) {
            out.transformations.push_back(doc.transformations[n.transformation]);
            n.transformation = (int32_t)out.transformations.size() - 1;
        }
        if (n.display >= 0) {
            out.displays.push_back(doc.displays[n.display]);
            n.display = (int32_t)out.displays.size() - 1;
        }
        if (n.widget >= 0) {
            out.widgets.push_back(doc.widgets[n.widget]);
            n.widget = (int32_t)out.widgets.size() - 1;
        }
        out.nodes.push_back(std::move(n));
    }
    return out;
}

void resizeWidget(plasma::Document &doc, int32_t node, glm::vec2 size) {
    const int32_t index = doc.nodes[node].widget;
    if (index < 0)
        return;
    plasma::Widget &w = doc.widgets[index];
    w.deformed = true;
    w.deformedPos = w.innerBindPos;
    w.deformedSize = w.innerBindSize + (size - w.bindSize);
}

void placeWidget(plasma::Document &doc, int32_t node, glm::vec2 at) {
    plasma::Node &n = doc.nodes[node];
    if (n.widget < 0 || n.transformation < 0)
        return;
    const plasma::Widget &w = doc.widgets[n.widget];
    // The bind rectangle's corner, in the node's own space, then where the node puts it.
    const glm::vec2 local = glm::vec2(glm::inverse(w.bindMatrix) * glm::vec4(w.bindPos, 0, 1));
    const glm::vec2 now = apply(Painter::nodeTransform(doc, n), local);
    doc.transformations[n.transformation].translation.value += at - now;
}

std::u16string utf16(const std::string &s) {
    std::u16string out;
    for (size_t i = 0; i < s.size();) {
        const uint8_t b = (uint8_t)s[i];
        char32_t c;
        int extra;
        if (b < 0x80) {
            c = b;
            extra = 0;
        } else if (b < 0xe0) {
            c = b & 0x1f;
            extra = 1;
        } else if (b < 0xf0) {
            c = b & 0x0f;
            extra = 2;
        } else {
            c = b & 0x07;
            extra = 3;
        }
        i++;
        for (int k = 0; k < extra && i < s.size(); k++, i++)
            c = (c << 6) | ((uint8_t)s[i] & 0x3f);
        if (c >= 0x10000) {
            c -= 0x10000;
            out += (char16_t)(0xd800 + (c >> 10));
            out += (char16_t)(0xdc00 + (c & 0x3ff));
        } else {
            out += (char16_t)c;
        }
    }
    return out;
}

} // namespace ui
