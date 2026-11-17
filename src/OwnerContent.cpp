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
const std::vector<ReferenceFile> &referenceFiles() {
    static const std::vector<ReferenceFile> files{
        {"Cube.exe", 3885568, ALPHA_IMAGE_SHA256},
        {"data1.db", 20941824, "0e482f6d628414db017d983f62843e5dbdc52c5b54fd478f6f0e06c786639b9d"},
        {"data2.db", 39565312, "e2231a7478d882d6382a4146ee70f1dae67662b58c8a9188fbcc294061b550fb"},
        {"data3.db", 100352, "e41dbde87bd58142d73bc22e0d3e9fd61fdb340e80f1e1e84b674edc63430438"},
        {"data4.db", 172032, "d859dd14eb855aa8b08dc5fae2062d7b722060b9204682ac5cabbc4af971599e"},
        {"gui.plx", 822138, "615106d59e5ef3af43157f3c019f7a8bdb35d407aee73e51fe1a4f9c3caa0695"},
        {"start.plx", 1534507, "be1efe6b5a1cb6b6984495994b10eb830a7f187583eaf44b7d0eae6504059c53"},
        {"help.plx", 258161, "1af05e8d119d6a268b71bfbf19b6ab428936e959727fa7a1332b0aa03295f5a5"},
        {"quest-tag.plx", 75421, "79923577ef8f6d90413ab7dbb0fb97f6c94df132a8a076b9ce1b74caa3a31161"},
        {"cursor.plx", 11609, "a82d79abd362a0b0c2117dfa75f92ab37dc133e96ba7915041ecd090ebc429b9"},
        {"resource1.dat", 19388, "f2d473a996288f7d3d0f4a4540bbb71608593bfe40965efa7b02e8c18cc45785"},
        {"resource2.dat", 15864, "1fb8573f8a39590ecc39725b365ef9c29a3f81b839e70df1e9e6d6cd6db7f173"},
        {"db.dat", 32, "c4dfd81a97b4b53fd287222cf0610c4f262dc70550b9ec2461eb2b8863ac413e"},
    };
    return files;
}

const std::vector<std::string> &requiredFiles() {
    static const std::vector<std::string> names = [] {
        std::vector<std::string> out;
        for (const auto &file : referenceFiles())
            out.push_back(file.name);
        return out;
    }();
    return names;
}

bool hasIdentifyingHeader(const std::string &name, const fs::path &file) {
    // *.plx: u32 0, 0x16, 1, 14 (little-endian), then the 14 bytes "PlasmaGraphics".
    static constexpr uint8_t PLX[30] = {0, 0, 0, 0, 0x16, 0, 0, 0, 1, 0, 0, 0, 14, 0, 0, 0,
                                        'P', 'l', 'a', 's', 'm', 'a', 'G', 'r', 'a', 'p', 'h', 'i', 'c', 's'};
    // resource*.dat: a TrueType font, sfnt version 0x00010000 (big-endian).
    static constexpr uint8_t SFNT[4] = {0, 1, 0, 0};
    const auto matches = [&](const uint8_t *expected, size_t length) {
        uint8_t head[30] = {};
        std::ifstream in(file, std::ios::binary);
        return in.read((char *)head, (std::streamsize)length) && std::memcmp(head, expected, length) == 0;
    };
    if (name.ends_with(".db"))
        return hasSqliteHeader(file);
    if (name.ends_with(".plx"))
        return matches(PLX, sizeof PLX);
    if (name.starts_with("resource") && name.ends_with(".dat"))
        return matches(SFNT, sizeof SFNT);
    std::error_code ec;
    return fs::file_size(file, ec) > 0 && !ec;
}

namespace {
// §A3: the key extension of each store's role, data1.db .. data4.db.
constexpr const char *STORE_ROLE[Content::COUNT] = {".cub", ".wav", ".png", ".xml"};
// §A5.4: the dictionary the client fetches from data4.db.
constexpr const char *DICTIONARY_KEY = "dict_en.xml";
}

ContentReport inspect(const fs::path &installation, bool fullCheck) {
    ContentReport result;
    for (const std::string &name : requiredFiles()) {
        std::error_code ec;
        const auto path = installation / pathOfUtf8(name);
        if (!fs::is_regular_file(path, ec) || fs::file_size(path, ec) == 0 || ec)
            result.problems.push_back("Missing or empty required file: " + name);
        else if (!hasIdentifyingHeader(name, path))
            result.problems.push_back("Unrecognised file header: " + name);
    }
    if (const auto hash = fileSha256(installation / "Cube.exe")) {
        result.executableHash = *hash;
        if (*hash != ALPHA_IMAGE_SHA256)
            result.problems.push_back("Cube.exe is not the supported Alpha image.");
    } else
        result.problems.push_back("Cannot read and identify Cube.exe.");
    if (!result.ready())
        return result;
    // §A7: the original client quits right after start-up when either font fails to load.
    std::string error;
    for (const char *name : {"resource1.dat", "resource2.dat"}) {
        ui::Font font;
        if (!font.load(installation / name, &error))
            result.problems.push_back(std::string(name) + ": " + error);
    }
    if (!result.ready() || !fullCheck)
        return result;

    for (const auto &reference : referenceFiles()) {
        std::error_code ec;
        const auto path = installation / pathOfUtf8(reference.name);
        const auto size = fs::file_size(path, ec);
        const auto hash = fileSha256(path);
        if (ec || size != reference.size || !hash || *hash != reference.sha256)
            result.notes.push_back(std::string(reference.name) + " differs from the reference installation.");
    }

    Content content;
    if (!content.open(installation, &error)) {
        result.problems.push_back(error);
        return result;
    }
    std::set<std::string> unique;
    for (int index = 0; index < Content::COUNT; index++) {
        const std::string store = "data" + std::to_string(index + 1) + ".db";
        const auto keys = content.store(index).keys();
        result.storeEntries[index] = keys.size();
        if (keys.empty())
            result.problems.push_back("Empty content store: " + store);
        else if (keys.size() != REFERENCE_STORE_ENTRIES[index])
            result.notes.push_back(store + " holds " + std::to_string(keys.size()) + " records; the reference holds " +
                                   std::to_string(REFERENCE_STORE_ENTRIES[index]) + ".");
        for (const auto &key : keys) {
            if (!unique.insert(key).second)
                result.problems.push_back("Duplicate content key: " + key);
            // The original client looks each kind up in its own store only.
            if (!key.ends_with(STORE_ROLE[index]))
                result.problems.push_back(store + " holds a record outside its role: " + key);
            const auto bytes = content.store(index).read(key);
            bool valid = bytes && !bytes->empty();
            if (valid && key.ends_with(".cub")) {
                CubModel model;
                valid = model.load(bytes->data(), bytes->size()) && model.complete;
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
    if (!content.store(3).raw(DICTIONARY_KEY))
        result.problems.push_back(std::string("data4.db has no ") + DICTIONARY_KEY);
    for (const char *name : {"gui.plx", "start.plx", "help.plx", "quest-tag.plx", "cursor.plx"})
        if (!ui::plasma::load(installation / name, &error))
            result.problems.push_back(std::string(name) + ": " + error);
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
