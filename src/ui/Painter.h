#pragma once
// Turns interface documents and text into triangles (alpha-ui-plasma.md §4, §5): node
// transforms, smooth meshes (subdivision, fill, stroke), textures and text shapes, drawn
// depth-first with the display colours multiplied down the tree.
//
// Not in yet: masks (§5.3; a clipping node's children are drawn unclipped and its own shape
// is not drawn), blur layers (§5.4, unused by the shipped files), and the inside of
// subdivided multi-face meshes, which is refined face by face (open question P-8).
#include "ui/Font.h"
#include "ui/Plasma.h"
#include <functional>
#include <string>
#include <vector>

namespace ui {

// One corner of an interface triangle: window pixels (y down), atlas coordinates, colour.
struct Vertex {
    glm::vec2 position{0}, uv{0};
    glm::vec4 colour{1};
};

// The two interface fonts, loaded from the install folder.
struct Fonts {
    Font main;   // resource1.dat, every interface text
    Font banner; // resource2.dat, the land-name banner
    bool load(const std::filesystem::path &install, std::string *error = nullptr);
    Font &byName(const std::string &file) { return file == "resource2.dat" ? banner : main; }
};

// A run of interface triangles drawn the same way. Masks (alpha-ui-plasma.md §5.3) are kept
// in the stencil buffer: MaskIn adds the mask's shape to it, the triangles inside are drawn
// where all `level` masks around them cover, and MaskOut takes the shape away again.
struct Batch {
    // Item: one item model (alpha-ui-screens.md §8.1.6), `first` indexing Frame::items.
    enum Op : uint8_t { Draw, MaskIn, MaskOut, Item };
    Op op = Draw;
    uint8_t level = 0;             // masks around the triangles
    uint32_t first = 0, count = 0; // vertices
};

// An item model drawn in the interface (alpha-ui-screens.md §8.1.6). `camera` takes the
// mesh (voxel units, centred on x/y, base at z = 0) to Alpha's camera space (x right, y up,
// z ahead); `lens` holds the projection's x and y scale and the NDC shift that puts the
// model's centre on its point.
struct ItemDraw {
    uint32_t mesh = 0; // the renderer's mesh id
    glm::mat4 camera{1};
    glm::vec4 lens{0};
};
// §8.1.6 steps 3-5: an item model of `size` voxels centred on window point `at`, turned by
// `turn` (degrees about x, y, z), its largest side `scale` units, `depth` units further away.
ItemDraw itemDraw(uint32_t mesh, glm::vec3 size, bool ring, glm::vec2 at, glm::vec3 turn, float scale, float depth,
                  glm::vec2 window);

// A model placed by `model` (mesh -> blocks) seen from `distance` blocks, turned by `turn`
// (degrees about x, y, z as for items), its origin on window point `at` (character screens §4.1).
ItemDraw modelDraw(uint32_t mesh, const glm::mat4 &model, glm::vec2 at, glm::vec3 turn, float distance, glm::vec2 window);
// The same draw in a colour (attached cubes): kept in the camera matrix's unused bottom row.
inline ItemDraw tinted(ItemDraw d, glm::vec3 colour) {
    d.camera[0][3] = colour.r, d.camera[1][3] = colour.g, d.camera[2][3] = colour.b;
    return d;
}

// What the interface draws in one frame.
struct Frame {
    std::vector<Vertex> triangles;
    std::vector<Batch> batches;
    std::vector<ItemDraw> items;
};

// A 2D affine transform (column vectors).
using Transform = glm::mat3;
Transform translate(glm::vec2 t);

// A text request (§4.4; also used for text the game draws in code).
struct TextStyle {
    float pixelSize = 12;
    int32_t flags = 0;         // plasma::TextFlags
    glm::vec4 colour{1};
    float outline = 0;         // radius in pixels, 0 for none
    glm::vec4 outlineColour{0, 0, 0, 1};
    float spacing = 0, lineSpacing = 0, wrapWidth = 200;
    bool banner = false;       // resource2.dat
};

class Painter {
  public:
    Painter(Fonts &fonts, Atlas &atlas) : fonts(fonts), atlas(atlas) {}

    std::vector<Vertex> triangles;

    // The frame drawn so far, its batches closed.
    Frame take();

    // Draws `node` of `doc` and its subtree. With `isFileRoot` the node's own shape and
    // transform are skipped (node 0 of a file, §3.7).
    void draw(const plasma::Document &doc, int32_t node, const Transform &parent, bool isFileRoot = false);
    // Text at `at` (the first line's baseline, aligned by the flags).
    void text(const std::u16string &s, glm::vec2 at, const TextStyle &style, const Transform &t = Transform(1));
    // The width of one line of text.
    float textWidth(const std::u16string &s, const TextStyle &style);
    void rectangle(glm::vec2 a, glm::vec2 b, glm::vec4 colour);
    // An atlas picture stretched over the rectangle a..b.
    void image(const AtlasRect &picture, glm::vec2 a, glm::vec2 b, glm::vec4 tint = glm::vec4(1));
    // An item model, drawn between the triangles before and after it.
    void item(const ItemDraw &draw);

    // The node's transform in its parent's space (§4.1).
    static Transform nodeTransform(const plasma::Document &doc, const plasma::Node &n);

  private:
    void mesh(const plasma::Document &doc, const plasma::MeshShape &m, const Transform &t, glm::vec4 fill,
              glm::vec4 stroke);
    void triangle(const Vertex &a, const Vertex &b, const Vertex &c) {
        triangles.push_back(a);
        triangles.push_back(b);
        triangles.push_back(c);
    }
    void drawNode(const plasma::Document &doc, int32_t node, const Transform &parent, glm::vec4 fill, glm::vec4 stroke,
                  bool isFileRoot, int depth);
    const AtlasRect *texture(const plasma::Document &doc, int32_t id);
    // Ends the current batch where the triangles stand and starts one of `op` at `level`.
    void cut(Batch::Op op, uint8_t level);

    std::vector<Batch> batches{Batch{}};
    std::vector<ItemDraw> items;
    // Text drawn so far: its triangles [first, end) and its origin in window pixels. A resized
    // widget moves text as a whole by its origin instead of stretching the glyphs.
    struct TextRun {
        size_t first, end;
        glm::vec2 origin;
    };
    std::vector<TextRun> textRuns;
    uint8_t maskLevel = 0;

    Fonts &fonts;
    Atlas &atlas;
};

// Deep-copies `node` and its subtree into a document of its own whose node 0 is that node
// (alpha-ui-plasma.md §6.4). Textures are copied along, so the clone stands alone.
plasma::Document clone(const plasma::Document &doc, int32_t node);

// Resizes a widget node to `size` pixels (alpha-ui-plasma.md §5.5): its inner rectangle grows
// by the change of its outer one, so corners keep their shape (open question P-11).
void resizeWidget(plasma::Document &doc, int32_t node, glm::vec2 size);
// Moves node `node` (a root) so the top-left corner of its widget rectangle, as resized, lands
// on `at` in the parent's space.
void placeWidget(plasma::Document &doc, int32_t node, glm::vec2 at);

// UTF-8 to UTF-16 for text written by the engine.
std::u16string utf16(const std::string &s);

} // namespace ui
