#include "FileHash.h"
#include <windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <vector>

namespace {
struct HashState {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    std::vector<uint8_t> object;
    bool open() {
        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
            return false;
        DWORD objectBytes = 0, returned = 0;
        if (BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, (PUCHAR)&objectBytes, sizeof objectBytes, &returned, 0) < 0)
            return false;
        object.resize(objectBytes);
        return BCryptCreateHash(algorithm, &hash, object.data(), objectBytes, nullptr, 0, 0) >= 0;
    }
    bool append(std::span<const uint8_t> bytes) {
        // BCrypt accepts a four-byte length; keep the buffer API valid on x64 too.
        while (!bytes.empty()) {
            const auto count = (ULONG)std::min<size_t>(bytes.size(), 65536);
            if (BCryptHashData(hash, (PUCHAR)bytes.data(), count, 0) < 0)
                return false;
            bytes = bytes.subspan(count);
        }
        return true;
    }
    std::optional<std::string> finish() {
        std::array<uint8_t, 32> digest{};
        if (BCryptFinishHash(hash, digest.data(), (ULONG)digest.size(), 0) < 0)
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
    if (!state.open())
        return std::nullopt;
    std::array<char, 65536> block{};
    while (input) {
        input.read(block.data(), block.size());
        const auto bytes = input.gcount();
        if (bytes > 0 && !state.append({reinterpret_cast<const uint8_t *>(block.data()), size_t(bytes)}))
            return std::nullopt;
    }
    if (!input.eof())
        return std::nullopt;
    return state.finish();
}

std::optional<std::string> bytesSha256(std::span<const uint8_t> bytes) {
    HashState state;
    if (!state.open() || !state.append(bytes))
        return std::nullopt;
    return state.finish();
}
