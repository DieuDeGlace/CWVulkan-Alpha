// Validate/register an owned Alpha installation, optionally extracting a private
// asset cache. This tool never executes or patches the original Cube.exe.
#include "Config.h"
#include "Install.h"
#include "OwnerContent.h"
#include <cstdio>

int wmain(int argc, wchar_t **argv) {
    try {
        fs::path game;
        fs::path destination, configuration;
        for (int index = 1; index < argc; index++) {
            const std::wstring option = argv[index];
            if (option == L"--check")
                continue;
            if (option == L"--help") {
                std::printf("AlphaSetup --game DIR [--check] [--register INI] [--extract-to NEW-DIR]\n");
                return 0;
            }
            if (index + 1 >= argc || (option != L"--game" && option != L"--register" && option != L"--extract-to")) {
                std::fprintf(stderr, "Unknown or incomplete option: %ls\n", argv[index]);
                return 1;
            }
            const fs::path value = argv[++index];
            if (option == L"--game") game = value;
            else if (option == L"--register") configuration = value;
            else destination = value;
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
        std::printf("Supported Alpha executable: %s\n", report.executableHash.c_str());
        for (size_t index = 0; index < report.storeEntries.size(); index++)
            std::printf("data%zu.db: %zu verified assets\n", index + 1, report.storeEntries[index]);
        std::string error;
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
