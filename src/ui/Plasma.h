#pragma once
// Alpha's interface files, `.plx` (docs/specs/alpha-ui-plasma.md): a document of textures,
// shapes, transformations, displays, widgets and a tree of nodes, read into plain structs.
// Section numbers point at that spec.
#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>

namespace ui::plasma {

// §2.4: one key of a named sequence.
struct Key {
    int32_t frame = 0; // which frame of the attribute holds the value
    int32_t timeMs = 0;
    float smoothness = 0;
};
struct Sequence {
    std::string name; // e.g. "shine", "button:enter"
    std::vector<Key> keys;
};

// §6.5: an attribute's running sequence. The working key list is empty when none runs.
struct Playback {
    std::vector<Key> keys; // sorted by time, period-filtered, with the implicit start key
    float elapsedMs = 0;   // time evaluated by the next advance
    int32_t periodMs = 0;  // 0 plays once; ≥ 1 loops
    size_t segment = 0;    // where the next segment search starts
    int32_t applied = -1;  // the last key's frame once applied at or after the last key
};

// §2.3, §2.4: an animatable value. `value` is frame 0, the current value: drawing reads it, the
// game and sequence evaluation write it (`frames[0]` keeps the authored copy). Frame 1 is the
// snapshot a sequence takes when its first key is later than 0; frames 2.. are keyframes.
template <class T> struct Attribute {
    std::vector<T> frames;
    std::vector<Sequence> sequences;
    T value{};
    Playback playback;
    const Sequence *sequence(const std::string &name) const {
        for (const Sequence &s : sequences)
            if (s.name == name)
                return &s;
        return nullptr;
    }
};

// §6.5: per-object sequence state (shapes, transformations, displays). `playing` is set when a
// sequence starts on one of its attributes and cleared by the first advance in which none of
// them still runs; `advancedIn` keeps an object to one advance per interface update.
struct Animated {
    bool playing = false;
    uint32_t advancedIn = 0;
};

// §3.1
struct Texture {
    std::string name;
    int32_t id = -1;
    int32_t width = 0, height = 0;
    std::vector<uint8_t> rgba; // rows top to bottom (byte order: P-2)
};

// §3.2
struct MeshShape {
    std::vector<std::vector<int32_t>> faces;
    std::vector<uint32_t> vertexFlags;
    Attribute<std::vector<glm::vec2>> positions, texCoords;
    Attribute<std::vector<glm::vec4>> colors, strokeColors;
    Attribute<std::vector<float>> strokeWidths;
    Attribute<int32_t> texture; // Texture id, -1 none
    Attribute<glm::vec2> textureTranslation, texturePivot;
    Attribute<glm::vec3> textureRotation;
    Attribute<float> textureOpacity;
    int32_t subdivisions = 0, flags = 0, strokeAlignment = 0, strokeJoint = 0, strokeCap = 0;
    float smoothWeight = 0.585786f;
};
enum MeshFlags : int32_t { STROKE = 2, NO_FILL = 4, EXTRUSION = 16 };

// §3.3
struct TextShape {
    Attribute<std::u16string> text;
    Attribute<glm::vec4> color, strokeColor;
    int32_t flags = 0;
    std::string font; // "resource1.dat" or "resource2.dat"
    float pixelSize = 12, strokeRadius = 0, spacing = 0, lineSpacing = 0, wrapWidth = 200;
};
enum TextFlags : int32_t { ALIGN_MASK = 3, CENTRE = 1, RIGHT = 2, MIDDLE = 4, WRAP = 16 };

struct Shape {
    enum class Kind { Mesh, Text, Generic } kind = Kind::Mesh;
    std::string name;
    MeshShape mesh;
    TextShape text;
    Animated animated;
};

// §3.4
struct Transformation {
    Attribute<glm::vec2> translation, pivot;
    Attribute<glm::vec3> rotation; // degrees about x, y, z
    Attribute<glm::mat4> deformation;
    Animated animated;
};

// §3.5
struct Display {
    Attribute<int32_t> visibility, clipping;
    Attribute<glm::vec4> fillColor, strokeColor;
    Attribute<float> blurRadius;
    int32_t flags = 0;
    Animated animated;
};

// §3.6
struct Widget {
    std::string kind; // Widget, Button, Edit, ...
    std::string name;
    glm::vec2 innerBindPos{0}, innerBindSize{0}, bindPos{0}, bindSize{0};
    glm::mat4 bindMatrix{1};
    int32_t horizontalAlignment = 0, verticalAlignment = 0, flags = 0;
    // Live state: the rectangle the game has resized the widget to (§5.5), in bind space.
    bool deformed = false;
    glm::vec2 deformedPos{0}, deformedSize{0};
};

// §3.7
struct Node {
    std::string name;
    int32_t shape = -1, transformation = -1, display = -1, widget = -1;
    std::vector<int32_t> children;
    int32_t flags = 0;
};
// §3.7, rulings manacubebar §3: an update walk enters a hidden node only under this flag.
constexpr int32_t UPDATE_HIDDEN = 0x2000;

struct Document {
    std::vector<Texture> textures;
    std::vector<Shape> shapes;
    std::vector<Transformation> transformations;
    std::vector<Display> displays;
    std::vector<Widget> widgets;
    std::vector<Node> nodes; // node 0 is the document root (§3.7)
    size_t nameCount = 0;    // names defined in the file (for checks, §8)
    uint32_t updates = 0;    // interface updates run by `advance` (the per-update guard)

    const Texture *textureById(int32_t id) const;
    // §6.3: the first node named `name` in a depth-first walk from `from`, or -1.
    int32_t find(const std::string &name, int32_t from = 0) const;
};

// §2: reads a whole document; nullopt and `error` when the file does not follow the format.
std::optional<Document> read(const std::vector<uint8_t> &bytes, std::string *error = nullptr);
std::optional<Document> load(const std::filesystem::path &file, std::string *error = nullptr);

// §6.5 starting: starts the sequence `name` on every attribute of the subtree at `node` that has
// one with at least one key, marking those objects as playing. `periodMs` 0 plays once (every
// call the game makes); ≥ 1 loops with that period.
void play(Document &doc, int32_t node, const std::string &name, int32_t periodMs = 0);
// §6.5, rulings manacubebar §2: whether a sequence plays in the subtree at `node` (its own
// shape, transformation or display, or any descendant's). True from the play call on.
bool playing(const Document &doc, int32_t node);
// Rulings manacubebar §2: one interface update of the subtree at `node`. Each playing object is
// advanced at most once: each attribute is evaluated at its elapsed time, then its time grows
// by `stepMs`; one that stops running drops its keys, and an object none of whose attributes
// ran stops playing. The walk skips a hidden node unless it, or an ancestor, has
// UPDATE_HIDDEN; `updateHidden` passes that flag down from the container holding the subtree.
void advance(Document &doc, int32_t node, float stepMs, bool updateHidden = false);
// The time of the last key of the sequences called `name` in the subtree at `node` (ms; 0 when
// there are none).
float length(const Document &doc, int32_t node, const std::string &name);

// §2.2: unscrambles a name with the library key (exposed for the checks of §8).
std::string unscramble(const std::vector<uint8_t> &scrambled);

} // namespace ui::plasma
