#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "engine.h"

static uint64_t nextRand = 42;

static float fmodPos(float x, float y) {
    float result = fmodf(x, y);
    return (result >= 0 ? result : result + y);
}

static float sinc(float x) {
    return x == 0 ? 1 : sinf(x) / x;
}

static void mixerRun(struct Mixer *mixer) {
    float total = 0;
    int len;
    for (len = 0; mixer->samplesIn[len] != NULL; len++) {
        total += (float) *mixer->samplesIn[len];
        printf("%d\n", len);
    }
    mixer->out = (total / len);
}

static void distRun(struct Distortion *distortion) {
    float x = (float) (*distortion->sampleIn + INT16_MAX) / (INT16_MAX - INT16_MIN);
    distortion->out = (INT16_MAX - INT16_MIN) * powf(x, *distortion->slope) / (powf(x, *distortion->slope) + powf(1 - x, *distortion->slope)) + INT16_MIN;
}

float sampleToFreq(int16_t sample) {
    return expf((float) sample * logf((float) SAMPLE_RATE / 2 - MIDDLE_C_FREQ) / INT16_MAX);
}

int16_t freqToSample(float freq) {
    return INT16_MAX * logf(freq) / (logf((float) SAMPLE_RATE / 2 - MIDDLE_C_FREQ));
}

float sampleToFloat(int16_t sample, float rangeMin, float rangeMax) {
    float floatOut = (rangeMax + rangeMin) / 2 + sample * (float) ((int) rangeMax - (int) rangeMin) / (INT16_MAX - INT16_MIN);
    // floating point is annoying
    if (floatOut < rangeMin) {
        return rangeMin;
    } else if (floatOut > rangeMax) {
        return rangeMax;
    }

    return floatOut;
}

static int16_t oscSine(float freq, uint16_t t) {
    return INT16_MAX * sinf(M_TAU * t * freq / SAMPLE_RATE);
}

static int16_t oscSquare(float freq, uint16_t t) {
    return (t < SAMPLE_RATE / freq / 2 ? INT16_MAX : INT16_MIN);
}

static int16_t oscSaw(float freq, uint16_t t) {
    float period = SAMPLE_RATE / freq;
    return INT16_MAX * (2 * fmod(t, period) / period - 1);
}

static int16_t oscTri(float freq, uint16_t t) {
    float period = SAMPLE_RATE / freq;
    return INT16_MAX * ((4.0 * fmodPos((t < period / 2 ? t : -t), period) / period) - 1);
}

void srandqd(int32_t seed) {
    nextRand = seed;
}

static int16_t randqd(void) {
    nextRand = 1664525 * nextRand + 1013904223;
    return nextRand;
}

static void oscRun(struct Oscillator *osc) {
    if (*osc->waveform == WAV_NOISE) {
        osc->out = randqd();
        return;
    }

    float freq = sampleToFreq(*osc->freqSample);
    float period = SAMPLE_RATE / freq;
    int16_t sample = INT16_MIN;
    if (osc->_internal.t > period) {
        osc->_internal.t = 0;
    }

    uint32_t tOffset = osc->_internal.t;
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
    }
    osc->_internal.t += 1;
    osc->out = sample;
}

static void ampRun(struct Amplifier *amp) {
    float sampleOut = (float) *amp->sampleIn * *amp->gain;
    if (sampleOut > INT16_MAX) {
        amp->out = INT16_MAX;
    } else if (sampleOut < INT16_MIN) {
        amp->out = INT16_MIN;
    } else {
        amp->out = sampleOut;
    }
}

static void attrRun(struct Attenuator *attr) {
    attr->out = *attr->sampleIn * (float) (*attr->amount - INT16_MIN) / (INT16_MAX - INT16_MIN);
}

static void envRun(struct Envelope *env) {
    uint32_t attackPeriod = *env->attackMs * (float) SAMPLE_RATE / 1000;
    uint32_t decayPeriod = *env->decayMs * (float) SAMPLE_RATE / 1000;
    uint32_t releasePeriod = *env->releaseMs * (float) SAMPLE_RATE / 1000;
    int16_t sample;

    int16_t sustain = *env->sustain;

    if (*env->gate == true && env->_internal.prevGate == false) {
        env->_internal.t = 0;
    }
    if (*env->gate == false && env->_internal.prevGate == true) {
        env->_internal.t = attackPeriod + decayPeriod;
        env->_internal.releaseSample = env->out;
    }

    env->_internal.prevGate = *env->gate;

    if (*env->gate == false) {
        // release case
        if (env->_internal.t >= attackPeriod + decayPeriod && env->_internal.t < attackPeriod + decayPeriod + releasePeriod) {
            sample = (int16_t) (env->_internal.releaseSample - (env->_internal.t - attackPeriod - decayPeriod) * (env->_internal.releaseSample - INT16_MIN) / releasePeriod);
            env->_internal.t += 1;
            env->out = sample;
        } else {
            env->out = INT16_MIN;
        }

        return;
    }

    if (env->_internal.t < attackPeriod) {
        sample = env->_internal.t * 2 * INT16_MAX / attackPeriod + INT16_MIN;
        env->_internal.t += 1;
    } else if (env->_internal.t < attackPeriod + decayPeriod) {
        sample = INT16_MAX - (env->_internal.t - attackPeriod) * (INT16_MAX - sustain) / decayPeriod;
        env->_internal.t += 1;
    } else {
        sample = sustain;
    } 

    env->out = sample;
}

