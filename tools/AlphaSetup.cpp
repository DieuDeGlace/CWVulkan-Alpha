// Validate/register an owned Alpha installation, optionally extracting a private
// asset cache. This tool never executes or patches the original Cube.exe and never
// creates or writes anything in the installation (alpha-extractor.md Part A).
#include "Config.h"
#include "Install.h"
#include "OwnerContent.h"
#include <cstdio>

int wmain(int argc, wchar_t **argv) {
    try {
        fs::path game;
        fs::path destination, configuration, saveDestination;
        for (int index = 1; index < argc; index++) {
            const std::wstring option = argv[index];
            if (option == L"--check")
                continue;
            if (option == L"--help") {
                std::printf("AlphaSetup --game DIR [--check] [--register INI] [--extract-to NEW-DIR] [--import-saves-to NEW-SAVE-DIR]\n");
                return 0;
            }
            if (index + 1 >= argc || (option != L"--game" && option != L"--register" && option != L"--extract-to" &&
                                      option != L"--import-saves-to")) {
                std::fprintf(stderr, "Unknown or incomplete option: %ls\n", argv[index]);
                return 1;
            }
            const fs::path value = argv[++index];
            if (value.empty()) {
                std::fprintf(stderr, "Option requires a path: %ls\n", option.c_str());
                return 1;
            }
            if (option == L"--game") game = value;
            else if (option == L"--register") configuration = value;
            else if (option == L"--extract-to") destination = value;
            else saveDestination = value;
        }
        Config config;
        config.load("AlphaVulkan.ini");
        const fs::path installation = game.empty() ? install::locate(config.text("Game", "Path")) : game;
        if (installation.empty()) {
            std::fprintf(stderr, "No owned Alpha installation found. Supply --game DIR.\n");
            return 1;
        }
        const auto report = owner::inspect(installation, true);
        for (const auto &problem : report.problems)
            std::fprintf(stderr, "%s\n", problem.c_str());
        if (!report.ready())
            return 1;
        for (const auto &note : report.notes)
            std::printf("Note: %s\n", note.c_str());
        std::printf("Supported Alpha executable: %s\n", report.executableHash.c_str());
        for (size_t index = 0; index < report.storeEntries.size(); index++)
            std::printf("data%zu.db: %zu verified assets\n", index + 1, report.storeEntries[index]);
        std::string error;
        if (!saveDestination.empty()) {
            if (!owner::importSaves(installation, saveDestination, error)) {
                std::fprintf(stderr, "%s\n", error.c_str());
                return 1;
            }
            std::printf("Imported single-player saves for the unmodded client into %s.\n", utf8(saveDestination).c_str());
        }
        if (!destination.empty() && !owner::extract(installation, destination, error)) {
            std::fprintf(stderr, "%s\n", error.c_str());
            return 1;
        }
        if (!configuration.empty() && !owner::remember(configuration, installation, error)) {
            std::fprintf(stderr, "%s\n", error.c_str());
            return 1;
        }
        std::printf("Owner content ready. Original installation opened read-only.\n");
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "Setup failed: %s\n", error.what());
        return 1;
    }
}
