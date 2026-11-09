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

// §2.3, §2.4: an animatable value. Frame 0 is the authored value, frame 1 the library's live
// slot (not trusted from the file), frames 2.. are keyframes. `value` is the live value,
// starting from frame 0 (open question P-1).
template <class T> struct Attribute {
    std::vector<T> frames;
    std::vector<Sequence> sequences;
    T value{};
    const Sequence *sequence(const std::string &name) const {
        for (const Sequence &s : sequences)
            if (s.name == name)
                return &s;
        return nullptr;
    }
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
};

// §3.4
struct Transformation {
    Attribute<glm::vec2> translation, pivot;
    Attribute<glm::vec3> rotation; // degrees, z used
    Attribute<glm::mat4> deformation;
};

// §3.5
struct Display {
    Attribute<int32_t> visibility, clipping;
    Attribute<glm::vec4> fillColor, strokeColor;
    Attribute<float> blurRadius;
    int32_t flags = 0;
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

struct Document {
    std::vector<Texture> textures;
    std::vector<Shape> shapes;
    std::vector<Transformation> transformations;
    std::vector<Display> displays;
    std::vector<Widget> widgets;
    std::vector<Node> nodes; // node 0 is the document root (§3.7)
    size_t nameCount = 0;    // names defined in the file (for checks, §8)

    const Texture *textureById(int32_t id) const;
    // §6.3: the first node named `name` in a depth-first walk from `from`, or -1.
    int32_t find(const std::string &name, int32_t from = 0) const;
};

// §2: reads a whole document; nullopt and `error` when the file does not follow the format.
std::optional<Document> read(const std::vector<uint8_t> &bytes, std::string *error = nullptr);
std::optional<Document> load(const std::filesystem::path &file, std::string *error = nullptr);

// §6.5: sets every attribute of the subtree at `node` that has a sequence called `name` to its
// value `timeMs` after the sequence's start (no looping).
void play(Document &doc, int32_t node, const std::string &name, float timeMs);
// The time of the last key of the sequences called `name` in the subtree at `node` (ms; 0 when
// there are none).
float length(const Document &doc, int32_t node, const std::string &name);

// §2.2: unscrambles a name with the library key (exposed for the checks of §8).
std::string unscramble(const std::vector<uint8_t> &scrambled);

} // namespace ui::plasma
