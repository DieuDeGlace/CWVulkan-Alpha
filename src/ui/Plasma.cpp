#include "ui/Plasma.h"
#include "ZlibStream.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <type_traits>

namespace ui::plasma {
namespace {

// §2.2: the library key, the hash of "PlasmaXGraphics".
constexpr uint64_t KEY = 0xFB3A260B5EC7CD5CULL;

struct Bytes {
    const uint8_t *p = nullptr, *end = nullptr;
    size_t left() const { return (size_t)(end - p); }
    void need(size_t n) const {
        if (left() < n)
            throw std::runtime_error("record runs past its end");
    }
    uint32_t u32() {
        need(4);
        uint32_t v;
        std::memcpy(&v, p, 4);
        p += 4;
        return v;
    }
    int32_t i32() { return (int32_t)u32(); }
    float f32() {
        const uint32_t v = u32();
        float f;
        std::memcpy(&f, &v, 4);
        return f;
    }
    Bytes take(size_t n) {
        need(n);
        Bytes b{p, p + n};
        p += n;
        return b;
    }
    std::u16string wstring() {
        const uint32_t n = u32();
        need((size_t)n * 2);
        std::u16string s(n, u'\0');
        std::memcpy(s.data(), p, (size_t)n * 2);
        p += (size_t)n * 2;
        return s;
    }
};

std::string utf8(const std::u16string &s) {
    std::string out;
    for (size_t i = 0; i < s.size(); i++) {
        uint32_t c = s[i];
        if (c >= 0xd800 && c < 0xdc00 && i + 1 < s.size())
            c = 0x10000 + ((c - 0xd800) << 10) + (s[++i] - 0xdc00);
        if (c < 0x80) {
            out += (char)c;
        } else if (c < 0x800) {
            out += (char)(0xc0 | (c >> 6));
            out += (char)(0x80 | (c & 63));
        } else if (c < 0x10000) {
            out += (char)(0xe0 | (c >> 12));
            out += (char)(0x80 | ((c >> 6) & 63));
            out += (char)(0x80 | (c & 63));
        } else {
            out += (char)(0xf0 | (c >> 18));
            out += (char)(0x80 | ((c >> 12) & 63));
            out += (char)(0x80 | ((c >> 6) & 63));
            out += (char)(0x80 | (c & 63));
        }
    }
    return out;
}

// §2.1: records, with the file-wide name table.
class Reader {
  public:
    std::vector<std::string> names{""};
    bool scrambled = false;

    // Calls `visit(name, payload)` for each record of `in`, in order.
    void records(Bytes in, const std::function<void(const std::string &, Bytes)> &visit) {
        while (in.left() > 0) {
            const std::string &name = token(in);
            const std::string copy = name; // (the table may grow while visiting)
            const uint32_t size = in.u32();
            visit(copy, in.take(size));
        }
    }

