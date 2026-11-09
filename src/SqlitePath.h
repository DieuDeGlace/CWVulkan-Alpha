#pragma once
#include "Install.h"

// SQLite URI filenames must quote reserved bytes, including literal percent
// signs in real filenames. Shared by both immutable owner-data/save readers.
inline std::string sqliteReadOnlyUri(const fs::path &file) {
    constexpr char HEX[] = "0123456789ABCDEF";
    std::string uri = "file:///";
    for (unsigned char byte : utf8(fs::absolute(file))) {
        if (byte == '\\')
            uri += '/';
        else if (byte == '%' || byte == '?' || byte == '#' || byte == ' ' || byte >= 0x80)
            uri += {'%', HEX[byte >> 4], HEX[byte & 15]};
        else
            uri += (char)byte;
    }
    return uri + "?immutable=1";
}
