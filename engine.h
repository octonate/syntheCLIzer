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

enum EnvStage {
    STAGE_Attack,
    STAGE_Decay,
    STAGE_Sustain,
    STAGE_Release,
};

struct NoteInput {
    int16_t val;
    bool gate;
};


struct Oscillator {
    int16_t *freqSample;
    int16_t *amt;
    int16_t *waveform;
    float *phaseOffset;

    struct {
        uint16_t t;
    } _priv;
};

struct EnvelopeAdsr {
    bool *gate;
    float *attackMs;
    float *decayMs;
    float *sustain;
    float *releaseMs;

    struct {
        bool prevGate;
        uint32_t t;
        int16_t releaseSample;
        int16_t prevOut;
    } _priv;
};

struct EnvelopeAd {
    bool *gate;
    float *attackMs;
    float *decayMs;

    struct {
        bool prevGate;
        uint32_t t;
    } _priv;
};

struct Amplifier {
    int16_t *sampleIn;
    float *gain;
};

struct Distortion {
    int16_t *sampleIn;
    float *slope;
};

struct Attenuator {
    int16_t *sampleIn;
    int16_t *amount;
};

struct Mixer {
    int16_t **samplesIn;
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
    } _priv;
};

//struct Synth {
//    struct Oscillator oscs[OSCS_LEN];
//    struct Envelope envs[ENVS_LEN];
//    struct Amplifier amps[AMPS_LEN];
//    struct Distortion dists[DISTS_LEN];
//    struct Attenuator attrs[ATTRS_LEN];
//    struct Mixer mixers[MIXERS_LEN];
//    struct Filter filters[FILTERS_LEN];
//    struct Scope *scope;
//    struct {
//        bool isInit;
//    } _priv;
//    int16_t *outPtr;
//};

enum SynthModuleType {
    MODULE_Oscillator,
    MODULE_EnvelopeAdsr,
    MODULE_EnvelopeAd,
    MODULE_Amplifier,
    MODULE_Distortion,
    MODULE_Attenuator,
    MODULE_Mixer,
    MODULE_Filter,
};

struct SynthModule {
    void *ptr;
    enum SynthModuleType tag;
    int16_t out;
};

struct Synth {
    struct SynthModule *modules;
    size_t modulesLen;
    struct {
        bool isInit;
    } _priv;
    int16_t *outPtr;
};

void synthRun(struct Synth *synth);
void modulesRun(struct Synth2 *synth);

float sampleToFreq(int16_t sample);
int16_t freqToSample(float freq);
float sampleToFloat(int16_t sample, float rangeMin, float rangeMax);
int16_t floatToAmt(float amt);


typedef struct {
    float real;
    float imag;
} Cplx;


void sftTest(void);
void slowFourierTransform(int16_t *sampleBuf, Cplx *outBuf, size_t bufLen);
void cplxPrint(Cplx z);

void srandqd(int32_t seed);

#endif //ENGINE_H
