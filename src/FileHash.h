#pragma once
#include <filesystem>
#include <optional>
#include <string>

std::optional<std::string> fileSha256(const std::filesystem::path &file);
