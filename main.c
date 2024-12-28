#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL.h>
#include "engine.h"
#include "tui.h"


#define NULL_TERM_ARR(type, ...) (type[]) {__VA_ARGS__, NULL}

#define PTR(x) &(int16_t){x}


SDL_AudioDeviceID audioDevice;
SDL_AudioSpec audioSpec;


struct Userdata {
    struct Synth *synth;
};


void audioCallback(void *userdata, uint8_t *stream, int len) {
    struct Userdata *data = (struct Userdata *)userdata;
    struct Synth *synth = data->synth;

    int16_t *stream16 = (int16_t *)stream;

    for (size_t i = 0; i < len / sizeof(int16_t); i++) {
        //switch (data->curChar) {
        //case '\0':
        //    break;
        //case '[':
        //    synth->input->gate = true;
        //    break;
        //case ']':
        //    synth->input->gate = false;
        //    break;
        //case 'q':
        //    data->quit = true;
        //    break;
        //case 'k':
        //    boxIncrFocElement(tui->boxes[tui->focBoxIdx]);
        //    break;
        //case 'j':
        //    boxDecrFocElement(tui->boxes[tui->focBoxIdx]);
        //    break;
        //case 'h':
        //    boxPrevElement(tui->boxes[tui->focBoxIdx]);
        //    break;
        //case 'l':
        //    boxNextElement(tui->boxes[tui->focBoxIdx]);
        //    break;
        //case 'H':
        //    tuiPrevBox(tui);
        //    break;
        //case 'L':
        //    tuiNextBox(tui);
        //    break;
        //default:
        //    synth->input->gate = true;
        //    synth->input->val = freqToSample(100 * pow(2, (double) (data->curChar - 48) / 12));
        //}
        //
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
            .waveform = PTR(WAV_NOISE),
        },
        .mixers[0].samplesIn = NULL_TERM_ARR(int16_t*,
            &synth.oscs[0].out,
            &synth.oscs[1].out,
            &synth.oscs[2].out
        ),
        .filters[0] = {
            .sampleIn = &synth.mixers[0].out,
            .cutoff = PTR(freqToSample(800.0)),
            .impulseLen = 96,
            .window = WINDOW_BLACKMAN,
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

    SDL_CloseAudioDevice(audioDevice);
    SDL_Quit();

    resetTerm();

    return 0;
}
