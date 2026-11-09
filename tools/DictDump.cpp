// Lists dictionary entries of the owner's install whose key starts with a prefix.
// Usage: DictDump <game folder> <kind> <key prefix>
#include "DataStore.h"
#include "Dictionary.h"
#include <cstdio>
#include <string>

int main(int argc, char **argv) {
    if (argc < 4) {
        std::printf("usage: DictDump <game folder> <kind> <key prefix>\n");
        return 1;
    }
    Content content;
    std::string error;
    if (!content.open(argv[1], &error)) {
        std::printf("%s\n", error.c_str());
        return 1;
    }
    const auto bytes = content.read("dict_en.xml");
    Dictionary dictionary;
    if (!bytes || !dictionary.parse(std::string(bytes->begin(), bytes->end()), &error)) {
        std::printf("no dictionary: %s\n", error.c_str());
        return 1;
    }
    const std::string kind = argv[2], prefix = argv[3];
    for (const DictionaryEntry &e : dictionary.entries())
        if (e.kind == kind && e.key.rfind(prefix, 0) == 0)
            std::printf("%s:%s = %s\n", e.kind.c_str(), e.key.c_str(), e.text.c_str());
    return 0;
}
