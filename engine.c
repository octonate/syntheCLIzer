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
    int32_t total = 0;
    size_t len;
    for (len = 0; mixer->samplesIn[len] != NULL; len++) {
        total += *mixer->samplesIn[len];
    }
    if (total > INT16_MAX) total = INT16_MAX;
    if (total < INT16_MIN) total = INT16_MIN;
    mixer->out = total;
}

static void distRun(struct Distortion *distortion) {
    float x = (float) (*distortion->sampleIn + INT16_MAX) / (INT16_MAX - INT16_MIN);
    distortion->out = (INT16_MAX - INT16_MIN) * powf(x, *distortion->slope) / (powf(x, *distortion->slope) + powf(1 - x, *distortion->slope)) + INT16_MIN;
}

float sampleToFreq(int16_t sample) {
    return expf(sample * logf(SAMPLE_RATE / 2.0f - MIDDLE_C_FREQ) / INT16_MAX);
}

int16_t freqToSample(float freq) {
    return INT16_MAX * logf(freq) / (logf(SAMPLE_RATE / 2.0f - MIDDLE_C_FREQ));
}

float sampleToFloat(int16_t sample, float rangeMin, float rangeMax) {
    float floatOut = (rangeMax + rangeMin) / 2 + sample * (rangeMax - rangeMin) / (INT16_MAX - INT16_MIN);
    // floating point is annoying
    if (floatOut < rangeMin) {
        return rangeMin;
    } else if (floatOut > rangeMax) {
        return rangeMax;
    }

    return floatOut;
}

int16_t floatToAmt(float amt) {
    if (amt >= 1) return INT16_MAX;
    if (amt <= 0) return INT16_MIN;

    return amt * (INT16_MAX - INT16_MIN) + INT16_MIN;
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
    int16_t amplitude = (*osc->amt - INT16_MIN) / 2;

    if (osc->_priv.t > period) {
        osc->_priv.t = 0;
    }

    uint32_t tOffset = osc->_priv.t;
    if (osc->phaseOffset != NULL) {
        tOffset += SAMPLE_RATE * fmodPos(*osc->phaseOffset, 360) / (360 * freq);
        tOffset = tOffset % (uint16_t) period;
    }

    switch (*osc->waveform) {
    case WAV_SINE:
        sample = amplitude * sinf(M_TAU * tOffset * freq / SAMPLE_RATE);
        break;
    case WAV_SQUARE:
        sample = (tOffset < SAMPLE_RATE / freq / 2 ? -amplitude : amplitude);
        break;
    case WAV_TRI:
        sample = amplitude * 4 * (fabs(fmodf(tOffset, period) - period / 2.0f) - period / 4.0f) / period;
        break;
    case WAV_SAW:
        sample = amplitude * (2.0f * fmodf(tOffset, period) / period - 1);
        break;
    }
    osc->_priv.t += 1;
    osc->out = sample;
}

static void ampRun(struct Amplifier *amp) {
    float sampleOut = *amp->sampleIn * *amp->gain;
    if (sampleOut > INT16_MAX) {
        amp->out = INT16_MAX;
    } else if (sampleOut < INT16_MIN) {
        amp->out = INT16_MIN;
    } else {
        amp->out = sampleOut;
    }
}

static void attrRun(struct Attenuator *attr) {
    attr->out = *attr->sampleIn * (*attr->amount - INT16_MIN) / (INT16_MAX - INT16_MIN);
}

static void envRun(struct Envelope *env) {
    uint32_t attackPeriod = *env->attackMs * SAMPLE_RATE / 1000.0f;
    uint32_t decayPeriod = *env->decayMs * SAMPLE_RATE / 1000.0f;
    uint32_t releasePeriod = *env->releaseMs * SAMPLE_RATE / 1000.0f;
    int16_t sample;

    int16_t sustain = *env->sustain;

    if (*env->gate == true && env->_priv.prevGate == false) {
        env->_priv.t = 0;
    }
    if (*env->gate == false && env->_priv.prevGate == true) {
        env->_priv.t = attackPeriod + decayPeriod;
        env->_priv.releaseSample = env->out;
    }

    env->_priv.prevGate = *env->gate;

    if (*env->gate == false) {
        // release case
        if (env->_priv.t >= attackPeriod + decayPeriod && env->_priv.t < attackPeriod + decayPeriod + releasePeriod) {
            sample = (int16_t) (env->_priv.releaseSample - (env->_priv.t - attackPeriod - decayPeriod) * (env->_priv.releaseSample - INT16_MIN) / releasePeriod);
            env->_priv.t += 1;
            env->out = sample;
        } else {
            env->out = INT16_MIN;
        }

        return;
    }

    if (env->_priv.t < attackPeriod) {
        sample = env->_priv.t * 2 * INT16_MAX / attackPeriod + INT16_MIN;
        env->_priv.t += 1;
    } else if (env->_priv.t < attackPeriod + decayPeriod) {
        sample = INT16_MAX - (env->_priv.t - attackPeriod) * (INT16_MAX - sustain) / decayPeriod;
        env->_priv.t += 1;
    } else {
        sample = sustain;
    } 

    env->out = sample;
}

