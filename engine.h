#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL.h>
#include "common.h"

#define SAMPLE_RATE 44100
#define STREAM_BUF_SIZE 1024
#define MIDDLE_C_FREQ 261.63

enum ModuleType {
    MODULE_OSC,
    MODULE_ENV,
    MODULE_AMP,
    MODULE_DIST,
    MODULE_ATTR,
    MODULE_MIXER,
    MODULE_FILTER,
};

enum Waveform {
    WAV_SINE,
    WAV_SQUARE,
    WAV_TRI,
    WAV_SAW,
    WAV_NOISE
};

struct NoteInput {
    int16_t val;
    bool gate;
};


struct Oscillator {
    int16_t *freqSample;
    enum Waveform *waveform;
    double *phaseOffset;

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
    double *gain;
    int16_t out;
};

struct Distortion {
    int16_t *sampleIn;
    double *slope;
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

struct Filter {
    int16_t samplesBuf[LIST_BUF_SIZE];
    int16_t *sampleIn;
    double *impulseResponse;
    int filterLen;
    int samplesBufIdx;
    int16_t out;
};

struct Module {
    enum ModuleType tag;
    union {
        struct Oscillator *osc;
        struct Envelope *env;
        struct Amplifier *amp;
        struct Distortion *dist;
        struct Attenuator *attr;
        struct Mixer *mixer;
        struct Filter *filter;
    } ptr;
};

struct Synth {
    struct NoteInput *input;
    struct Module modules[LIST_BUF_SIZE];
    int modulesLen;
    int16_t *outPtr;
    struct Scope *scope;
};

void synthInit(struct Synth *synth);
void synthRun(struct Synth *synth);

void synthAddmixer(struct Synth *synth, struct Mixer *mixer, int16_t *samplesIn[]);
void synthAddOsc(struct Synth *synth, struct Oscillator *osc, int16_t *freqIn, enum Waveform *waveform, double *phaseOffset);
void synthAddAmp(struct Synth *synth, struct Amplifier *amp, int16_t *sampleIn, double *gain);
void synthAddAttr(struct Synth *synth, struct Attenuator *attr, int16_t *sampleIn, int16_t *gainSample);
void synthAddEnv(struct Synth *synth, struct Envelope *env, bool *gate, double *attackPtr, double *decayPtr, double *sustainPtr, double *releasePtr);
void synthAddDist(struct Synth *synth, struct Distortion *dist, int16_t *sampleIn, double *slope);
void synthAddFilter(struct Synth *synth, struct Filter *filter, int16_t *sampleIn, double *impulseResponse, int filterLen);

double sampleToFreq(int16_t sample);
int16_t freqToSample(double freq);
double sampleToDouble(int16_t sample, double rangeMin, double rangeMax);

void srandqd(int32_t seed);

void audioCallback(void *userdata, Uint8 *stream, int len);

#endif //ENGINE_H
