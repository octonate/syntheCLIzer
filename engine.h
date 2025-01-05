#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>
#include <stdlib.h>

#define M_TAU 6.28318530717958647692

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
    int16_t *amt;
    float *phaseOffset;

    struct {
        uint16_t t;
    } _priv;

    int16_t out;
};

struct Envelope {
    bool *gate;
    float *attackMs;
    float *decayMs;
    float *sustain;
    float *releaseMs;

    struct {
        bool prevGate;
        uint32_t t;
        int16_t releaseSample;
    } _priv;

    int16_t out;
};

struct Amplifier {
    int16_t *sampleIn;
    float *gain;
    int16_t out;
};

struct Distortion {
    int16_t *sampleIn;
    float *slope;
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
        float windowBuf[FILTER_BUF_SIZE];
        float impulseResponse[FILTER_BUF_SIZE];
        int16_t samplesBuf[FILTER_BUF_SIZE];
        size_t samplesBufIdx;
        int16_t prevCutoff;
        bool isWindowInit;
    } _priv;

    int16_t out;
};

struct Synth {
    struct Oscillator oscs[OSCS_LEN];
    struct Envelope envs[ENVS_LEN];
    struct Amplifier amps[AMPS_LEN];
    struct Distortion dists[DISTS_LEN];
    struct Attenuator attrs[ATTRS_LEN];
    struct Mixer mixers[MIXERS_LEN];
    struct Filter filters[FILTERS_LEN];
    struct Scope *scope;
    int16_t *outPtr;
};

void synthRun(struct Synth *synth);

float sampleToFreq(int16_t sample);
int16_t freqToSample(float freq);
float sampleToFloat(int16_t sample, float rangeMin, float rangeMax);
int16_t floatToAmt(float amt);

void srandqd(int32_t seed);

#endif //ENGINE_H
