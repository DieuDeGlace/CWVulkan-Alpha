#include "ZlibStream.h"
#include <zlib.h>

std::vector<uint8_t> zlibCompress(const uint8_t *data, size_t size) {
    uLongf length = compressBound((uLong)size);
    std::vector<uint8_t> out(length);
    if (compress(out.data(), &length, data, (uLong)size) != Z_OK)
        return {};
    out.resize(length);
    return out;
}

std::optional<std::vector<uint8_t>> zlibInflate(const uint8_t *data, size_t size, size_t limit) {
    z_stream stream{};
    if (inflateInit(&stream) != Z_OK)
        return std::nullopt;
    stream.next_in = const_cast<Bytef *>(data);
    stream.avail_in = (uInt)size;
    std::vector<uint8_t> out;
    uint8_t chunk[16384];
    int result;
    do {
        stream.next_out = chunk;
        stream.avail_out = sizeof chunk;
        result = inflate(&stream, Z_NO_FLUSH);
        if (result != Z_OK && result != Z_STREAM_END)
            break;
        out.insert(out.end(), chunk, chunk + (sizeof chunk - stream.avail_out));
        if (out.size() > limit) {
            result = Z_MEM_ERROR;
            break;
        }
    } while (result != Z_STREAM_END);
    inflateEnd(&stream);
    if (result != Z_STREAM_END)
        return std::nullopt;
    return out;
}
