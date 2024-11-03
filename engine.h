#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL.h>
#include "common.h"

#define SAMPLE_RATE 44100
#define STREAM_BUF_SIZE 1024
#define MIDDLE_C_FREQ 261.63

extern int keyNum;

enum Waveform {
    SINE,
    SQUARE,
    TRI,
    SAW
};

struct NoteInput {
    int16_t val;
    bool gate;
};

struct Oscillator {
    int16_t *freqSample;
    enum Waveform waveform;

    uint16_t t;
    int16_t out;
};

struct Envelope {
    bool *gate;
    double *attackMs;
    double *decayMs;
    double *sustain;
    double *releaseMs;

    bool prevGate;
    uint32_t t;
    int16_t releaseSample;
    int16_t out;
};

struct Amplifier {
    int16_t *sampleIn;
    double gain;
    int16_t out;
};

struct Attenuator {
    int16_t *sampleIn;
    int16_t *amount;
    int16_t out;
};

struct Mixer {
    int16_t **samplesIn;
    int16_t out;
};

struct Synth {
    struct NoteInput *input;
    struct Oscillator *oscs[LIST_BUF_SIZE];
    struct Envelope *envs[LIST_BUF_SIZE];
    struct Amplifier *amps[LIST_BUF_SIZE];
    struct Attenuator *attrs[LIST_BUF_SIZE];
    struct Mixer *mixers[LIST_BUF_SIZE];
    int oscsLen;
    int envsLen;
    int ampsLen;
    int attrsLen;
    int mixersLen;
    int16_t *outPtr;
};

void synthInit(struct Synth *synth);
void synthRun(struct Synth *synth);

void synthAddmixer(struct Synth *synth, struct Mixer *mixer, int16_t *samplesIn[]);
void synthAddOsc(struct Synth *synth, struct Oscillator *osc, int16_t *freqIn, enum Waveform waveform);
void synthAddAmp(struct Synth *synth, struct Amplifier *amp, int16_t *sampleIn, double gain);
void synthAddAttr(struct Synth *synth, struct Attenuator *attr, int16_t *sampleIn, int16_t *gainSample);
void synthAddEnv(struct Synth *synth, struct Envelope *env, bool *gate, double *attackPtr, double *decayPtr, double *sustainPtr, double *releasePtr);

double sampleToFreq(int16_t sample);
int16_t freqToSample(double freq);
double sampleToDouble(int16_t sample, double rangeMin, double rangeMax);

void audioCallback(void *userdata, Uint8 *stream, int len);

#endif //ENGINE_H
