#include "FileHash.h"
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <cstdint>
#include <fstream>
#include <vector>

namespace {
struct HashState {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    std::vector<uint8_t> object;
    ~HashState() {
        if (hash)
            BCryptDestroyHash(hash);
        if (algorithm)
            BCryptCloseAlgorithmProvider(algorithm, 0);
    }
};
}

std::optional<std::string> fileSha256(const std::filesystem::path &file) {
    std::ifstream input(file, std::ios::binary);
    if (!input)
        return std::nullopt;
    HashState state;
    if (BCryptOpenAlgorithmProvider(&state.algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
        return std::nullopt;
    DWORD objectBytes = 0, returned = 0;
    if (BCryptGetProperty(state.algorithm, BCRYPT_OBJECT_LENGTH, (PUCHAR)&objectBytes, sizeof objectBytes, &returned, 0) < 0)
        return std::nullopt;
    state.object.resize(objectBytes);
    if (BCryptCreateHash(state.algorithm, &state.hash, state.object.data(), objectBytes, nullptr, 0, 0) < 0)
        return std::nullopt;
    std::array<char, 65536> block{};
    while (input) {
        input.read(block.data(), block.size());
        const auto bytes = input.gcount();
        if (bytes > 0 && BCryptHashData(state.hash, (PUCHAR)block.data(), (ULONG)bytes, 0) < 0)
            return std::nullopt;
    }
    if (!input.eof())
        return std::nullopt;
    std::array<uint8_t, 32> digest{};
    if (BCryptFinishHash(state.hash, digest.data(), (ULONG)digest.size(), 0) < 0)
        return std::nullopt;
    constexpr char hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(digest.size() * 2);
    for (uint8_t byte : digest) {
        result += hex[byte >> 4];
        result += hex[byte & 15];
    }
    return result;
}
