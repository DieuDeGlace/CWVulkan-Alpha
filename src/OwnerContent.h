#pragma once
#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace owner {
inline constexpr const char *ALPHA_IMAGE_SHA256 = "84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf";

struct ContentReport {
    std::string executableHash;
    std::array<size_t, 4> storeEntries{};
    std::vector<std::string> problems;
    bool ready() const { return problems.empty(); }
};

const std::vector<std::string> &requiredFiles();
ContentReport inspect(const std::filesystem::path &installation, bool fullCheck);
// Writes decoded owner assets into a new private directory; never overwrites.
bool extract(const std::filesystem::path &installation, const std::filesystem::path &destination, std::string &error);
bool remember(const std::filesystem::path &configuration, const std::filesystem::path &installation, std::string &error);
}
