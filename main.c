#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL.h>
#include "engine.h"
#include "tui.h"
#include "callback.h"

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

    struct Tui tui;
    tuiInit(&tui, "3xOsc");

    struct Box oscBox1, oscBox2, oscBox3, env;
    tuiAddBox(&tui, &oscBox1, 10, 10, 16, 7, "osc1", OUTLINE_THIN);
    tuiAddBox(&tui, &oscBox2, 10, 20, 16, 7, "osc2", OUTLINE_THIN);
    tuiAddBox(&tui, &oscBox3, 10, 30, 16, 7, "osc3", OUTLINE_THIN);
    tuiAddBox(&tui, &env, 30, 10, 9, 7, "env", OUTLINE_THIN);

    struct Radios shape1, shape2, shape3;
    boxAddRadios(&oscBox1, &shape1, 1, 1, "shape");
    boxAddRadios(&oscBox2, &shape2, 1, 1, "shape");
    boxAddRadios(&oscBox3, &shape3, 1, 1, "shape");

    radiosAddButton(&shape1, "sin", WAV_SINE);
    radiosAddButton(&shape1, "sqr", WAV_SQUARE);
    radiosAddButton(&shape1, "tri", WAV_TRI);
    radiosAddButton(&shape1, "saw", WAV_SAW);
    radiosAddButton(&shape2, "sin", WAV_SINE);
    radiosAddButton(&shape2, "sqr", WAV_SQUARE);
    radiosAddButton(&shape2, "tri", WAV_TRI);
    radiosAddButton(&shape2, "saw", WAV_SAW);
    radiosAddButton(&shape3, "sin", WAV_SINE);
    radiosAddButton(&shape3, "sqr", WAV_SQUARE);
    radiosAddButton(&shape3, "tri", WAV_TRI);
    radiosAddButton(&shape3, "saw", WAV_SAW);

    struct Slider detune1, detune2, detune3;
    boxAddSlider(&oscBox1, &detune1, 7, 1, 4, 0.9, 1.1, 'T');
    boxAddSlider(&oscBox2, &detune2, 7, 1, 4, 0.9, 1.1, 'T');
    boxAddSlider(&oscBox3, &detune3, 7, 1, 4, 0.9, 1.1, 'T');

    struct Slider phase1, phase2, phase3;
    boxAddSlider(&oscBox1, &phase1, 9, 1, 4, -180, 180, 'P');
    boxAddSlider(&oscBox2, &phase2, 9, 1, 4, -180, 180, 'P');
    boxAddSlider(&oscBox3, &phase3, 9, 1, 4, -180, 180, 'P');

    struct Slider attack, decay, sustain, release, drive;
    boxAddSlider(&env, &attack, 1, 1, 4, 0, 1000, 'a');
    boxAddSlider(&env, &decay, 3, 1, 4, 0, 1000, 'd');
    boxAddSlider(&env, &sustain, 5, 1, 4, INT16_MIN, INT16_MAX, 's');
    boxAddSlider(&env, &release, 7, 1, 4, 0, 2000, 'r');

    boxAddSlider(&env, &drive, 9, 1, 4, 0, 2, 'G');


    struct NoteInput input1 = {
        .gate = 0,
        .val = 0,
    };

    struct Synth synth;
    synthInit(&synth);

    struct Amplifier detuner1, detuner2, detuner3;
    synthAddAmp(&synth, &detuner1, &input1.val, &detune1.val);
    synthAddAmp(&synth, &detuner2, &input1.val, &detune2.val);
    synthAddAmp(&synth, &detuner3, &input1.val, &detune3.val);

    struct Oscillator osc1, osc2, osc3;
    synthAddOsc(&synth, &osc1, &detuner1.out, (enum Waveform *)&shape1.val, &phase1.val);
    synthAddOsc(&synth, &osc2, &detuner2.out, (enum Waveform *)&shape2.val, &phase2.val);
    synthAddOsc(&synth, &osc3, &detuner3.out, (enum Waveform *)&shape3.val, &phase3.val);

    struct Mixer mixer;
    synthAddmixer(&synth, &mixer, (int16_t *[]) {&osc1.out, &osc2.out, &osc3.out, NULL});
    //synthAddmixer(&synth, &mixer, (int16_t *[]) {&osc1.out, &osc2.out, NULL});

    struct Distortion distortion;
    synthAddDist(&synth, &distortion, &mixer.out, &drive.val);

    struct Envelope env1;
    synthAddEnv(&synth, &env1, &input1.gate, &attack.val, &decay.val, &sustain.val, &release.val);

    struct Attenuator attr;
    synthAddAttr(&synth, &attr, &distortion.out, &env1.out);

    synth.input = &input1;

    synth.outPtr = &attr.out;

    struct Box triggerBox;
    tuiAddBox(&tui, &triggerBox, 45, 20, 3, 7, "trig", OUTLINE_DOUBLE);
    struct Slider trigSlider;
    boxAddSlider(&triggerBox, &trigSlider, 1, 1, 4, 0, 10000, 'T');

    struct Scope scope;
    tuiAddScope(&scope, synth.outPtr, 50, 10, 100, 50, 4, &trigSlider.val, TRIG_RISING_EDGE);
    synth.scope = &scope;

    struct Userdata callbackData;
    callbackData.synth = &synth;
    callbackData.tui = &tui;


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


    while ((callbackData.curChar = getchar()) != 'q') {}

    SDL_Delay(50);

    goodbye();

    return 0;
}
