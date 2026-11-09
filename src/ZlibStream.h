#pragma once
// zlib streams as Alpha makes them: compress() at its default level, read with inflate
// (alpha-saves.md §0; alpha-net-transport.md for the network).
#include <cstdint>
#include <optional>
#include <vector>

std::vector<uint8_t> zlibCompress(const uint8_t *data, size_t size);
inline std::vector<uint8_t> zlibCompress(const std::vector<uint8_t> &data) { return zlibCompress(data.data(), data.size()); }
// The whole inflated stream, or nothing if it is not a complete zlib stream. `limit` bounds the
// output (a save or packet larger than that is refused rather than filling memory).
std::optional<std::vector<uint8_t>> zlibInflate(const uint8_t *data, size_t size, size_t limit = 64u << 20);
inline std::optional<std::vector<uint8_t>> zlibInflate(const std::vector<uint8_t> &data, size_t limit = 64u << 20) {
    return zlibInflate(data.data(), data.size(), limit);
}
