#include "ui/ItemList.h"
#include "ui/Widgets.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>

namespace ui {

void ItemListRange::updateRange(size_t groups, size_t entries) {
    scroll.resize(groups, 0);
    if (size_t(currentGroup) >= groups) {
        range = 1;
        return;
    }
    auto &row = scroll[currentGroup];
    if (!entries) {
        range = row = 0;
        return;
    }
    if (entries > std::numeric_limits<uint32_t>::max())
        throw std::length_error("Inventory row count exceeds the held width");
    if (columns <= 0) { // Degenerate custom skin, outside the original's nonzero-column precondition.
        range = 0;
        return;
    }
    const uint32_t positions = (uint32_t(entries) - 1u) / uint32_t(columns) - uint32_t(rows) + 2u;
    range = std::max(1, std::bit_cast<int32_t>(positions));
    row = std::min(row, range - 1);
}

int32_t ItemListRange::offset() const {
    return size_t(currentGroup) < scroll.size() ? scroll[currentGroup] : 0;
}

void ItemListControl::measure(const std::optional<plasma::Document> &panel, const std::optional<plasma::Document> &cell) {
    if (panel)
        panelSize = widgetSize(*panel, 0, panelSize);
    if (cell)
        cellSize = glm::trunc(widgetSize(*cell, 0, glm::vec2(40)));
    measureGrid();
}

void ItemListControl::measureGrid() {
    columns = int32_t((panelSize.x - 10.f) / (cellSize.x + 5.f));
    rows = int32_t((panelSize.y - 40.f) / (cellSize.y + 5.f));
}

glm::vec2 ItemListControl::cellAt(int32_t col, int32_t row) const {
    return glm::vec2(10, 40) + glm::vec2(col, row) * (cellSize + 5.f);
}

void ItemListControl::placeThumb() {
    if (!thumb || !range || scroll.empty())
        return;
    const int track = int(panelSize.y - 70.f);
    const int height = track / range;
    const int travel = track - height;
    const int y = range == 1 ? 35 : 35 + int(int64_t(travel) * offset() / (range - 1));
    thumbAt = {panelSize.x - 28.f, float(y)};
    const float width = widgetSize(*thumb, 0, {}).x;
    placeWidget(*thumb, 0, thumbAt);
    resizeWidget(*thumb, 0, {width, float(height)});
    placed = true;
}

bool ItemListControl::step(int direction) {
    if (size_t(currentGroup) >= scroll.size())
        return false;
    auto &row = scroll[currentGroup];
    if (direction < 0 ? row <= 0 : row >= range - 1)
        return false;
    row += direction;
    placeThumb();
    return true;
}

bool ItemListControl::pressThumb(glm::vec2 local, float pointerY) {
    if (!placed || !thumb || !range || size_t(currentGroup) >= scroll.size() ||
        !widgets::inside(local, thumbAt, widgetSize(*thumb, 0, {})))
        return false;
    dragY = pointerY;
    return true;
}

void ItemListControl::moveThumb(float pointerY) {
    if (!dragY)
        return;
    const float dy = pointerY - *dragY;
    dragY = pointerY;
    if (!thumb || range <= 0 || size_t(currentGroup) >= scroll.size())
        return;
    const int travel = int(panelSize.y - 70.f) - int(widgetSize(*thumb, 0, {}).y);
    const int y = std::min(std::max(35, int(thumbAt.y + dy)), 35 + travel);
    auto &row = scroll[currentGroup];
    if (travel > 0)
        row = int(int64_t(y - 35) * (range - 1) / travel);
    row = std::clamp(row, 0, range - 1);
    thumbAt.y = float(y);
    placeWidget(*thumb, 0, thumbAt);
}

void ItemListControl::drawThumb(Painter &painter, glm::vec2 origin, glm::vec2 local) {
    if (!placed || !thumb)
        return;
    const bool hover = widgets::inside(local, thumbAt, widgetSize(*thumb, 0, {}));
    widgets::tint(*thumb, 0, hover ? glm::vec4(0, 1, 1, 1) : glm::vec4(1));
    painter.draw(*thumb, 0, translate(origin));
}

} // namespace ui
