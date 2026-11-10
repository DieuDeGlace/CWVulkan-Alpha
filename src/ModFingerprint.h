#pragma once
#include <filesystem>
#include <optional>
#include <string>

// Engine compatibility identity for mod.ini, the declared DLL and the flat
// content keys actually consumed by Content. Paths outside this input set and
// dependencies loaded by a plugin are not covered. Does not load executable code.
std::optional<std::string> modFingerprint(const std::filesystem::path &folder, const std::string &plugin,
                                         std::string *error = nullptr);