  private:
    const std::string &token(Bytes &in) {
        uint32_t t = in.u32();
        while (t == 0) {
            Bytes def = in.take(in.u32());
            const uint32_t number = def.u32();
            const uint32_t length = def.u32();
            Bytes text = def.take(length);
            std::vector<uint8_t> raw(text.p, text.end);
            if (names.size() <= number)
                names.resize(number + 1);
            names[number] = scrambled ? unscramble(raw) : std::string(raw.begin(), raw.end());
            t = in.u32();
        }
        if (t >= names.size())
            throw std::runtime_error("record uses an undefined name");
        return names[t];
    }
};

// §2.3, §2.4: an attribute's frames as raw bytes, then decoded by type.
struct RawAttribute {
    int32_t arraySize = -1;
    std::vector<Bytes> frames;
    std::vector<Sequence> sequences;
};

RawAttribute readAttribute(Reader &r, Bytes payload) {
    RawAttribute a;
    r.records(payload, [&](const std::string &name, Bytes b) {
        if (name == "Attribute.frame" || name == "ArrayAttribute.frame")
            a.frames.push_back(b);
        else if (name == "ArrayAttribute.size")
            a.arraySize = b.i32();
        else if (name == "Attribute.sequence") {
            Sequence s;
            r.records(b, [&](const std::string &field, Bytes v) {
                if (field == "Attribute.sequence.wname")
                    s.name = utf8(v.wstring());
                else if (field == "Attribute.sequence.key") {
                    Key k;
                    r.records(v, [&](const std::string &kf, Bytes kv) {
                        if (kf == "Attribute.sequence.key.frame")
                            k.frame = kv.i32();
                        else if (kf == "Attribute.sequence.key.time")
                            k.timeMs = kv.i32();
                        else if (kf == "Attribute.sequence.key.smoothness")
                            k.smoothness = kv.f32();
                    });
                    s.keys.push_back(k);
                }
            });
            a.sequences.push_back(std::move(s));
        }
    });
    return a;
}

template <class T> T decodeOne(Bytes &b);
template <> float decodeOne(Bytes &b) { return b.f32(); }
template <> int32_t decodeOne(Bytes &b) { return b.i32(); }
template <> glm::vec2 decodeOne(Bytes &b) {
    const float x = b.f32();
    return {x, b.f32()};
}
template <> glm::vec3 decodeOne(Bytes &b) {
    const float x = b.f32(), y = b.f32();
    return {x, y, b.f32()};
}
template <> glm::vec4 decodeOne(Bytes &b) {
    const float x = b.f32(), y = b.f32(), z = b.f32();
    return {x, y, z, b.f32()};
}
// Row-major with the translation in the last row: read in memory order this is the
// column-vector matrix glm expects.
template <> glm::mat4 decodeOne(Bytes &b) {
    glm::mat4 m;
    for (int c = 0; c < 4; c++)
        for (int k = 0; k < 4; k++)
            m[c][k] = b.f32();
    return m;
}
template <> std::u16string decodeOne(Bytes &b) { return b.wstring(); }

template <class T> void decode(Reader &r, Bytes payload, Attribute<T> &out) {
    RawAttribute raw = readAttribute(r, payload);
    for (Bytes b : raw.frames)
        out.frames.push_back(decodeOne<T>(b));
    out.sequences = std::move(raw.sequences);
    if (!out.frames.empty())
        out.value = out.frames[0];
}

template <class T> void decode(Reader &r, Bytes payload, Attribute<std::vector<T>> &out) {
    RawAttribute raw = readAttribute(r, payload);
    for (Bytes b : raw.frames) {
        std::vector<T> values;
        for (int32_t i = 0; i < raw.arraySize; i++)
            values.push_back(decodeOne<T>(b));
        out.frames.push_back(std::move(values));
    }
    out.sequences = std::move(raw.sequences);
    if (!out.frames.empty())
        out.value = out.frames[0];
}

std::vector<uint32_t> intArray(Bytes b) {
    std::vector<uint32_t> v(b.u32());
    for (auto &x : v)
        x = b.u32();
    return v;
}

// The property name after "<Kind>.".
std::string field(const std::string &name) {
    const size_t dot = name.find('.');
    return dot == std::string::npos ? name : name.substr(dot + 1);
}

Texture readTexture(Reader &r, Bytes payload) {
    Texture t;
    r.records(payload, [&](const std::string &name, Bytes b) {
        const std::string f = field(name);
        if (f == "wname")
            t.name = utf8(b.wstring());
        else if (f == "id")
            t.id = b.i32();
        else if (f == "width")
            t.width = b.i32();
        else if (f == "height")
            t.height = b.i32();
        else if (f == "compressedPixels") {
            Bytes z = b.take(b.u32());
            if (auto pixels = zlibInflate(z.p, z.left()))
                t.rgba = std::move(*pixels);
        } else if (f == "pixels")
            t.rgba.assign(b.p, b.end);
    });
    return t;
}

Shape readMesh(Reader &r, Bytes payload) {
    Shape s;
    MeshShape &m = s.mesh;
    r.records(payload, [&](const std::string &name, Bytes b) {
        const std::string f = field(name);
        if (f == "wname")
            s.name = utf8(b.wstring());
        else if (f == "face") {
            const auto v = intArray(b);
            m.faces.emplace_back(v.begin(), v.end());
        } else if (f == "vertexFlags")
            m.vertexFlags = intArray(b);
        else if (f == "vertexPositions")
            decode(r, b, m.positions);
        else if (f == "vertexTexCoords")
            decode(r, b, m.texCoords);
        else if (f == "vertexColors")
            decode(r, b, m.colors);
        else if (f == "strokeColors")
            decode(r, b, m.strokeColors);
        else if (f == "strokeWidths")
            decode(r, b, m.strokeWidths);
        else if (f == "texture")
            decode(r, b, m.texture);
        else if (f == "textureTranslation")
            decode(r, b, m.textureTranslation);
        else if (f == "texturePivot")
            decode(r, b, m.texturePivot);
        else if (f == "textureRotation")
            decode(r, b, m.textureRotation);
        else if (f == "textureOpacity")
            decode(r, b, m.textureOpacity);
        else if (f == "subdivisions")
            m.subdivisions = b.i32();
        else if (f == "flags")
            m.flags = b.i32();
        else if (f == "strokeAlignment")
            m.strokeAlignment = b.i32();
        else if (f == "strokeJointType")
            m.strokeJoint = b.i32();
        else if (f == "strokeCapType")
            m.strokeCap = b.i32();
        else if (f == "smoothWeight")
            m.smoothWeight = b.f32();
    });
    return s;
}

Shape readText(Reader &r, Bytes payload) {
    Shape s;
    s.kind = Shape::Kind::Text;
    TextShape &t = s.text;
    r.records(payload, [&](const std::string &name, Bytes b) {
        const std::string f = field(name);
        if (f == "wname")
            s.name = utf8(b.wstring());
        else if (f == "string")
            decode(r, b, t.text);
        else if (f == "color")
            decode(r, b, t.color);
        else if (f == "strokeColor")
            decode(r, b, t.strokeColor);
        else if (f == "flags")
            t.flags = b.i32();
        else if (f == "wfontName")
            t.font = utf8(b.wstring());
        else if (f == "pixelSize")
            t.pixelSize = b.f32();
        else if (f == "strokeRadius")
            t.strokeRadius = b.f32();
        else if (f == "spacing")
            t.spacing = b.f32();
        else if (f == "lineSpacing")
            t.lineSpacing = b.f32();
        else if (f == "wrapWidth")
            t.wrapWidth = b.f32();
    });
    return s;
}

Transformation readTransformation(Reader &r, Bytes payload) {
    Transformation t;
    t.deformation.value = glm::mat4(1);
    r.records(payload, [&](const std::string &name, Bytes b) {
        const std::string f = field(name);
        if (f == "translation")
            decode(r, b, t.translation);
        else if (f == "rotation")
            decode(r, b, t.rotation);
        else if (f == "pivot")
            decode(r, b, t.pivot);
        else if (f == "deformation")
            decode(r, b, t.deformation);
    });
    return t;
}

Display readDisplay(Reader &r, Bytes payload) {
    Display d;
    d.visibility.value = 1;
    d.fillColor.value = d.strokeColor.value = glm::vec4(1);
    r.records(payload, [&](const std::string &name, Bytes b) {
        const std::string f = field(name);
        if (f == "visibility")
            decode(r, b, d.visibility);
        else if (f == "clipping")
            decode(r, b, d.clipping);
        else if (f == "fillColor")
            decode(r, b, d.fillColor);
        else if (f == "strokeColor")
            decode(r, b, d.strokeColor);
        else if (f == "blurRadius")
            decode(r, b, d.blurRadius);
        else if (f == "flags")
            d.flags = b.i32();
    });
    return d;
}

Widget readWidget(Reader &r, const std::string &kind, Bytes payload) {
    Widget w;
    w.kind = kind;
    r.records(payload, [&](const std::string &name, Bytes b) {
        const std::string f = field(name);
        if (f == "wname")
            w.name = utf8(b.wstring());
        else if (f == "innerBindPos")
            w.innerBindPos = decodeOne<glm::vec2>(b);
        else if (f == "innerBindSize")
            w.innerBindSize = decodeOne<glm::vec2>(b);
        else if (f == "bindPos")
            w.bindPos = decodeOne<glm::vec2>(b);
        else if (f == "bindSize")
            w.bindSize = decodeOne<glm::vec2>(b);
        else if (f == "bindMatrix")
            w.bindMatrix = decodeOne<glm::mat4>(b);
        else if (f == "horizontalAlignment")
            w.horizontalAlignment = b.i32();
        else if (f == "verticalAlignment")
            w.verticalAlignment = b.i32();
        else if (f == "flags")
            w.flags = b.i32();
    });
    return w;
}

Node readNode(Reader &r, Bytes payload) {
    Node n;
    r.records(payload, [&](const std::string &name, Bytes b) {
        const std::string f = field(name);
        if (f == "wname")
            n.name = utf8(b.wstring());
        else if (f == "shape")
            n.shape = b.i32();
        else if (f == "transformation")
            n.transformation = b.i32();
        else if (f == "display")
            n.display = b.i32();
        else if (f == "widget")
            n.widget = b.i32();
        else if (f == "child")
            n.children.push_back(b.i32());
        else if (f == "flags")
            n.flags = b.i32();
    });
    return n;
}

} // namespace

std::string unscramble(const std::vector<uint8_t> &in) {
    const size_t n = in.size();
    if (n == 0)
        return {};
    const uint32_t r = (uint32_t)(KEY & 0xffffffffu) % (uint32_t)n;
    std::string out(n, '\0');
    for (size_t i = 0; i < n; i++) {
        const size_t d = (i + r) % n;
        out[d] = (char)(uint8_t)(in[i] - (uint8_t)(KEY >> (8 * (d % 8))));
    }
    return out;
}

const Texture *Document::textureById(int32_t id) const {
    for (const Texture &t : textures)
        if (t.id == id)
            return &t;
    return nullptr;
}

int32_t Document::find(const std::string &name, int32_t from) const {
    if (from < 0 || from >= (int32_t)nodes.size())
        return -1;
    if (nodes[from].name == name)
        return from;
    for (int32_t c : nodes[from].children)
        if (const int32_t hit = find(name, c); hit >= 0)
            return hit;
    return -1;
}

std::optional<Document> read(const std::vector<uint8_t> &bytes, std::string *error) {
    Document doc;
    Reader r;
    try {
        r.records({bytes.data(), bytes.data() + bytes.size()}, [&](const std::string &name, Bytes b) {
            if (name == "PlasmaGraphics") {
                if (b.i32() > 1)
                    throw std::runtime_error("invalid version");
            } else if (name == "Seal") {
                r.scrambled = true; // every name defined from here on (§2.2)
            } else if (name == "Texture") {
                doc.textures.push_back(readTexture(r, b));
            } else if (name == "SmoothMeshShape") {
                doc.shapes.push_back(readMesh(r, b));
            } else if (name == "TextShape") {
                doc.shapes.push_back(readText(r, b));
            } else if (name == "GenericShape") {
                Shape s;
                s.kind = Shape::Kind::Generic;
                doc.shapes.push_back(s);
            } else if (name == "Transformation") {
                doc.transformations.push_back(readTransformation(r, b));
            } else if (name == "Display") {
                doc.displays.push_back(readDisplay(r, b));
            } else if (name == "Widget" || name == "Button" || name == "ListWidget" || name == "Edit" ||
                       name == "ScrollButton" || name == "ScrollSlider" || name == "PopUpButton") {
                doc.widgets.push_back(readWidget(r, name, b));
            } else if (name == "Node") {
                doc.nodes.push_back(readNode(r, b));
            }
        });
    } catch (const std::exception &e) {
        if (error)
            *error = e.what();
        return std::nullopt;
    }
    doc.nameCount = r.names.size() - 1;
    if (doc.nodes.empty()) {
        if (error)
            *error = "no nodes";
        return std::nullopt;
    }
    return doc;
}

std::optional<Document> load(const std::filesystem::path &file, std::string *error) {
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        if (error)
            *error = "cannot open " + file.string();
        return std::nullopt;
    }
    const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return read(bytes, error);
}

