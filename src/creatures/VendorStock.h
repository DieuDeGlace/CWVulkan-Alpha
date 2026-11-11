#pragma once
#include "creatures/InventoryValue.h"
#include <functional>

namespace creatures::vendor_stock {

enum class Route { General, Armour, Weapons };
using Draw = std::function<int32_t()>;
// Thirty ordered bands. Inserts into the supplied value without clearing it.
// Native caller admission, imported RNG state and allocation/lifetime are not
// supplied by this kernel. The fixed pet draw requires its ordinary 0..5 remainder.
void generate(InventoryValue &inventory, Route route, const Draw &draw);

} // namespace creatures::vendor_stock
