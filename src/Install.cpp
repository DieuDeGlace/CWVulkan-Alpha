#include "Install.h"
#include <cstdlib>
#include <system_error>

namespace install {

bool looksLikeAlpha(const fs::path &dir) {
    std::error_code ec;
    if (!fs::is_regular_file(dir / "Cube.exe", ec))
        return false;
    for (const char *store : {"data1.db", "data2.db", "data3.db", "data4.db"})
        if (!fs::is_regular_file(dir / store, ec))
            return false;
    return true;
}

static fs::path environmentPath(const wchar_t *name) {
    wchar_t *value = nullptr;
    size_t length = 0;
    fs::path result;
    if (_wdupenv_s(&value, &length, name) == 0 && value)
        result = value;
    std::free(value);
    return result;
}

std::vector<fs::path> usualPlaces() {
    // Alpha was never on Steam: owners unpacked it wherever they liked. Try the working folder,
    // then the user's common folders, each directly and one "Cube World Alpha" level down.
    std::vector<fs::path> roots = {fs::current_path()};
    const fs::path home = environmentPath(L"USERPROFILE");
    if (!home.empty())
        for (const char *sub : {"Downloads", "Desktop", "Documents", "Games"})
            roots.push_back(home / sub);
    for (const char *drive : {"C:\\Games", "D:\\Games", "C:\\Program Files (x86)", "C:\\Program Files"})
        roots.push_back(drive);

    std::vector<fs::path> places;
    for (const fs::path &root : roots) {
        places.push_back(root);
        places.push_back(root / "Cube World Alpha");
        places.push_back(root / "Cube World");
        places.push_back(root / "cubeworld" / "Cube World Alpha");
    }
    return places;
}

fs::path locate(const std::string &configured) {
    if (!configured.empty() && looksLikeAlpha(pathOfUtf8(configured)))
        return pathOfUtf8(configured);
    for (const fs::path &place : usualPlaces())
        if (looksLikeAlpha(place))
            return place;
    return {};
}

} // namespace install
