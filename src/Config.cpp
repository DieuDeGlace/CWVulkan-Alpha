#include "Config.h"
#include <cstdlib>
#include <fstream>

static std::string trimmed(const std::string &s) {
    const auto first = s.find_first_not_of(" \t\r");
    if (first == std::string::npos)
        return {};
    return s.substr(first, s.find_last_not_of(" \t\r") - first + 1);
}

bool Config::load(const std::filesystem::path &file) {
    std::ifstream in(file);
    if (!in)
        return false;
    values.clear();
    std::string line, section;
    while (std::getline(in, line)) {
        if (line.starts_with("\xEF\xBB\xBF"))
            line.erase(0, 3);
        line = trimmed(line);
        if (line.empty() || line.front() == ';')
            continue;
        const std::string header = trimmed(line.substr(0, line.find(';')));
        if (header.front() == '[' && header.back() == ']') {
            section = header.substr(1, header.size() - 2);
            continue;
        }
        const auto eq = line.find('=');
        if (eq != std::string::npos && line.find(';') > eq) {
            std::string value = trimmed(line.substr(eq + 1));
            if (!value.empty() && value.front() == '"') {
                const auto closing = value.find('"', 1);
                value = closing == std::string::npos ? std::string() : value.substr(1, closing - 1);
            } else
                value = trimmed(value.substr(0, value.find(';')));
            values[section + "." + trimmed(line.substr(0, eq))] = value;
        }
    }
    return true;
}

std::string Config::text(const std::string &section, const std::string &key, const std::string &fallback) const {
    const auto it = values.find(section + "." + key);
    return it == values.end() || it->second.empty() ? fallback : it->second;
}

int Config::number(const std::string &section, const std::string &key, int fallback) const {
    const std::string value = text(section, key);
    return value.empty() ? fallback : std::atoi(value.c_str());
}
