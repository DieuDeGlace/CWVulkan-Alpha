#pragma once
// Sound playback for the WAV files in data2.db (alpha-data-stores.md §1), through miniaudio.
// Each sound is decoded once, on first use, and kept in memory; plays share that copy.
// Which sound the game plays when, and how loud, comes with the gameplay specs.
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

class Content;

// One decoded sound: interleaved 32-bit float samples.
struct SoundClip {
    uint32_t channels = 0, sampleRate = 0;
    std::vector<float> samples;
    uint64_t frames() const { return channels ? samples.size() / channels : 0; }
    double seconds() const { return sampleRate ? (double)frames() / sampleRate : 0; }
};

// Decodes a WAV (or any format miniaudio reads) from memory. False if it cannot be read.
bool decodeSound(const uint8_t *data, size_t size, SoundClip &out, std::string *error = nullptr);

class Audio {
  public:
    Audio();
    ~Audio();
    Audio(const Audio &) = delete;
    Audio &operator=(const Audio &) = delete;

    // Opens the default playback device. Without one the engine runs silent.
    bool init(const Content &content, std::string *error = nullptr);
    bool active() const;
    // Starts `key` (e.g. "menu-select.wav") at `volume` (1 = as stored), its speed and pitch
    // scaled by `pitch`. False if unknown.
    bool play(const std::string &key, float volume = 1, float pitch = 1);
    // True while any sound is still playing.
    bool busy();
    void setMasterVolume(float volume);

  private:
    struct State; // miniaudio objects, kept out of this header
    std::unique_ptr<State> state;
    const Content *content = nullptr;
    std::map<std::string, std::shared_ptr<SoundClip>> clips;
};