static void rectangularWindow(float *windowBuf, int impulseLen) {
    for (int i = 0; i < impulseLen; i++) {
        windowBuf[i] = 1;
    }
}

static void hannWindow(float *windowBuf, int impulseLen) {
    for (int i = 0; i < impulseLen; i++) {
        windowBuf[i] = 0.5 * (1 - cosf(M_TAU * i / (impulseLen - 1)));
    }
}

static void hammingWindow(float *windowBuf, int impulseLen) {
    for (int i = 0; i < impulseLen; i++) {
        windowBuf[i] = 0.54 - 0.46 * cosf(M_TAU * i / (impulseLen - 1));
    }
}

static void bartlettWindow(float *windowBuf, int impulseLen) {
    for (int i = 0; i < impulseLen; i++) {
        if (i < (impulseLen - 1) / 2) {
            windowBuf[i] = 2.0 * i / (impulseLen - 1);
        } else {
            windowBuf[i] =  2.0 - 2.0 * i / (impulseLen - 1);
        }
    }
}

static void blackmanWindow(float *windowBuf, int impulseLen) {
    for (int i = 0; i < impulseLen; i++) {
        windowBuf[i] = 0.42 - 0.5 * cosf(M_TAU * i / (impulseLen - 1) + 0.08 * cosf(2 * M_TAU * i / (impulseLen - 1)));
    }
}

static void createFirWindow(float windowBuf[FILTER_BUF_SIZE], enum FirWindowType window, size_t impulseLen) {
    switch (window) {
    case WINDOW_RECTANGULAR:
        rectangularWindow(windowBuf, impulseLen);
        break;
    case WINDOW_HANN:
        hannWindow(windowBuf, impulseLen);
        break;
    case WINDOW_HAMMING:
        hammingWindow(windowBuf, impulseLen);
        break;
    case WINDOW_BARTLETT:
        bartlettWindow(windowBuf, impulseLen);
        break;
    case WINDOW_BLACKMAN:
        blackmanWindow(windowBuf, impulseLen);
        break;
    }
}

static void filterRun(struct Filter *filter) {
    if (filter->_internal.isWindowInit == false) {
        createFirWindow(filter->_internal.windowBuf, filter->window, filter->impulseLen);
        filter->_internal.isWindowInit = true;
    }
    filter->_internal.samplesBuf[filter->_internal.samplesBufIdx] = *filter->sampleIn;
    if (++filter->_internal.samplesBufIdx == filter->impulseLen) {
        filter->_internal.samplesBufIdx = 0;
    }

    if (*filter->cutoff != filter->_internal.prevCutoff) {
        float cutoffFreq = sampleToFreq(*filter->cutoff);
        float responseSum = 0;

        for (size_t i = 0; i < filter->impulseLen; i++) {
            float nextImpulse = filter->_internal.windowBuf[i] * sinc(M_TAU * cutoffFreq / SAMPLE_RATE * ((float) i - (float) (filter->impulseLen - 1) / 2));
            filter->_internal.impulseResponse[i] = nextImpulse;
            responseSum += nextImpulse;
        }

        for (size_t i = 0; i < filter->impulseLen; i++) {
            filter->_internal.impulseResponse[i] /= responseSum;
        }
    }

    float sampleOut = 0;
    int sumIdx = filter->_internal.samplesBufIdx;

    for (size_t i = 0; i < filter->impulseLen; i++) {
        if (--sumIdx < 0) {
            sumIdx = filter->impulseLen - 1;
        }
        sampleOut += (float) filter->_internal.samplesBuf[sumIdx] * filter->_internal.impulseResponse[i];
    }

    filter->_internal.prevCutoff = *filter->cutoff;
    filter->out = sampleOut;
}

void synthRun(struct Synth *synth) {
    for (int i = 0; synth->amps[i].sampleIn != NULL && i < AMPS_LEN; i++) {
        ampRun(&synth->amps[i]);
    }
    for (int i = 0; synth->oscs[i].waveform != NULL && i < OSCS_LEN; i++) {
        oscRun(&synth->oscs[i]);
    }
    for (int i = 0; synth->mixers[i].samplesIn != NULL && i < MIXERS_LEN; i++) {
        mixerRun(&synth->mixers[i]);
    }
    for (int i = 0; synth->filters[i].sampleIn != NULL && i < FILTERS_LEN; i++) {
        filterRun(&synth->filters[i]);
    }
    for (int i = 0; synth->envs[i].gate != NULL && i < ENVS_LEN; i++) {
        envRun(&synth->envs[i]);
    }
    for (int i = 0; synth->dists[i].sampleIn != NULL && i < DISTS_LEN; i++) {
        distRun(&synth->dists[i]);
    }
    for (int i = 0; synth->attrs[i].sampleIn != NULL && i < ATTRS_LEN; i++) {
        attrRun(&synth->attrs[i]);
    }
}

