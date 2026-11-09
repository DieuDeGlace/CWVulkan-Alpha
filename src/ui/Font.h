#pragma once
// The interface fonts and the texture atlas the interface draws from (alpha-ui-plasma.md §7):
// resource1.dat and resource2.dat are TrueType files in the install folder. Glyphs are
// rasterised on demand (grey-scale, anti-aliased) into one RGBA atlas, with an outlined
// variant for text that has a stroke radius.
#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace ui {

// A rectangle in the atlas, in texture coordinates.
struct AtlasRect {
    glm::vec2 uv0{0}, uv1{0};
};

// One RGBA image packed with rectangles on shelves. Position (0, 0) holds a white block that
// plain fills sample.
class Atlas {
  public:
    explicit Atlas(int size = 2048);
    // Adds an RGBA image; false when the atlas is full.
    bool add(int width, int height, const uint8_t *rgba, AtlasRect &out);
    AtlasRect white() const { return whiteRect; }
    // Where a texture already went, by its pixels' address and size (textures of the loaded
    // interface documents stay put, so each is added once).
    const AtlasRect *placed(const void *pixels, int width, int height) const {
        const auto it = known.find({pixels, width, height});
        return it == known.end() ? nullptr : &it->second;
    }
    void remember(const void *pixels, int width, int height, const AtlasRect &rect) { known[{pixels, width, height}] = rect; }
    int size() const { return side; }
    const std::vector<uint8_t> &pixels() const { return image; }
    // Whether anything was added since the last call (the renderer then uploads it again).
    bool takeChanged() {
        const bool was = changed;
        changed = false;
        return was;
    }

  private:
    int side;
    std::vector<uint8_t> image;
    int shelfX = 0, shelfY = 0, shelfHeight = 0;
    AtlasRect whiteRect;
    bool changed = true;
    std::map<std::tuple<const void *, int, int>, AtlasRect> known;
};

// A glyph placed relative to the pen on the baseline (y down).
struct Glyph {
    AtlasRect rect;
    glm::vec2 offset{0}, size{0}; // pixels
    float advance = 0;            // pixels
};

class Font {
  public:
    bool load(const std::filesystem::path &file, std::string *error = nullptr);
    // Line metrics at a pixel size, in pixels (ascent above the baseline, descent below it as a
    // positive number, plus the line gap).
    float ascent(float px) const;
    float descent(float px) const;
    float lineHeight(float px) const;
    float kerning(char32_t a, char32_t b, float px) const;
    // A glyph's fill (outline = 0) or its outline of that radius, rasterised once and cached.
    const Glyph &glyph(char32_t c, float px, float outline, Atlas &atlas);

  private:
    float scale(float px) const;
    std::vector<uint8_t> data;
    std::shared_ptr<void> info; // stbtt_fontinfo
    std::map<std::tuple<char32_t, int, int>, Glyph> cache;
};

} // namespace ui
