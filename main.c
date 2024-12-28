#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL.h>
#include "engine.h"
#include "tui.h"


#define PTR(x) _Generic((x), \
    int16_t: &(int16_t){x}, \
    int: &(int16_t){x}, \
    double: &(double){x}, \
    default: &(int16_t){x})

#define NULL_TERM_ARR(type, ...) (type[]) {__VA_ARGS__, NULL}

SDL_AudioDeviceID audioDevice;
SDL_AudioSpec audioSpec;


void goodbye(void) {
    SDL_CloseAudioDevice(audioDevice);
    SDL_Quit();

    resetTerm();
}

struct Userdata {
    struct Synth *synth;
};


void audioCallback(void *userdata, uint8_t *stream, int len) {
    struct Userdata *data = (struct Userdata *)userdata;
    struct Synth *synth = data->synth;

    int16_t *stream16 = (int16_t *)stream;

    for (unsigned i = 0; i < len / sizeof (int16_t); i++) {
        synthRun(synth);
        stream16[i] = *synth->outPtr;
    }
}

int main(void) {
    system("clear");
    termInit();
    srandqd(42);

    struct Synth synth = {
        .oscs[0] = {
            .waveform = PTR(WAV_SAW),
            .freqSample = PTR(freqToSample(300)),
        },
        .oscs[1] = {
            .waveform = PTR(WAV_SQUARE),
            .freqSample = PTR(freqToSample(400)),
        },
        .oscs[2] = {
            .waveform = PTR(WAV_SINE),
            .freqSample = PTR(freqToSample(1)),
        },
        .mixers[0].samplesIn = NULL_TERM_ARR(int16_t*,
            &synth.oscs[0].out,
            &synth.oscs[1].out
        ),
        .filters[0] = {
            .sampleIn = &synth.mixers[0].out,
            .cutoff = PTR(freqToSample(100.0)),
            .impulseLen = 128,
            .window = WINDOW_HAMMING,
        },
        .outPtr = &synth.filters[0].out,
    };

    struct Userdata callbackData;
    callbackData.synth = &synth;

    SDL_Init(SDL_INIT_AUDIO);

    SDL_zero(audioSpec);
    audioSpec.freq = SAMPLE_RATE;
    audioSpec.format = AUDIO_S16SYS;
    audioSpec.channels = 1;
    audioSpec.samples = STREAM_BUF_SIZE;
    audioSpec.callback = &audioCallback;
    audioSpec.userdata = &callbackData;


    audioDevice = SDL_OpenAudioDevice(NULL, 0, &audioSpec, NULL, 0);
    SDL_PauseAudioDevice(audioDevice, 0);


    while (getchar() != 'q');

    SDL_Delay(50);

    goodbye();

    return 0;
}
