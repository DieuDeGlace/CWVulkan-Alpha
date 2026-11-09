#include "OwnerContent.h"
#include "Audio.h"
#include "Config.h"
#include "CubModel.h"
#include "DataStore.h"
#include "Dictionary.h"
#include "FileHash.h"
#include "Install.h"
#include "ui/Font.h"
#include "ui/Plasma.h"
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <stb_image.h>

namespace owner {
const std::vector<std::string> &requiredFiles() {
    static const std::vector<std::string> names{
        "Cube.exe", "data1.db", "data2.db", "data3.db", "data4.db", "resource1.dat", "resource2.dat",
        "gui.plx", "start.plx", "help.plx", "quest-tag.plx", "cursor.plx"};
    return names;
}

ContentReport inspect(const fs::path &installation, bool fullCheck) {
    ContentReport result;
    for (const std::string &name : requiredFiles()) {
        std::error_code ec;
        const auto path = installation / pathOfUtf8(name);
        if (!fs::is_regular_file(path, ec) || fs::file_size(path, ec) == 0 || ec)
            result.problems.push_back("Missing or empty required file: " + name);
    }
    if (const auto hash = fileSha256(installation / "Cube.exe")) {
        result.executableHash = *hash;
        if (*hash != ALPHA_IMAGE_SHA256)
            result.problems.push_back("Cube.exe is not the supported Alpha image.");
    } else
        result.problems.push_back("Cannot read and identify Cube.exe.");
    if (!result.ready() || !fullCheck)
        return result;

    Content content;
    std::string error;
    if (!content.open(installation, &error)) {
        result.problems.push_back(error);
        return result;
    }
    std::set<std::string> unique;
    for (int index = 0; index < Content::COUNT; index++) {
        const auto keys = content.store(index).keys();
        result.storeEntries[index] = keys.size();
        if (keys.empty())
            result.problems.push_back("Empty content store: data" + std::to_string(index + 1) + ".db");
        for (const auto &key : keys) {
            if (!unique.insert(key).second)
                result.problems.push_back("Duplicate content key: " + key);
            const auto bytes = content.store(index).read(key);
            bool valid = bytes && !bytes->empty();
            if (valid && key.ends_with(".cub")) {
                CubModel model;
                valid = model.load(bytes->data(), bytes->size());
            } else if (valid && key.ends_with(".wav")) {
                valid = bytes->size() >= 12 && std::memcmp(bytes->data(), "RIFF", 4) == 0 &&
                        std::memcmp(bytes->data() + 8, "WAVE", 4) == 0;
                SoundClip sound;
                valid = valid && decodeSound(bytes->data(), bytes->size(), sound) && sound.frames() > 0;
            } else if (valid && key.ends_with(".png")) {
                valid = bytes->size() >= 8 && std::memcmp(bytes->data(), "\x89PNG\r\n\x1a\n", 8) == 0;
                int width = 0, height = 0, channels = 0;
                auto *pixels = valid && bytes->size() <= (size_t)std::numeric_limits<int>::max()
                    ? stbi_load_from_memory(bytes->data(), (int)bytes->size(), &width, &height, &channels, 4) : nullptr;
                valid = pixels && width > 0 && height > 0;
                stbi_image_free(pixels);
            } else if (valid && key.ends_with(".xml")) {
                Dictionary dictionary;
                valid = dictionary.parse(std::string(bytes->begin(), bytes->end()));
            }
            if (!valid)
                result.problems.push_back("Invalid content record: " + key);
            if (result.problems.size() >= 50)
                return result;
        }
    }
    for (const char *name : {"gui.plx", "start.plx", "help.plx", "quest-tag.plx", "cursor.plx"})
        if (!ui::plasma::load(installation / name, &error))
            result.problems.push_back(std::string(name) + ": " + error);
    for (const char *name : {"resource1.dat", "resource2.dat"}) {
        ui::Font font;
        if (!font.load(installation / name, &error))
            result.problems.push_back(std::string(name) + ": " + error);
    }
    return result;
}

namespace {
bool beneath(const fs::path &child, const fs::path &parent) {
    auto at = child.begin();
    for (const auto &part : parent) {
        if (at == child.end() || _wcsicmp(at->c_str(), part.c_str()) != 0)
            return false;
        ++at;
    }
    return true;
}

bool validName(const std::string &key) {
    if (key.empty() || key.back() == '.' || key.back() == ' ' ||
        key.find_first_of("<>:\"/\\|?*") != std::string::npos ||
        std::any_of(key.begin(), key.end(), [](unsigned char c) { return c < 32; }))
        return false;
    const std::wstring stem = pathOfUtf8(key.substr(0, key.find('.'))).wstring();
    for (const wchar_t *device : {L"CON", L"PRN", L"AUX", L"NUL", L"CLOCK$", L"CONIN$", L"CONOUT$"})
        if (_wcsicmp(stem.c_str(), device) == 0)
            return false;
    if (stem.size() == 4 && (_wcsnicmp(stem.c_str(), L"COM", 3) == 0 || _wcsnicmp(stem.c_str(), L"LPT", 3) == 0)) {
        const wchar_t digit = stem.back();
        if ((digit >= L'1' && digit <= L'9') || digit == 0x00b9 || digit == 0x00b2 || digit == 0x00b3)
            return false;
    }
    return true;
}

struct FileNameLess {
    bool operator()(const std::wstring &a, const std::wstring &b) const {
        return CompareStringOrdinal(a.c_str(), (int)a.size(), b.c_str(), (int)b.size(), TRUE) == CSTR_LESS_THAN;
    }
};
}

bool extract(const fs::path &installation, const fs::path &destination, std::string &error) {
    const auto report = inspect(installation, true);
    if (!report.ready()) {
        error = report.problems.front();
        return false;
    }
    const auto source = fs::weakly_canonical(installation);
    const auto requested = fs::absolute(destination).lexically_normal();
    // Resolve the existing parent, rather than probing a nonexistent leaf.
    // Exclusive directory creation below enforces the no-overwrite requirement.
    std::error_code pathError;
    const auto parent = fs::canonical(requested.parent_path(), pathError);
    if (pathError || requested.filename().empty()) {
        error = "The private cache must have an existing parent directory.";
        return false;
    }
    const auto target = parent / requested.filename();
    if (beneath(target, source) || beneath(source, target)) {
        error = "Choose a new private directory outside the original installation.";
        return false;
    }
    Content content;
    if (!content.open(source, &error))
        return false;
    // Preflight every name before creating a cache. Windows names are case-insensitive;
    // records must not alias each other, loose files, devices or the completion marker.
    std::set<std::wstring, FileNameLess> names{L"OWNER-CONTENT.txt", L"OWNER-CONTENT.txt.tmp"};
    for (const auto &name : requiredFiles())
        names.insert(pathOfUtf8(name).wstring());
    for (int index = 0; index < Content::COUNT; index++)
        for (const auto &key : content.store(index).keys())
            if (!validName(key) || !names.insert(pathOfUtf8(key).wstring()).second) {
                error = "Unsafe or colliding asset key: " + key;
                return false;
            }
    if (!CreateDirectoryW(target.c_str(), nullptr)) {
        error = "Cannot create a new private directory; existing directories are never overwritten.";
        return false;
    }
    std::vector<std::string> extracted;
    for (int index = 0; index < Content::COUNT; index++)
        for (const auto &key : content.store(index).keys()) {
            const auto bytes = content.store(index).read(key);
            if (!bytes) {
                error = "Cannot read asset: " + key;
                return false;
            }
            std::ofstream output(target / pathOfUtf8(key), std::ios::binary);
            output.write((const char *)bytes->data(), (std::streamsize)bytes->size());
            if (!output) {
                error = "Cannot write asset: " + key;
                return false;
            }
            extracted.push_back(key);
        }
    for (const auto &name : requiredFiles())
        if (name != "Cube.exe" && !name.ends_with(".db")) {
            fs::copy_file(source / name, target / name);
            extracted.push_back(name);
        }
    // A completed manifest is written last. This private cache is owner data,
    // not a redistributable engine package or a substitute executable.
    std::ostringstream manifest;
    manifest << "AlphaVulkan private owner-content cache v1\nCube.exe SHA-256=" << report.executableHash << '\n';
    for (const auto &name : extracted) {
        const auto hash = fileSha256(target / pathOfUtf8(name));
        if (!hash) {
            error = "Cannot verify extracted asset: " + name;
            return false;
        }
        manifest << *hash << "  " << name << '\n';
    }
    const auto temporaryManifest = target / "OWNER-CONTENT.txt.tmp";
    std::ofstream output(temporaryManifest, std::ios::binary);
    output << manifest.str();
    output.close();
    if (!output) {
        error = "Cannot write the owner-content manifest.";
        return false;
    }
    if (!MoveFileExW(temporaryManifest.c_str(), (target / "OWNER-CONTENT.txt").c_str(), MOVEFILE_WRITE_THROUGH)) {
        error = "Cannot complete the owner-content manifest.";
        return false;
    }
    return true;
}

bool remember(const fs::path &configuration, const fs::path &installation, std::string &error) {
    std::string text;
    if (std::ifstream input(configuration, std::ios::binary); input)
        text.assign(std::istreambuf_iterator<char>(input), {});
    const std::string pathLine = "Path=\"" + utf8(fs::absolute(installation)) + "\"";
    const std::string newline = text.find("\r\n") == std::string::npos ? "\n" : "\r\n";
    std::istringstream input(text);
    std::ostringstream updated;
    bool inGame = false, foundGame = false;
    const auto trim = [](std::string value) {
        const auto first = value.find_first_not_of(" \t\r");
        return first == std::string::npos ? std::string() :
            value.substr(first, value.find_last_not_of(" \t\r") - first + 1);
    };
    for (std::string original; std::getline(input, original);) {
        if (!original.empty() && original.back() == '\r')
            original.pop_back();
        if (original.starts_with("\xEF\xBB\xBF"))
            original.erase(0, 3);
        const std::string clean = trim(original.substr(0, original.find(';')));
        if (!clean.empty() && clean.front() == '[' && clean.back() == ']') {
            inGame = clean == "[Game]";
            updated << original << newline;
            if (inGame) {
                foundGame = true;
                updated << pathLine << newline;
            }
        } else if (!(inGame && trim(clean.substr(0, clean.find('='))) == "Path"))
            updated << original << newline;
    }
    if (!foundGame)
        updated << newline << "[Game]" << newline << pathLine << newline;
    text = updated.str();
    const auto temporary = configuration.wstring() + L".tmp-" + std::to_wstring(GetCurrentProcessId()) + L"-" +
                           std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count());
    {
        std::ofstream output(temporary, std::ios::binary);
        output << text;
        output.close();
        if (!output) {
            std::error_code ignored;
            fs::remove(temporary, ignored);
            error = "Cannot write installation settings.";
            return false;
        }
    }
    if (!MoveFileExW(temporary.c_str(), configuration.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::error_code ignored;
        fs::remove(temporary, ignored);
        error = "Cannot save installation path to " + utf8(configuration);
        return false;
    }
    return true;
}
}
