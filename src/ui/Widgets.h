#pragma once
// Small helpers the code-drawn windows share (inventory, crafting): placing and tinting cloned
// templates, hit boxes, and the outlined text of alpha-ui-screens.md §3.4.
#include "ui/Painter.h"
#include <cmath>
#include <cstdio>
#include <string>

namespace ui::widgets {

// Places a cloned template so its pivot lands on `at` (screens §2).
inline void anchor(plasma::Document &doc, glm::vec2 at) {
    if (!doc.nodes.empty() && doc.nodes[0].transformation >= 0) {
        plasma::Transformation &t = doc.transformations[doc.nodes[0].transformation];
        t.translation.value = at - t.pivot.value;
    }
}

inline bool inside(glm::vec2 p, glm::vec2 at, glm::vec2 size) {
    return p.x >= at.x && p.y >= at.y && p.x < at.x + size.x && p.y < at.y + size.y;
}

inline std::u16string number(int64_t n) { return utf16(std::to_string(n)); }

// round₀.₁ of screens §8.1.4, as text.
inline std::u16string tenths(float x) {
    const float r = std::trunc(std::abs(x) * 10.f + 0.5f) / 10.f;
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%s%.1f", x < 0 && r > 0 ? "-" : "", r);
    std::string s = buffer;
    if (s.size() > 2 && s.compare(s.size() - 2, 2, ".0") == 0)
        s.resize(s.size() - 2);
    return utf16(s);
}

inline void setVisible(plasma::Document &doc, const char *name, bool visible) {
    const int32_t n = doc.find(name);
    if (n >= 0 && doc.nodes[n].display >= 0)
        doc.displays[doc.nodes[n].display].visibility.value = visible ? 1 : 0;
}

inline void tint(plasma::Document &doc, int32_t node, glm::vec4 colour) {
    if (node >= 0 && doc.nodes[node].display >= 0)
        doc.displays[doc.nodes[node].display].fillColor.value = colour;
}

inline void setLabel(plasma::Document &doc, const char *name, const std::u16string &text) {
    for (const plasma::Node &n : doc.nodes)
        if (n.name == name && n.shape >= 0 && doc.shapes[n.shape].kind == plasma::Shape::Kind::Text)
            doc.shapes[n.shape].text.text.value = text;
}

// Code-drawn outlined text (screens §3.4).
inline void outlined(Painter &painter, const std::u16string &text, glm::vec2 at, float size, float stroke,
                     glm::vec4 colour, int32_t flags = 0, float wrap = 0) {
    TextStyle style;
    style.pixelSize = size;
    style.outline = stroke;
    style.colour = colour;
    style.flags = flags;
    if (wrap > 0)
        style.wrapWidth = wrap;
    painter.text(text, at, style);
}

// A PNG's texture added to a cloned template, returning its id.
inline int32_t addTexture(plasma::Document &doc, plasma::Texture texture) {
    texture.id = 1;
    for (const plasma::Texture &t : doc.textures)
        texture.id = std::max(texture.id, t.id + 1);
    doc.textures.push_back(std::move(texture));
    return doc.textures.back().id;
}

} // namespace ui::widgets
