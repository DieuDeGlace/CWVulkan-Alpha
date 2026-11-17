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

    // Opens the default playback device. Without one the engine runs silent. (Engine adapter:
    // Alpha shuts down when XAudio2 cannot start, audio §1.1; the engine's own backend failing
    // must not make it unusable.)
    bool init(const Content &content, std::string *error = nullptr);
    bool active() const;
    // Audio §1.2 step 6, §1.1: starts `key` (e.g. "menu-select.wav") as a new voice at the
    // voice volume (soundVolume · `volume`) · 0.01, `volume` being the distance-attenuated
    // volume a (1 for an interface sound at the listener). Nothing plays at a voice volume
    // <= 0. Its speed and pitch are scaled by `pitch`, clamped to [1/1024, 2]. Never panned.
    // False if nothing plays.
    bool play(const std::string &key, float volume = 1, float pitch = 1);
    // True while any sound is still playing.
    bool busy();
    // The integer `soundVolume` option (0 ... 100), read at every play.
    void setSoundVolume(int32_t volume) { soundVolume = volume; }

  private:
    struct State; // miniaudio objects, kept out of this header
    std::unique_ptr<State> state;
    const Content *content = nullptr;
    int32_t soundVolume = 100;
    // Sounds load on first play and stay cached by name; a name the stores lack (audio §6.4:
    // cry.wav, fireball.wav, bird3.wav) caches nothing and is looked up again next time.
    std::map<std::string, std::shared_ptr<SoundClip>> clips;
};
