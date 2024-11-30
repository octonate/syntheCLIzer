#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL.h>
#include <err.h>
#include "engine.h"

uint64_t nextRand = 42;

void synthInit(struct Synth *synth) {
    synth->modulesLen = 0;
}

static double fmodPos(double x, double y) {
    double result = fmod(x, y);
    return (result > 0 ? result : result + y);
}

static double sinc(double x) {
    return x == 0 ? 1 : sin(x) / x;
}

static void mixerRun(struct Mixer *mixer) {
    double total = 0;
    int len;
    for (len = 0; mixer->samplesIn[len] != NULL; len++) {
        total += (double) *mixer->samplesIn[len];
    }
    mixer->out = (total / len);
}

void synthAddmixer(struct Synth *synth, struct Mixer *mixer, int16_t *samplesIn[]) {
    mixer->samplesIn = samplesIn;
    mixer->out = INT16_MIN;

    synth->modules[synth->modulesLen].tag = MODULE_MIXER;
    synth->modules[synth->modulesLen].ptr.mixer = mixer;
    ++synth->modulesLen;
}

void synthAddDist(struct Synth *synth, struct Distortion *dist, int16_t *sampleIn, double *slope) {
    dist->sampleIn = sampleIn;
    dist->slope = slope;
    dist->out = INT16_MIN;

    synth->modules[synth->modulesLen].tag = MODULE_DIST;
    synth->modules[synth->modulesLen].ptr.dist = dist;
    ++synth->modulesLen;
}

static void distortionRun(struct Distortion *distortion) {
    double x = (double) (*distortion->sampleIn + INT16_MAX) / (INT16_MAX - INT16_MIN);
    distortion->out = (INT16_MAX - INT16_MIN) * pow(x, *distortion->slope) / (pow(x, *distortion->slope) + pow(1 - x, *distortion->slope)) + INT16_MIN;
}

double sampleToFreq(int16_t sample) {
    return exp((double) sample * log((double) SAMPLE_RATE / 2 - MIDDLE_C_FREQ) / INT16_MAX);
}

int16_t freqToSample(double freq) {
    return INT16_MAX * log(freq) / (log((double) SAMPLE_RATE / 2 - MIDDLE_C_FREQ));
}

double sampleToDouble(int16_t sample, double rangeMin, double rangeMax) {
    double doubleOut = (rangeMax + rangeMin) / 2 + sample * (double) ((int) rangeMax - (int) rangeMin) / (INT16_MAX - INT16_MIN);
    // floating point is annoying
    if (doubleOut < rangeMin) {
        return rangeMin;
    } else if (doubleOut > rangeMax) {
        return rangeMax;
    }

    return doubleOut;
}


static int16_t oscSine(double freq, uint16_t t) {
    return INT16_MAX * sin(M_TAU * t * freq / SAMPLE_RATE);
}

static int16_t oscSquare(double freq, uint16_t t) {
    return (t < SAMPLE_RATE / freq / 2 ? INT16_MAX : INT16_MIN);
}

static int16_t oscSaw(double freq, uint16_t t) {
    double period = SAMPLE_RATE / freq;
    return (int16_t) INT16_MAX * (2 * fmod(t, period) / period - 1);
}

static int16_t oscTri(double freq, uint16_t t) {
    double period = SAMPLE_RATE / freq;
    return (int16_t) INT16_MAX * (4 * fmod((t < period / 2 ? t : -t), period) / period - 1);
}

void srandqd(int32_t seed) {
    nextRand = seed;
}

static int16_t randqd(void) {
    nextRand = 1664525 * nextRand + 1013904223;
    return nextRand;
}

static void oscRun(struct Oscillator *osc) {
    double freq = sampleToFreq(*osc->freqSample);
    double period = SAMPLE_RATE / freq;
    int16_t sample;
    if (osc->t > period) {
        osc->t = 0;
    }

    uint16_t tOffset = osc->t;
    if (osc->phaseOffset != NULL) {
        tOffset += SAMPLE_RATE * fmodPos(*osc->phaseOffset, 360) / (360 * freq);
        tOffset = tOffset % (uint16_t) period;
    }

    switch (*osc->waveform) {
    case WAV_SINE:
        sample = oscSine(freq, tOffset);
        break;
    case WAV_SQUARE:
        sample = oscSquare(freq, tOffset);
        break;
    case WAV_TRI:
        sample = oscTri(freq, tOffset);
        break;
    case WAV_SAW:
        sample = oscSaw(freq, tOffset);
        break;
    case WAV_NOISE:
        sample = randqd();
        break;
    }
    osc->t += 1;
    osc->out = sample;
}

