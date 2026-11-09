// Alpha's options.cfg (docs/specs/alpha-ui-screens.md §6.6).
#include "GameOptions.h"
#include <cstdlib>
#include <fstream>
#include <string>

namespace {

const char *const KEYS[GameOptions::COUNT] = {
    "fullscreen", "resolutionX", "resolutionY", "antiAliasing", "renderDistance", "soundVolume",
    "musicVolume", "cameraSpeed", "cameraSmoothness", "invertY", "language", "minTimeStep",
};

} // namespace

void GameOptions::load(const std::filesystem::path &file) {
    std::ifstream in(file);
    std::string token;
    while (in >> token) {
        for (int f = 0; f < COUNT; f++)
            if (token == KEYS[f]) {
                std::string number;
                if (!(in >> number))
                    return;
                char *end = nullptr;
                const long v = std::strtol(number.c_str(), &end, 10);
                if (end == number.c_str() || *end)
                    return; // reading stops at the first token that is not a number
                values[f] = (int32_t)v;
            }
    }
}

bool GameOptions::save(const std::filesystem::path &file) const {
    std::ofstream out(file); // text mode: CR LF on Windows, as Alpha writes it
    if (!out)
        return false;
    for (Field f : {FULLSCREEN, RESOLUTION_X, RESOLUTION_Y, ANTI_ALIASING, RENDER_DISTANCE, SOUND_VOLUME, MUSIC_VOLUME,
                    CAMERA_SPEED, CAMERA_SMOOTHNESS, LANGUAGE, INVERT_Y, MIN_TIME_STEP})
        out << KEYS[f] << ' ' << values[f] << '\n';
    return (bool)out;
}
