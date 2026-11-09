#include "Audio.h"
#include <algorithm>
#include "DataStore.h"
#include <miniaudio.h>
#include <list>

bool decodeSound(const uint8_t *data, size_t size, SoundClip &out, std::string *error) {
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0); // keep channels and rate
    ma_uint64 frames = 0;
    void *pcm = nullptr;
    if (ma_decode_memory(data, size, &config, &frames, &pcm) != MA_SUCCESS) {
        if (error)
            *error = "cannot decode";
        return false;
    }
    // ma_decode_memory fills in the stored channel count and rate.
    out.channels = config.channels;
    out.sampleRate = config.sampleRate;
    const float *samples = (const float *)pcm;
    out.samples.assign(samples, samples + frames * config.channels);
    ma_free(pcm, nullptr);
    return true;
}

// One sound that is playing: a buffer over its clip's samples and the sound reading it.
struct Voice {
    std::shared_ptr<SoundClip> clip;
    ma_audio_buffer buffer;
    ma_sound sound;
};

struct Audio::State {
    ma_engine engine;
    bool running = false;
    std::list<Voice> voices; // list: a voice must not move while miniaudio points at it

    void reapFinished() {
        for (auto it = voices.begin(); it != voices.end();)
            if (ma_sound_at_end(&it->sound)) {
                ma_sound_uninit(&it->sound);
                ma_audio_buffer_uninit(&it->buffer);
                it = voices.erase(it);
            } else
                ++it;
    }
};

Audio::Audio() : state(std::make_unique<State>()) {}

Audio::~Audio() {
    if (!state->running)
        return;
    for (Voice &v : state->voices) {
        ma_sound_uninit(&v.sound);
        ma_audio_buffer_uninit(&v.buffer);
    }
    ma_engine_uninit(&state->engine);
}

bool Audio::init(const Content &store, std::string *error) {
    content = &store;
    if (ma_engine_init(nullptr, &state->engine) != MA_SUCCESS) {
        if (error)
            *error = "no audio device";
        return false;
    }
    state->running = true;
    return true;
}

bool Audio::active() const { return state->running; }

bool Audio::play(const std::string &key, float volume, float pitch) {
    if (!state->running || !content)
        return false;
    state->reapFinished();
    auto &clip = clips[key];
    if (!clip) {
        const auto bytes = content->read(key);
        auto decoded = std::make_shared<SoundClip>();
        if (!bytes || !decodeSound(bytes->data(), bytes->size(), *decoded)) {
            clips.erase(key);
            return false;
        }
        clip = decoded;
    }
    Voice &v = state->voices.emplace_back();
    v.clip = clip;
    ma_audio_buffer_config config =
        ma_audio_buffer_config_init(ma_format_f32, clip->channels, clip->frames(), clip->samples.data(), nullptr);
    config.sampleRate = clip->sampleRate;
    if (ma_audio_buffer_init(&config, &v.buffer) != MA_SUCCESS) {
        state->voices.pop_back();
        return false;
    }
    if (ma_sound_init_from_data_source(&state->engine, &v.buffer, MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr,
                                       &v.sound) != MA_SUCCESS) {
        ma_audio_buffer_uninit(&v.buffer);
        state->voices.pop_back();
        return false;
    }
    ma_sound_set_volume(&v.sound, volume);
    ma_sound_set_pitch(&v.sound, std::min(pitch, 2.f)); // audio §1.1: the rate ratio stops at 2
    ma_sound_start(&v.sound);
    return true;
}

bool Audio::busy() {
    if (!state->running)
        return false;
    state->reapFinished();
    return !state->voices.empty();
}

void Audio::setMasterVolume(float volume) {
    if (state->running)
        ma_engine_set_volume(&state->engine, volume);
}