void synthAddOsc(struct Synth *synth, struct Oscillator *osc, int16_t *freqIn, enum Waveform *waveform, double *phaseOffset) {
    osc->freqSample = freqIn;
    osc->waveform = waveform;
    osc->phaseOffset = phaseOffset;
    osc->t = 0;
    osc->out = INT16_MIN;

    synth->modules[synth->modulesLen].tag = MODULE_OSC;
    synth->modules[synth->modulesLen].ptr.osc= osc;
    ++synth->modulesLen;
}

static void ampRun(struct Amplifier *amp) {
    //double freqOut = sampleToFreq(*amp->sampleIn) * *amp->gain;
    //double sampleOut = freqToSample(freqOut);
    double sampleOut = (double) *amp->sampleIn * *amp->gain;
    if (sampleOut > INT16_MAX) {
        amp->out = INT16_MAX;
    } else if (sampleOut < INT16_MIN) {
        amp->out = INT16_MIN;
    } else {
        amp->out = sampleOut;
    }
}

void synthAddAmp(struct Synth *synth, struct Amplifier *amp, int16_t *sampleIn, double *gain) {
    amp->sampleIn = sampleIn;
    amp->gain = gain;
    amp->out = INT16_MIN;

    synth->modules[synth->modulesLen].tag = MODULE_AMP;
    synth->modules[synth->modulesLen].ptr.amp = amp;
    ++synth->modulesLen;
}

static void attrRun(struct Attenuator *attr) {
    attr->out = *attr->sampleIn * (double) (*attr->amount - INT16_MIN) / (INT16_MAX - INT16_MIN);
}

void synthAddAttr(struct Synth *synth, struct Attenuator *attr, int16_t *sampleIn, int16_t *gainSample) {
    attr->sampleIn = sampleIn;
    attr->amount = gainSample;
    attr->out = INT16_MIN;

    synth->modules[synth->modulesLen].tag = MODULE_ATTR;
    synth->modules[synth->modulesLen].ptr.attr = attr;
    ++synth->modulesLen;
}

static void envRun(struct Envelope *env) {
    uint32_t attackPeriod = *env->attackMs * (double) SAMPLE_RATE / 1000;
    uint32_t decayPeriod = *env->decayMs * (double) SAMPLE_RATE / 1000;
    uint32_t releasePeriod = *env->releaseMs * (double) SAMPLE_RATE / 1000;
    int16_t sample;

    int16_t sustain = *env->sustain;

    if (*env->gate == true && env->prevGate == false) {
        env->t = 0;
    }
    if (*env->gate == false && env->prevGate == true) {
        env->t = attackPeriod + decayPeriod;
        env->releaseSample = env->out;
    }

    env->prevGate = *env->gate;

    if (*env->gate == false) {
        // release case
        if (env->t >= attackPeriod + decayPeriod && env->t < attackPeriod + decayPeriod + releasePeriod) {
            sample = (int16_t) (env->releaseSample - (env->t - attackPeriod - decayPeriod) * (env->releaseSample - INT16_MIN) / releasePeriod);
            env->t += 1;
            env->out = sample;
        } else {
            env->out = INT16_MIN;
        }

        return;
    }

    if (env->t < attackPeriod) {
        sample = env->t * 2 * INT16_MAX / attackPeriod + INT16_MIN;
        env->t += 1;
    } else if (env->t < attackPeriod + decayPeriod) {
        sample = INT16_MAX - (env->t - attackPeriod) * (INT16_MAX - sustain) / decayPeriod;
        env->t += 1;
    } else {
        sample = sustain;
    } 

    env->out = sample;
}

void synthAddEnv(struct Synth *synth, struct Envelope *env, bool *gate, double *attackPtr, double *decayPtr, double *sustainPtr, double *releasePtr) {
    env->gate = gate;
    env->attackMs = attackPtr;
    env->decayMs = decayPtr;
    env->sustain = sustainPtr;
    env->releaseMs = releasePtr;

    *env->gate = false;
    env->t = 0;
    env->prevGate = false;
    env->out = INT16_MIN;

    synth->modules[synth->modulesLen].tag = MODULE_ENV;
    synth->modules[synth->modulesLen].ptr.env = env;
    ++synth->modulesLen;
}

