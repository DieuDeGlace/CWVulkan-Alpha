#pragma once
#include <filesystem>
#include <optional>
#include <cstdint>
#include <span>
#include <string>

std::optional<std::string> fileSha256(const std::filesystem::path &file);
std::optional<std::string> bytesSha256(std::span<const uint8_t> bytes);
