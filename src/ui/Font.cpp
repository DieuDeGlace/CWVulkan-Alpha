#include "ui/Font.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stb_truetype.h>

namespace ui {

Atlas::Atlas(int size) : side(size), image((size_t)size * size * 4, 0) {
    // A 4 x 4 white block; plain fills sample its centre so filtering never reaches a neighbour.
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            std::fill_n(&image[((size_t)y * side + x) * 4], 4, (uint8_t)255);
    const float c = 2.f / side;
    whiteRect = {{c, c}, {c, c}};
    shelfX = 5;
    shelfHeight = 5;
}

bool Atlas::add(int width, int height, const uint8_t *rgba, AtlasRect &out) {
    const int pad = 1;
    if (shelfX + width + pad > side) {
        shelfY += shelfHeight;
        shelfX = 0;
        shelfHeight = 0;
    }
    if (shelfY + height + pad > side || width + pad > side)
        return false;
    for (int y = 0; y < height; y++)
        std::copy_n(rgba + (size_t)y * width * 4, (size_t)width * 4, &image[((size_t)(shelfY + y) * side + shelfX) * 4]);
    out.uv0 = glm::vec2(shelfX, shelfY) / (float)side;
    out.uv1 = glm::vec2(shelfX + width, shelfY + height) / (float)side;
    shelfX += width + pad;
    shelfHeight = std::max(shelfHeight, height + pad);
    changed = true;
    return true;
}

namespace {
const stbtt_fontinfo &fontInfo(const std::shared_ptr<void> &p) { return *(const stbtt_fontinfo *)p.get(); }
} // namespace

bool Font::load(const std::filesystem::path &file, std::string *error) {
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        if (error)
            *error = "cannot open " + file.string();
        return false;
    }
    data.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    auto font = std::make_shared<stbtt_fontinfo>();
    if (!stbtt_InitFont(font.get(), data.data(), stbtt_GetFontOffsetForIndex(data.data(), 0))) {
        if (error)
            *error = file.string() + " is not a TrueType font";
        return false;
    }
    info = font;
    cache.clear();
    return true;
}

// Glyph sizes are em sizes: pixelSize pixels per em.
float Font::scale(float px) const { return stbtt_ScaleForMappingEmToPixels(&fontInfo(info), px); }

float Font::ascent(float px) const {
    int a, d, g;
    stbtt_GetFontVMetrics(&fontInfo(info), &a, &d, &g);
    return a * scale(px);
}

float Font::descent(float px) const {
    int a, d, g;
    stbtt_GetFontVMetrics(&fontInfo(info), &a, &d, &g);
    return -d * scale(px);
}

float Font::lineHeight(float px) const {
    int a, d, g;
    stbtt_GetFontVMetrics(&fontInfo(info), &a, &d, &g);
    return (a - d + g) * scale(px);
}

float Font::kerning(char32_t a, char32_t b, float px) const {
    return stbtt_GetCodepointKernAdvance(&fontInfo(info), (int)a, (int)b) * scale(px);
}

const Glyph &Font::glyph(char32_t c, float px, float outline, Atlas &atlas) {
    const auto key = std::make_tuple(c, (int)std::lround(px * 64), (int)std::lround(outline * 64));
    if (auto it = cache.find(key); it != cache.end())
        return it->second;
    Glyph g;
    const stbtt_fontinfo &f = fontInfo(info);
    const float s = scale(px);
    int advance, bearing;
    stbtt_GetCodepointHMetrics(&f, (int)c, &advance, &bearing);
    g.advance = advance * s;
    int w = 0, h = 0, x0 = 0, y0 = 0;
    uint8_t *alpha = stbtt_GetCodepointBitmap(&f, s, s, (int)c, &w, &h, &x0, &y0);
    if (alpha && w > 0 && h > 0) {
        // The outline: the glyph's coverage spread over a disc of the outline radius.
        const int r = (int)std::ceil(outline);
        const int W = w + 2 * r, H = h + 2 * r;
        std::vector<uint8_t> rgba((size_t)W * H * 4, 255);
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++) {
                float a = 0;
                for (int dy = -r; dy <= r && a < 255; dy++)
                    for (int dx = -r; dx <= r; dx++) {
                        const float d = std::sqrt((float)(dx * dx + dy * dy));
                        if (d > outline + 0.5f && r > 0)
                            continue;
                        const int sx = x - r + dx, sy = y - r + dy;
                        if (sx < 0 || sy < 0 || sx >= w || sy >= h)
                            continue;
                        // Soft edge one pixel wide at the rim of the disc.
                        const float rim = r > 0 ? std::clamp(outline + 0.5f - d, 0.f, 1.f) : 1.f;
                        a = std::max(a, alpha[sy * w + sx] * rim);
                    }
                rgba[((size_t)y * W + x) * 4 + 3] = (uint8_t)a;
            }
        atlas.add(W, H, rgba.data(), g.rect);
        g.offset = {(float)(x0 - r), (float)(y0 - r)};
        g.size = {(float)W, (float)H};
    }
    stbtt_FreeBitmap(alpha, nullptr);
    return cache.emplace(key, g).first->second;
}

} // namespace ui