namespace {

// §6.5: one key segment of a sequence, a cubic Bézier through the key values with control
// points from the neighbours and the keys' smoothness.
template <class T> T bezier(const T &p0, const T &p1, const T &p2, const T &p3, float s1, float s2, float u) {
    const T c1 = p1 + 0.25f * (s1 * (p2 - p0) + (1 - s1) * (p2 - p1));
    const T c2 = p2 - 0.25f * (s2 * (p3 - p1) + (1 - s2) * (p2 - p1));
    const float v = 1 - u;
    return v * v * v * p1 + 3 * u * v * v * c1 + 3 * u * u * v * c2 + u * u * u * p2;
}
template <class T> std::vector<T> bezier(const std::vector<T> &p0, const std::vector<T> &p1, const std::vector<T> &p2,
                                         const std::vector<T> &p3, float s1, float s2, float u) {
    std::vector<T> out = p1;
    for (size_t k = 0; k < out.size() && k < p0.size() && k < p2.size() && k < p3.size(); k++)
        out[k] = bezier(p0[k], p1[k], p2[k], p3[k], s1, s2, u);
    return out;
}
// Integer and text attributes step: key i's value while u < 1, key i + 1's at u = 1.
int32_t bezier(int32_t, int32_t p1, int32_t p2, int32_t, float, float, float u) { return u < 1 ? p1 : p2; }
std::u16string bezier(const std::u16string &, const std::u16string &p1, const std::u16string &p2, const std::u16string &,
                      float, float, float u) {
    return u < 1 ? p1 : p2;
}

// Frame `f` of an attribute; frame 0 is the current value (§2.4). Null for a missing frame.
template <class T> const T *frameOf(const Attribute<T> &a, int32_t f) {
    if (f == 0)
        return &a.value;
    return f > 0 && f < (int32_t)a.frames.size() ? &a.frames[(size_t)f] : nullptr;
}

// §6.5 evaluating, at time `t` of the working key list: writes the current value and says
// whether the attribute still runs (rulings manacubebar §2). Played once, the last key is
// applied on the first evaluation at or after its time and the next one reports the end; a
// key on a missing frame ends it too.
template <class T> bool evaluate(Attribute<T> &a, float t) {
    Playback &pb = a.playback;
    const std::vector<Key> &k = pb.keys;
    const size_t n = k.size();
    if (n == 0)
        return false;
    const bool looping = pb.periodMs >= 1;
    if (n == 1 || (!looping && t >= (float)k[n - 1].timeMs)) {
        const Key &last = k[n - 1];
        if (!looping && pb.applied == last.frame)
            return false;
        const T *v = frameOf(a, last.frame);
        if (!v)
            return false;
        a.value = *v;
        pb.applied = last.frame;
        return true;
    }
    if (!looping && t < (float)k[0].timeMs) {
        const T *v = frameOf(a, k[0].frame);
        if (!v)
            return false;
        a.value = *v;
        pb.applied = -1;
        return true;
    }
    // The segment i, i + 1 holding t, searched from the one found last time. Looping, the key
    // times repeat every period: there are n segments, the last one from key n − 1 to key 0 a
    // period later, and neighbours wrap; played once, neighbours clamp to the ends.
    size_t i = std::min(pb.segment, looping ? n - 1 : n - 2), before, after, beyond;
    float local = t;
    int32_t from, to;
    if (looping) {
        const float period = (float)pb.periodMs;
        local = t - std::floor(t / period) * period;
        if (local < (float)k[0].timeMs) {
            local += period;
            i = n - 1;
        } else {
            while (i > 0 && local < (float)k[i].timeMs)
                i--;
            while (i + 1 < n && local > (float)k[i + 1].timeMs)
                i++;
        }
        before = (i + n - 1) % n, after = (i + 1) % n, beyond = (i + 2) % n;
        from = k[i].timeMs;
        to = i + 1 < n ? k[i + 1].timeMs : k[0].timeMs + pb.periodMs;
    } else {
        while (i > 0 && t < (float)k[i].timeMs)
            i--;
        while (i + 2 < n && t > (float)k[i + 1].timeMs)
            i++;
        before = i == 0 ? 0 : i - 1, after = i + 1, beyond = std::min(i + 2, n - 1);
        from = k[i].timeMs, to = k[i + 1].timeMs;
    }
    pb.segment = i;
    const T *p0 = frameOf(a, k[before].frame), *p1 = frameOf(a, k[i].frame), *p2 = frameOf(a, k[after].frame),
            *p3 = frameOf(a, k[beyond].frame);
    if (!p0 || !p1 || !p2 || !p3)
        return false;
    const float u = (local - (float)from) / (float)std::max(1, to - from);
    T value = bezier(*p0, *p1, *p2, *p3, k[i].smoothness, k[after].smoothness, u);
    a.value = std::move(value);
    pb.applied = -1;
    return true;
}

// §6.5 starting one attribute: false when it has no keyed sequence of that name.
template <class T> bool start(Attribute<T> &a, const std::string &name, int32_t periodMs) {
    const Sequence *s = a.sequence(name);
    if (!s || s->keys.empty())
        return false;
    Playback &pb = a.playback;
    pb.keys = s->keys;
    std::stable_sort(pb.keys.begin(), pb.keys.end(), [](const Key &x, const Key &y) { return x.timeMs < y.timeMs; });
    if (periodMs >= 1)
        std::erase_if(pb.keys, [&](const Key &key) { return key.timeMs >= periodMs; });
    if (!pb.keys.empty() && pb.keys[0].timeMs != 0) {
        // From whatever the value is now, towards the first key: snapshot into frame 1.
        if (a.frames.size() < 2)
            a.frames.resize(2);
        a.frames[1] = a.value;
        pb.keys.insert(pb.keys.begin(), Key{1, 0, pb.keys[0].smoothness});
    }
    pb.elapsedMs = 0;
    pb.periodMs = periodMs;
    pb.segment = 0;
    pb.applied = -1;
    return true;
}

// One update of one attribute: evaluated at its elapsed time, then the time advances.
template <class T> bool step(Attribute<T> &a, float stepMs) {
    Playback &pb = a.playback;
    if (pb.keys.empty())
        return false;
    const bool running = evaluate(a, pb.elapsedMs);
    pb.elapsedMs += stepMs;
    if (!running)
        pb.keys.clear();
    return running;
}

// Calls `f` with every animatable attribute of a shape, transformation or display.
template <class O, class F> void eachAttribute(O &o, F &&f) {
    using Object = std::remove_const_t<O>;
    if constexpr (std::is_same_v<Object, Shape>) {
        auto &m = o.mesh;
        auto &t = o.text;
        f(m.positions), f(m.texCoords), f(m.colors), f(m.strokeColors), f(m.strokeWidths), f(m.texture);
        f(m.textureTranslation), f(m.texturePivot), f(m.textureRotation), f(m.textureOpacity);
        f(t.text), f(t.color), f(t.strokeColor);
    } else if constexpr (std::is_same_v<Object, Transformation>) {
        f(o.translation), f(o.pivot), f(o.rotation), f(o.deformation);
    } else {
        static_assert(std::is_same_v<Object, Display>);
        f(o.visibility), f(o.clipping), f(o.fillColor), f(o.strokeColor), f(o.blurRadius);
    }
}

// Calls `f` with a node's shape, transformation and display, those it has.
template <class D, class F> void eachObject(D &doc, const Node &n, F &&f) {
    if (n.shape >= 0)
        f(doc.shapes[(size_t)n.shape]);
    if (n.transformation >= 0)
        f(doc.transformations[(size_t)n.transformation]);
    if (n.display >= 0)
        f(doc.displays[(size_t)n.display]);
}

bool validNode(const Document &doc, int32_t node) { return node >= 0 && node < (int32_t)doc.nodes.size(); }

void advanceNode(Document &doc, int32_t node, float stepMs, bool updateHidden, int depth) {
    if (!validNode(doc, node) || depth > 64)
        return;
    const Node &n = doc.nodes[(size_t)node];
    updateHidden = updateHidden || (n.flags & UPDATE_HIDDEN);
    if (!updateHidden && n.display >= 0 && doc.displays[(size_t)n.display].visibility.value == 0)
        return;
    const uint32_t update = doc.updates;
    eachObject(doc, n, [&](auto &o) {
        if (o.animated.advancedIn == update)
            return;
        o.animated.advancedIn = update;
        if (!o.animated.playing)
            return;
        bool running = false;
        eachAttribute(o, [&](auto &a) { running = step(a, stepMs) || running; });
        o.animated.playing = running;
    });
    for (int32_t child : n.children)
        advanceNode(doc, child, stepMs, updateHidden, depth + 1);
}

} // namespace

