#pragma once
// Finds the owner's Cube World Alpha install: the folder holding Cube.exe and the four content
// stores. Content is only ever read from there; none is bundled.
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// UTF-8 text of a path (C++20 u8string returns char8_t).
inline std::string utf8(const fs::path &p) {
    auto s = p.u8string();
    return std::string(s.begin(), s.end());
}
inline fs::path pathOfUtf8(const std::string &s) { return fs::path(std::u8string(s.begin(), s.end())); }

namespace install {

// True when `dir` holds Cube.exe and data1.db .. data4.db.
bool looksLikeAlpha(const fs::path &dir);
// The folders tried when no path is configured, most likely first.
std::vector<fs::path> usualPlaces();
// `configured` (AlphaVulkan.ini [Game] Path) when it is an Alpha install, else the first usual
// place that is one; empty when none is found.
fs::path locate(const std::string &configured);

} // namespace install
