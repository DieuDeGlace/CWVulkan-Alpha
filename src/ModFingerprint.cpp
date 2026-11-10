#include "ModFingerprint.h"
#include "FileHash.h"
#include "Install.h"
#include <algorithm>
#include <utility>
#include <vector>

std::optional<std::string> modFingerprint(const std::filesystem::path &folder, const std::string &plugin,
                                         std::string *error) {
    std::vector<std::pair<std::string, std::filesystem::path>> files{{"mod.ini", folder / "mod.ini"}};
    if (!plugin.empty())
        files.emplace_back("plugin/" + plugin, folder / pathOfUtf8(plugin));
    std::error_code ec;
    const auto content = folder / "content";
    if (std::filesystem::is_directory(content, ec)) {
        std::filesystem::directory_iterator entry(content, ec), end;
        while (!ec && entry != end) {
            if (entry->is_regular_file(ec))
                files.emplace_back("content/" + utf8(entry->path().filename()), entry->path());
            if (!ec)
                entry.increment(ec);
        }
    } else if (ec == std::errc::no_such_file_or_directory) {
        ec.clear(); // no content override is a normal DLL-only mod
    }
    if (ec) {
        if (error)
            *error = "cannot enumerate mod content for its compatibility fingerprint";
        return std::nullopt;
    }
    std::sort(files.begin(), files.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
    std::string manifest = "AlphaVulkan mod inputs 1\n";
    for (const auto &[name, path] : files) {
        const auto hash = fileSha256(path);
        if (!hash) {
            if (error)
                *error = "cannot fingerprint mod input: " + name;
            return std::nullopt;
        }
        manifest += std::to_string(name.size()) + ':' + name + *hash + '\n';
    }
    const auto result = bytesSha256({reinterpret_cast<const uint8_t *>(manifest.data()), manifest.size()});
    if (!result && error)
        *error = "cannot compute the mod compatibility fingerprint";
    return result;
}
