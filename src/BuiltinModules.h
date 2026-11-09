#pragma once
#include "alphavulkan_mod.h"
#include <span>
#include <string_view>

struct BuiltinModule {
    std::string_view id;
    AvModLoadFunction load;
    std::string_view version;
};

// Generated from explicit CMake registrations, avoiding static-init/linker order.
std::span<const BuiltinModule> builtinModules();
