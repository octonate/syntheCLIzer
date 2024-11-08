#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL.h>
#include <err.h>
#include "common.h"
#include "engine.h"

int keyNum = 49;

void synthInit(struct Synth *synth) {
    synth->oscsLen = 0;
    synth->envsLen = 0;
    synth->ampsLen = 0;
    synth->attrsLen = 0;
    synth->mixersLen = 0;
}

void mixerRun(struct Mixer *mixer) {
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

    synth->mixers[synth->mixersLen] = mixer;
    ++synth->mixersLen;
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


int16_t oscSine(double freq, uint16_t t) {
    return INT16_MAX * sin(M_TAU * t * freq / SAMPLE_RATE);
}

int16_t oscSquare(double freq, uint16_t t) {
    return (t < SAMPLE_RATE / freq / 2 ? INT16_MAX : INT16_MIN);
}

int16_t oscSaw(double freq, uint16_t t) {
    double period = SAMPLE_RATE / freq;
    return (int16_t) INT16_MAX * (2 * fmod(t, period) / period - 1);
}

int16_t oscTri(double freq, uint16_t t) {
    double period = SAMPLE_RATE / freq;
    return (int16_t) INT16_MAX * (4 * fmod((t < period / 2 ? t : -t), period) / period - 1);
}

void oscRun(struct Oscillator *osc) {
    double freq = sampleToFreq(*osc->freqSample);
    double period = SAMPLE_RATE / freq;
    int16_t sample;

    if (osc->t > period) {
        osc->t = 0;
    }
    switch (*osc->waveform) {
    case SINE:
        sample = oscSine(freq, osc->t);
        break;
    case SQUARE:
        sample = oscSquare(freq, osc->t);
        break;
    case TRI:
        sample = oscTri(freq, osc->t);
        break;
    case SAW:
        sample = oscSaw(freq, osc->t);
        break;
    }
    osc->t += 1;
    osc->out = sample;
}

void synthAddOsc(struct Synth *synth, struct Oscillator *osc, int16_t *freqIn, enum Waveform *waveform) {
    osc->freqSample = freqIn;
    osc->waveform = waveform;
    osc->t = 0;
    osc->out = INT16_MIN;

    synth->oscs[synth->oscsLen] = osc;
    ++synth->oscsLen;
}

void ampRun(struct Amplifier *amp) {
    double freqOut = sampleToFreq(*amp->sampleIn) * amp->gain;
    double sampleOut = freqToSample(freqOut);
    if (sampleOut > INT16_MAX) {
        amp->out = INT16_MAX;
    } else if (sampleOut < INT16_MIN) {
        amp->out = INT16_MIN;
    } else {
        amp->out = sampleOut;
    }
}

void synthAddAmp(struct Synth *synth, struct Amplifier *amp, int16_t *sampleIn, double gain) {
    amp->sampleIn = sampleIn;
    amp->gain = gain;
    amp->out = INT16_MIN;

    synth->amps[synth->ampsLen] = amp;
    ++synth->ampsLen;
}

void attrRun(struct Attenuator *attr) {
    attr->out = *attr->sampleIn * (double) (*attr->amount - INT16_MIN) / (INT16_MAX - INT16_MIN);
}

void synthAddAttr(struct Synth *synth, struct Attenuator *attr, int16_t *sampleIn, int16_t *gainSample) {
    attr->sampleIn = sampleIn;
    attr->amount = gainSample;
    attr->out = INT16_MIN;

    synth->attrs[synth->attrsLen] = attr;
    ++synth->attrsLen;
}

void envRun(struct Envelope *env) {
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
    //env->releaseSample = 0;
    env->out = INT16_MIN;

    synth->envs[synth->envsLen] = env;
    ++synth->envsLen;
}

void synthRun(struct Synth *synth) {
    for (int i = 0; i < synth->oscsLen; i++) {
        oscRun(synth->oscs[i]);
    }
    for (int i = 0; i < synth->envsLen; i++) {
        envRun(synth->envs[i]);
    }
    for (int i = 0; i < synth->attrsLen; i++) {
        attrRun(synth->attrs[i]);
    }
    for (int i = 0; i < synth->mixersLen; i++) {
        mixerRun(synth->mixers[i]);
    }
    for (int i = 0; i < synth->ampsLen; i++) {
        ampRun(synth->amps[i]);
    }
}

void audioCallback(void *userdata, Uint8 *stream, int len) {
    struct Synth *synth = (struct Synth*) userdata;
    int16_t *stream16 = (int16_t*) stream;

    for (unsigned i = 0; i < len / sizeof (int16_t); i++) {
        synthRun(synth);
        stream16[i] = *synth->outPtr;
    }
}

