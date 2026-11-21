#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace owner {
inline constexpr const char *ALPHA_IMAGE_SHA256 = "84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf";

struct ContentReport {
    std::string executableHash;
    std::array<size_t, 4> storeEntries{};
    std::vector<std::string> problems; // the installation cannot be used
    std::vector<std::string> notes;    // usable, but different from the reference installation
    bool ready() const { return problems.empty(); }
};

// A file of the reference installation (alpha-extractor.md §A1): its size and SHA-256.
struct ReferenceFile {
    const char *name;
    uint64_t size;
    const char *sha256;
};
const std::vector<ReferenceFile> &referenceFiles();
// The reference installation's record counts of data1.db .. data4.db (§A3; not limits).
inline constexpr std::array<size_t, 4> REFERENCE_STORE_ENTRIES{2857, 93, 104, 2};

const std::vector<std::string> &requiredFiles();
// Whether `file` begins with the identifying header of its kind (§A1), judged by `name`:
// SQLite 3 for *.db, the PlasmaGraphics header for *.plx, a TrueType sfnt for resource*.dat;
// any non-empty file for the rest.
bool hasIdentifyingHeader(const std::string &name, const std::filesystem::path &file);
// Quick check (presence, identifying headers, the supported Cube.exe image, loadable fonts and
// a non-empty db.dat, which the original client needs to start: §A7, §A8). The full check also
// opens every store read-only, decodes every record by its store's role, loads the interface
// scenes and compares every file with the reference installation's size and SHA-256 (notes).
// Nothing is created or written in `installation`.
ContentReport inspect(const std::filesystem::path &installation, bool fullCheck);
// Writes decoded owner assets into a new private directory; never overwrites.
bool extract(const std::filesystem::path &installation, const std::filesystem::path &destination, std::string &error);
// Import a closed original single-player Save into a new engine Save directory.
// The caller validates the owned installation before requesting the copy.
bool importSaves(const std::filesystem::path &installation, const std::filesystem::path &destination, std::string &error);
bool remember(const std::filesystem::path &configuration, const std::filesystem::path &installation, std::string &error);
}
