// Prints the node tree of a .plx file (alpha-ui-plasma.md): names, transforms, displays and
// mesh bounds. Usage: PlxDump file.plx [node name]
#include "ui/Plasma.h"
#include <cstdio>
using namespace ui::plasma;
static const Document *D;
void walk(int i, int depth) {
    const Node &n = D->nodes[i];
    const auto *t = n.transformation >= 0 ? &D->transformations[n.transformation] : nullptr;
    const auto *d = n.display >= 0 ? &D->displays[n.display] : nullptr;
    std::printf("%*s%d '%s' shape %d flags %x", depth * 2, "", i, n.name.c_str(), n.shape, n.flags);
    if (t) std::printf(" t(%.1f,%.1f) p(%.1f,%.1f) rot %.0f scale %.2f", t->translation.value.x, t->translation.value.y, t->pivot.value.x, t->pivot.value.y, t->rotation.value.z, t->deformation.value[0][0]);
    if (d) std::printf(" vis %d clip %d fill(%.2f,%.2f,%.2f,%.2f) dflags %d", d->visibility.value, d->clipping.value, d->fillColor.value.r, d->fillColor.value.g, d->fillColor.value.b, d->fillColor.value.a, d->flags);
    if (n.shape >= 0 && D->shapes[n.shape].kind == Shape::Kind::Mesh) {
        const auto &m = D->shapes[n.shape].mesh;
        glm::vec2 lo(1e9), hi(-1e9);
        for (auto p : m.positions.value) { lo = glm::min(lo, p); hi = glm::max(hi, p); }
        std::printf(" mesh %zu faces %zu verts box (%.0f,%.0f)-(%.0f,%.0f) mflags %d sub %d col0 a %.2f", m.faces.size(), m.positions.value.size(), lo.x, lo.y, hi.x, hi.y, m.flags, m.subdivisions, m.colors.value.empty() ? -1.f : m.colors.value[0].a);
    }
    if (n.widget >= 0) {
        const Widget &w = D->widgets[n.widget];
        std::printf(" %s '%s' bind (%.1f,%.1f)+(%.1f,%.1f) inner (%.1f,%.1f)+(%.1f,%.1f) matrix t(%.1f,%.1f)", w.kind.c_str(),
                    w.name.c_str(), w.bindPos.x, w.bindPos.y, w.bindSize.x, w.bindSize.y, w.innerBindPos.x,
                    w.innerBindPos.y, w.innerBindSize.x, w.innerBindSize.y, w.bindMatrix[3][0], w.bindMatrix[3][1]);
    }
    std::printf("\n");
    if (depth < 4) for (int c : n.children) walk(c, depth + 1);
}
int main(int argc, char **argv) {
    auto doc = load(argv[1]);
    if (!doc) return 1;
    D = &*doc;
    walk(argc > 2 ? doc->find(argv[2]) : 0, 0);
}

