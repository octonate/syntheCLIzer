#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL.h>
#include "common.h"

#define SAMPLE_RATE 44100
#define STREAM_BUF_SIZE 1024
#define MIDDLE_C_FREQ 261.63
#define FILTER_BUF_SIZE 512
#define MODULE_BUF_SIZE 16

enum {
    OSCS_LEN = 16,
    ENVS_LEN = 16,
    AMPS_LEN = 16,
    DISTS_LEN = 16,
    ATTRS_LEN = 16,
    MIXERS_LEN = 16,
    FILTERS_LEN = 16
};

enum Waveform {
    WAV_SINE,
    WAV_SQUARE,
    WAV_TRI,
    WAV_SAW,
    WAV_NOISE
};

enum FirWindowType {
    WINDOW_RECTANGULAR,
    WINDOW_HAMMING,
    WINDOW_HANN,
    WINDOW_BARTLETT,
    WINDOW_BLACKMAN,
};

struct NoteInput {
    int16_t val;
    bool gate;
};


struct Oscillator {
    int16_t *freqSample;
    int16_t *waveform;
    double *phaseOffset;

    struct {
        uint16_t t;
    } _internal;

    int16_t out;
};

struct Envelope {
    bool *gate;
    double *attackMs;
    double *decayMs;
    double *sustain;
    double *releaseMs;

    struct {
        bool prevGate;
        uint32_t t;
        int16_t releaseSample;
    } _internal;

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
    int16_t *sampleIn;
    int16_t *cutoff;
    size_t impulseLen;
    enum FirWindowType window;

    struct {
        double windowBuf[FILTER_BUF_SIZE];
        double impulseResponse[FILTER_BUF_SIZE];
        int16_t samplesBuf[FILTER_BUF_SIZE];
        size_t samplesBufIdx;
        int16_t prevCutoff;
        bool isWindowInit;
    } _internal;

    int16_t out;
};

struct Synth {
    struct NoteInput input;
    struct Oscillator oscs[OSCS_LEN];
    struct Envelope envs[ENVS_LEN];
    struct Amplifier amps[AMPS_LEN];
    struct Distortion dists[DISTS_LEN];
    struct Attenuator attrs[ATTRS_LEN];
    struct Mixer mixers[MIXERS_LEN];
    struct Filter filters[FILTERS_LEN];
    int16_t *outPtr;
    struct Scope *scope;
};

void synthRun(struct Synth *synth);

double sampleToFreq(int16_t sample);
int16_t freqToSample(double freq);
double sampleToDouble(int16_t sample, double rangeMin, double rangeMax);

void srandqd(int32_t seed);

void audioCallback(void *userdata, Uint8 *stream, int len);

#endif //ENGINE_H
