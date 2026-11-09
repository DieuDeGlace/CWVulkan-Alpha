#pragma once
// Alpha's game settings and options.cfg (docs/specs/alpha-ui-screens.md §6.6): twelve integers,
// read as "key value" tokens and written back in Alpha's own order.
#include <array>
#include <cstdint>
#include <filesystem>

struct GameOptions {
    enum Field {
        FULLSCREEN, RESOLUTION_X, RESOLUTION_Y, ANTI_ALIASING, RENDER_DISTANCE, SOUND_VOLUME, MUSIC_VOLUME,
        CAMERA_SPEED, CAMERA_SMOOTHNESS, INVERT_Y, LANGUAGE, MIN_TIME_STEP, COUNT
    };
    // §6.6: the defaults (the resolution is the desktop's).
    std::array<int32_t, COUNT> values{0, 1920, 1080, 0, 50, 100, 100, 50, 80, 0, 0, 9};

    int32_t &operator[](Field f) { return values[f]; }
    int32_t operator[](Field f) const { return values[f]; }
    bool operator==(const GameOptions &o) const { return values == o.values; }

    // Reads the file over the current values; a missing file or key leaves them.
    void load(const std::filesystem::path &file);
    // Rewrites the file, twelve "key value" lines (language before invertY).
    bool save(const std::filesystem::path &file) const;
};