float length(const Document &doc, int32_t node, const std::string &name) {
    if (!validNode(doc, node))
        return 0;
    const Node &n = doc.nodes[(size_t)node];
    float last = 0;
    eachObject(doc, n, [&](const auto &o) {
        eachAttribute(o, [&](const auto &a) {
            if (const Sequence *s = a.sequence(name))
                for (const Key &k : s->keys)
                    last = std::max(last, (float)k.timeMs);
        });
    });
    for (int32_t child : n.children)
        last = std::max(last, length(doc, child, name));
    return last;
}

void play(Document &doc, int32_t node, const std::string &name, int32_t periodMs) {
    if (!validNode(doc, node))
        return;
    const Node &n = doc.nodes[(size_t)node];
    eachObject(doc, n, [&](auto &o) {
        bool started = false;
        eachAttribute(o, [&](auto &a) { started = start(a, name, periodMs) || started; });
        if (started)
            o.animated.playing = true;
    });
    for (int32_t child : n.children)
        play(doc, child, name, periodMs);
}

bool playing(const Document &doc, int32_t node) {
    if (!validNode(doc, node))
        return false;
    const Node &n = doc.nodes[(size_t)node];
    bool any = false;
    eachObject(doc, n, [&](const auto &o) { any = any || o.animated.playing; });
    if (any)
        return true;
    for (int32_t child : n.children)
        if (playing(doc, child))
            return true;
    return false;
}

void advance(Document &doc, int32_t node, float stepMs, bool updateHidden) {
    if (++doc.updates == 0) // (0 is the stamp of objects never advanced)
        doc.updates = 1;
    advanceNode(doc, node, stepMs, updateHidden, 0);
}

} // namespace ui::plasma