static void hannWindow(double *impulseResponse, int impulseLen) {
    for (int i = 0; i < impulseLen; i++) {
        impulseResponse[i] *= 0.5 * (1 - cos(M_TAU * i / (impulseLen - 1)));
    }
}

static void hammingWindow(double *impulseResponse, int impulseLen) {
    for (int i = 0; i < impulseLen; i++) {
        impulseResponse[i] *= 0.54 - 0.46 * cos(M_TAU * i / (impulseLen - 1));
    }
}

static void bartlettWindow(double *impulseResponse, int impulseLen) {
    for (int i = 0; i < impulseLen; i++) {
        if (i < (impulseLen - 1) / 2) {
            impulseResponse[i] *= 2.0 * i / (impulseLen - 1);
        } else {
            impulseResponse[i] *=  2.0 - 2.0 * i / (impulseLen - 1);
        }
    }
}

static void blackmanWindow(double *impulseResponse, int impulseLen) {
    for (int i = 0; i < impulseLen; i++) {
        impulseResponse[i] *= 0.42 - 0.5 * cos(M_TAU * i / (impulseLen - 1) + 0.08 * cos(2 * M_TAU * i / (impulseLen - 1)));
    }
}

void synthAddFilter(struct Synth *synth, struct Filter *filter, enum firWindowType window, int16_t *sampleIn, int16_t *cutoff, int impulseLen) {
    filter->window = window;
    filter->sampleIn = sampleIn;
    filter->cutoff = cutoff;
    filter->impulseLen = impulseLen;

    filter->samplesBufIdx = 0;

    synth->modules[synth->modulesLen].tag = MODULE_FILTER;
    synth->modules[synth->modulesLen].ptr.filter = filter;
    ++synth->modulesLen;
}

static void filterRun(struct Filter *filter) {
    filter->samplesBuf[filter->samplesBufIdx] = *filter->sampleIn;
    if (++filter->samplesBufIdx == filter->impulseLen) {
        filter->samplesBufIdx = 0;
    }

    if (*filter->cutoff != filter->prevCutoff) {
        double cutoffFreq = sampleToFreq(*filter->cutoff);
        double responseSum = 0;

        for (int i = 0; i < filter->impulseLen; i++) {
            double nextImpulse = sinc(M_TAU * cutoffFreq / SAMPLE_RATE * ((double) i - (double) (filter->impulseLen - 1) / 2));
            filter->impulseResponse[i] = nextImpulse;
            responseSum += nextImpulse;
        }

        switch (filter->window) {
        case WINDOW_RECTANGULAR:
            break;
        case WINDOW_HANN:
            hannWindow(filter->impulseResponse, filter->impulseLen);
            break;
        case WINDOW_HAMMING:
            hammingWindow(filter->impulseResponse, filter->impulseLen);
            break;
        case WINDOW_BARTLETT:
            bartlettWindow(filter->impulseResponse, filter->impulseLen);
            break;
        case WINDOW_BLACKMAN:
            blackmanWindow(filter->impulseResponse, filter->impulseLen);
            break;
        }

        for (int i = 0; i < filter->impulseLen; i++) {
            filter->impulseResponse[i] /= responseSum;
        }
    }

    double sampleOut = 0;
    int sumIdx = filter->samplesBufIdx;

    for (int i = 0; i < filter->impulseLen; i++) {
        if (--sumIdx < 0) {
            sumIdx = filter->impulseLen - 1;
        }
        sampleOut += (double) filter->samplesBuf[sumIdx] * filter->impulseResponse[i];
    }

    filter->prevCutoff = *filter->cutoff;
    filter->out = sampleOut;
}

void synthRun(struct Synth *synth) {
    for (int i = 0; i < synth->modulesLen; i++) {
        struct Module curModule = synth->modules[i];
        switch (curModule.tag) {
        case MODULE_OSC:
            oscRun(curModule.ptr.osc);
            break;
        case MODULE_ENV:
            envRun(curModule.ptr.env);
            break;
        case MODULE_AMP:
            ampRun(curModule.ptr.amp);
            break;
        case MODULE_DIST:
            distortionRun(curModule.ptr.dist);
            break;
        case MODULE_ATTR:
            attrRun(curModule.ptr.attr);
            break;
        case MODULE_MIXER:
            mixerRun(curModule.ptr.mixer);
            break;
        case MODULE_FILTER:
            filterRun(curModule.ptr.filter);
            break;
        }
    }
}

