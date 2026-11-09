#pragma once
// AlphaVulkan.ini: `[Section]` / `Key=Value`; `;` comments, or quoted literal values.
#include <filesystem>
#include <map>
#include <string>

class Config {
  public:
    bool load(const std::filesystem::path &file);
    std::string text(const std::string &section, const std::string &key, const std::string &fallback = {}) const;
    int number(const std::string &section, const std::string &key, int fallback) const;

  private:
    std::map<std::string, std::string> values; // "section.key" -> value
};
