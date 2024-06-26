#include "c8/audio.h"

#include "c8/c8.h"

#include <math.h>
#include <stdbool.h>
#include <SDL2/SDL_audio.h>

struct c8_audio {
    SDL_AudioDeviceID device;
    SDL_AudioSpec spec;
};

C8Audio *c8_audio_new()
{
    C8Audio *audio = calloc(1, sizeof(C8Audio));

    if (audio == NULL) {
        return NULL;
    }

    SDL_AudioSpec desired = {
        .freq = 64 * C8_TIMERS_HZ,
        .format = AUDIO_F32SYS,
        .channels = 1,
        .samples = 64
    };

    if (SDL_GetNumAudioDevices(false) > 0) {
        audio->device = SDL_OpenAudioDevice(NULL, false,
                                            &desired, &audio->spec,
                                            SDL_AUDIO_ALLOW_ANY_CHANGE);
        if (audio->device == 0) {
            fprintf(stderr, "audio: %s\n", SDL_GetError());
        }

        SDL_PauseAudioDevice(audio->device, false);
    }

    return audio;
}

void c8_audio_free(C8Audio *audio)
{
    if (audio) {
        if (audio->device > 0) {
            SDL_CloseAudioDevice(audio->device);
        }
        free(audio);
    }
}

void c8_audio_play(C8Audio *audio)
{
    if (audio->device == 0) {
        return;
    }

    SDL_AudioSpec *spec = &audio->spec;
    size_t n = spec->channels * spec->samples;

    float data[n];
    for (uint32_t i = 0; i < n; i++) {
        float time = (float)i / (n - 1);
        float x = 2.0f * M_PI * time * 64;
        data[i] = sinf(x);
    }

    if (SDL_QueueAudio(audio->device, data, n * sizeof(float)) < 0) {
        fprintf(stderr, "audio: %s\n", SDL_GetError());
    }
}