static void rectangularWindow(float *windowBuf, size_t impulseLen) {
    for (size_t i = 0; i < impulseLen; i++) {
        windowBuf[i] = 1.0f;
    }
}

static void hannWindow(float *windowBuf, size_t impulseLen) {
    for (size_t i = 0; i < impulseLen; i++) {
        windowBuf[i] = 0.5f * (1.0f - cosf(M_TAU * i / (impulseLen - 1)));
    }
}

static void hammingWindow(float *windowBuf, size_t impulseLen) {
    for (size_t i = 0; i < impulseLen; i++) {
        windowBuf[i] = 0.54f - 0.46f * cosf(M_TAU * i / (impulseLen - 1));
    }
}

static void bartlettWindow(float *windowBuf, size_t impulseLen) {
    for (size_t i = 0; i < impulseLen; i++) {
        if (i < (impulseLen - 1) / 2) {
            windowBuf[i] = 2.0f * i / (impulseLen - 1);
        } else {
            windowBuf[i] =  2.0f - 2.0f * i / (impulseLen - 1);
        }
    }
}

static void blackmanWindow(float *windowBuf, size_t impulseLen) {
    for (size_t i = 0; i < impulseLen; i++) {
        windowBuf[i] = 0.42 - 0.5 * cosf(M_TAU * i / (impulseLen - 1) + 0.08f * cosf(2.0f * M_TAU * i / (impulseLen - 1)));
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
    filter->_priv.samplesBuf[filter->_priv.samplesBufIdx] = *filter->sampleIn;
    if (++filter->_priv.samplesBufIdx == filter->impulseLen) {
        filter->_priv.samplesBufIdx = 0;
    }

    if (*filter->cutoff != filter->_priv.prevCutoff) {
        float cutoffFreq = sampleToFreq(*filter->cutoff);
        float responseSum = 0;

        for (size_t i = 0; i < filter->impulseLen; i++) {
            float nextImpulse = filter->_priv.windowBuf[i] * sinc(M_TAU * cutoffFreq / SAMPLE_RATE * (i - (filter->impulseLen - 1) / 2.0f));
            filter->_priv.impulseResponse[i] = nextImpulse;
            responseSum += nextImpulse;
        }

        for (size_t i = 0; i < filter->impulseLen; i++) {
            filter->_priv.impulseResponse[i] /= responseSum;
        }
    }

    float sampleOut = 0;
    size_t sumIdx = filter->_priv.samplesBufIdx;

    for (size_t i = 0; i < filter->impulseLen; i++) {
        if (--sumIdx == SIZE_MAX) {
            sumIdx = filter->impulseLen - 1;
        }
        sampleOut += filter->_priv.samplesBuf[sumIdx] * filter->_priv.impulseResponse[i];
    }

    filter->_priv.prevCutoff = *filter->cutoff;
    filter->out = sampleOut;
}

static void synthInit(struct Synth *synth) {
    for (size_t i = 0; i < FILTERS_LEN; i++) {
        struct Filter *filter = &synth->filters[i];
        createFirWindow(filter->_priv.windowBuf, filter->window, filter->impulseLen);
    }
}

void synthRun(struct Synth *synth) {
    if (synth->_priv.isInit == false) {
        synthInit(synth);
        synth->_priv.isInit = true;
    }
    for (size_t i = 0; synth->amps[i].sampleIn != NULL && i < AMPS_LEN; i++) {
        ampRun(&synth->amps[i]);
    }
    for (size_t i = 0; synth->oscs[i].waveform != NULL && i < OSCS_LEN; i++) {
        oscRun(&synth->oscs[i]);
    }
    for (size_t i = 0; synth->mixers[i].samplesIn != NULL && i < MIXERS_LEN; i++) {
        mixerRun(&synth->mixers[i]);
    }
    for (size_t i = 0; synth->filters[i].sampleIn != NULL && i < FILTERS_LEN; i++) {
        filterRun(&synth->filters[i]);
    }
    for (size_t i = 0; synth->envs[i].gate != NULL && i < ENVS_LEN; i++) {
        envRun(&synth->envs[i]);
    }
    for (size_t i = 0; synth->dists[i].sampleIn != NULL && i < DISTS_LEN; i++) {
        distRun(&synth->dists[i]);
    }
    for (size_t i = 0; synth->attrs[i].sampleIn != NULL && i < ATTRS_LEN; i++) {
        attrRun(&synth->attrs[i]);
    }
}

