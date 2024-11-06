#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL.h>
#include "engine.h"
#include "tui.h"

SDL_AudioDeviceID audioDevice;
SDL_AudioSpec audioSpec;

void goodbye() {
    SDL_CloseAudioDevice(audioDevice);
    SDL_Quit();

    resetTerm();
}

int main() {
    system("clear");
    hello();

    struct Box box;
    boxInit(&box, 10, 10, 16, 7, "osc1");

    struct Slider sliderA, sliderD, sliderS, sliderR;
    boxAddSlider(&box, &sliderA, 1, 1, 4, 0, 1000, 'a');
    boxAddSlider(&box, &sliderD, 3, 1, 4, 0, 1000, 'd');
    boxAddSlider(&box, &sliderS, 5, 1, 4, INT16_MIN, INT16_MAX, 's');
    boxAddSlider(&box, &sliderR, 7, 1, 4, 0, 2000, 'r');

    struct Radios shape;
    boxAddRadios(&box, &shape, 10, 1, "shape");
    radiosAddButton(&shape, "sin", SINE);
    radiosAddButton(&shape, "sqr", SQUARE);
    radiosAddButton(&shape, "tri", TRI);
    radiosAddButton(&shape, "saw", SAW);


    struct Synth synth;
    synthInit(&synth);

    struct NoteInput input1 = {
        .gate = 0,
        .val = 0,
    };

    struct Amplifier octaveUp, octaveDown;
    synthAddAmp(&synth, &octaveUp, &input1.val, 2);
    synthAddAmp(&synth, &octaveDown, &input1.val, 0.5);

    struct Oscillator osc1, osc2, osc3;
    synthAddOsc(&synth, &osc1, &octaveDown.out, &(enum Waveform){SAW});
    synthAddOsc(&synth, &osc2, &octaveUp.out, &(enum Waveform){SINE});
    synthAddOsc(&synth, &osc3, &input1.val, (enum Waveform *)&shape.val);


    struct Mixer mixer;
    synthAddmixer(&synth, &mixer, (int16_t *[]) {&osc1.out, &osc2.out, &osc3.out, NULL});

    struct Envelope env1;
    synthAddEnv(&synth, &env1, &input1.gate, &sliderA.val, &sliderD.val, &sliderS.val, &sliderR.val);

    struct Attenuator attr;
    synthAddAttr(&synth, &attr, &mixer.out, &env1.out);

    synth.input = &input1;

    synth.outPtr = &attr.out;


    bool quit = false;
    int curKey;

    SDL_Init(SDL_INIT_AUDIO);

    SDL_zero(audioSpec);
    audioSpec.freq = SAMPLE_RATE;
    audioSpec.format = AUDIO_S16SYS;
    audioSpec.channels = 1;
    audioSpec.samples = STREAM_BUF_SIZE;
    audioSpec.callback = &audioCallback;
    audioSpec.userdata = &synth;


    audioDevice = SDL_OpenAudioDevice(NULL, 0, &audioSpec, NULL, 0);
    SDL_PauseAudioDevice(audioDevice, 0);

    while (!quit) {
        curKey = getchar();
        switch (curKey) {
        case '[':
            input1.gate = true;
            break;
        case ']':
            input1.gate = false;
            break;
        case 'q':
            quit = true;
            break;
        case 'k':
            elementIncr(box.elements[box.focElementIdx]);
            break;
        case 'j':
            elementDecr(box.elements[box.focElementIdx]);
            break;
        case 'h':
            boxPrevElement(&box);
            break;
        case 'l':
            boxNextElement(&box);
            break;
        default:
            input1.gate = true;
            input1.val = freqToSample(200 * pow(2, (double) (curKey - 48) / 12));
        }

        boxDraw(&box);

    }

    SDL_Delay(50);

    goodbye();

    return 0;
}
