#pragma once

#include "ui/Painter.h"
#include <optional>

namespace ui {

// Shared by the bag, crafting and vendor lists. Only the active group's offset
// is inspected when its range is recalculated.
struct ItemListRange {
    int32_t currentGroup = 0, columns = 0, rows = 0, range = 1;
    std::vector<int32_t> scroll;

    void updateRange(size_t groups, size_t entries);
    int32_t offset() const;
};

// Each list owns its child in parent-local coordinates. A draw can update the
// range without changing the rectangle left by a layout or movement event.
struct ItemListControl : ItemListRange {
    explicit ItemListControl(glm::vec2 panel) : panelSize(panel) { measureGrid(); }
    glm::vec2 panelSize, cellSize{40};
    std::optional<plasma::Document> thumb;
    glm::vec2 thumbAt{0};
    bool placed = false;

    void measure(const std::optional<plasma::Document> &panel, const std::optional<plasma::Document> &cell);
    template<class Groups> void recalculate(const Groups &groups) {
        measureGrid();
        updateRange(groups.size(), size_t(currentGroup) < groups.size() ? groups[currentGroup].size() : 0);
    }
    glm::vec2 cellAt(int32_t col, int32_t row) const;
    glm::vec2 upAt() const { return {panelSize.x - 30, 10}; }
    glm::vec2 downAt() const { return {panelSize.x - 30, panelSize.y - 30}; }
    void placeThumb();
    bool step(int direction);
    bool pressThumb(glm::vec2 local, float pointerY);
    void moveThumb(float pointerY);
    void releaseThumb() { dragY.reset(); }
    void drawThumb(Painter &painter, glm::vec2 origin, glm::vec2 local);

  private:
    void measureGrid();
    std::optional<float> dragY;
};

} // namespace ui
