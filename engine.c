#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "engine.h"

int16_t floatToAmt(float amt) {
    if (amt >= 1) return INT16_MAX;
    if (amt <= 0) return INT16_MIN;

    return amt * (INT16_MAX - INT16_MIN) + INT16_MIN;
}

int16_t tToRange(float t, float tInitial, float tFinal, int16_t yInitial, int16_t yFinal) {
    if (t >= tFinal) return yFinal;
    if (t <= tInitial) return yInitial;

    float slope = (yFinal - yInitial) / (tFinal - tInitial);

    return slope * (t - tInitial) + yInitial;
}

int16_t freqToSample(float freq) {
    return INT16_MAX * logf(freq) / (logf(SAMPLE_RATE / 2.0f - MIDDLE_C_FREQ));
}

float sampleToFreq(int16_t sample) {
    return expf(sample * logf(SAMPLE_RATE / 2.0f - MIDDLE_C_FREQ) / INT16_MAX);
}

float sampleToFloat(int16_t sample, float rangeMin, float rangeMax) {
    float floatOut =
        (rangeMax + rangeMin) / 2
        + sample * (rangeMax - rangeMin) / (INT16_MAX - INT16_MIN);

    // floating point is annoying
    if (floatOut <= rangeMin) return rangeMin;
    if (floatOut >= rangeMax) return rangeMax;

    return floatOut;
}

static float fmodPos(float x, float y) {
    float result = fmodf(x, y);
    return (result >= 0 ? result : result + y);
}

static float sinc(float x) {
    return x == 0 ? 1 : sinf(x) / x;
}

Cplx euler(float phi) {
    return (Cplx) {
        .real = cosf(phi),
        .imag = sinf(phi)
    };
}

Cplx cplxAdd(Cplx z1, Cplx z2) {
    return (Cplx){ z1.real + z2.real, z1.imag + z2.imag };
}

Cplx cplxScale(Cplx z, float k) {
    return (Cplx){ k * z.real, k * z.imag};
}

Cplx cplxMult(Cplx z1, Cplx z2) {
    return (Cplx){
        .real = z1.real * z2.real - z1.imag * z2.imag,
        .imag = z1.real * z2.imag + z2.real * z1.imag
    };
}

void cplxPrint(Cplx z) {
    printf("%f %c j%f\n", z.real, z.imag >= 0 ? '+' : '-', fabs(z.imag));
}

void slowFourierTransformHalf(int16_t *sampleBuf, Cplx *outBuf, size_t bufLen) {
    for (size_t k = 0; k < bufLen / 2; k++) {
        outBuf[k] = (Cplx){ 0, 0 };
        for (size_t n = 0; n < bufLen; n++) {
            float b = M_TAU * k * n / bufLen;
            outBuf[k] = cplxAdd(outBuf[k], cplxScale(euler(-b), 2.0f * (float) sampleBuf[n] / bufLen));
        }
    }
}

void slowFourierTransform(int16_t *sampleBuf, Cplx *outBuf, size_t bufLen) {
    for (size_t k = 0; k < bufLen; k++) {
        outBuf[k] = (Cplx){ 0, 0 };
        for (size_t n = 0; n < bufLen; n++) {
            float b = M_TAU * k * n / bufLen;
            outBuf[k] = cplxAdd(outBuf[k], cplxScale(euler(-b), (float) sampleBuf[n] / bufLen));
        }
    }
}


void recursiveFFT(const Cplx *inBuf, Cplx *outBuf, size_t bufLen) {
    if (bufLen == 1) {
        outBuf[0] = inBuf[0];
        return;
    }

    Cplx nthRoot = euler(M_TAU / bufLen);
    Cplx w = {.real = 1};

    Cplx evensIn[bufLen / 2];
    Cplx oddsIn[bufLen / 2];
    Cplx evensOut[bufLen / 2];
    Cplx oddsOut[bufLen / 2];
    for (size_t i = 0; i < bufLen / 2; i++) {
        evensIn[i] = inBuf[2 * i];
        oddsIn[i] = inBuf[2 * i + 1];
    }

    recursiveFFT(evensIn, evensOut, bufLen / 2);
    recursiveFFT(oddsIn, oddsOut, bufLen / 2);

    for (size_t k = 0; k < bufLen / 2; k++) {
        outBuf[k] = cplxAdd(evensOut[k], cplxMult(w, oddsOut[k]));
        outBuf[k + bufLen / 2] = cplxAdd(evensOut[k], cplxMult(cplxScale(w, -1), oddsOut[k]));
        w = cplxMult(w, nthRoot);
    }
}

void slowFFT(const int16_t *inBuf, Cplx *outBuf, size_t bufLen) {
    Cplx cplxInBuf[bufLen];
    for (size_t i = 0; i < bufLen; i++) {
        cplxInBuf[i] = (Cplx){ .real = inBuf[i] };
    }
    recursiveFFT(cplxInBuf, outBuf, bufLen);
    for (size_t i = 0; i < bufLen; i++) {
        outBuf[i] = cplxScale(outBuf[i], 1.0f / bufLen);
    }
}

void printFourier(Cplx *fourier, size_t fourierLen, float sampleRate) {
    for (size_t i = 0; i < fourierLen; i++) {
        printf("%.2f Hz: ", i * sampleRate / fourierLen / 2);
        cplxPrint(fourier[i]);
    }
}


void sftTest(void) {
    int16_t samples[128];
    float freq = 1300;
    float freq2 = 12000;
    float amplitude = 1000;
    float sampleRate = SAMPLE_RATE;
    for (int i = 0; i < 128; i++) {
        samples[i] = 
            amplitude
            * sinf(M_TAU * i * freq / sampleRate)
            + amplitude
            * sinf(M_TAU * i * freq2 / sampleRate);
    }
    Cplx fourierSFT[128];
    Cplx fourierFFT[128];
    slowFourierTransform(samples, fourierSFT, 128);
    slowFFT(samples, fourierFFT, 128);
    printf("slow:\n");
    printFourier(fourierSFT, 128, sampleRate);
    printf("\n\n\nfast:\n");
    printFourier(fourierFFT, 128, sampleRate);
}

static uint64_t nextRand = 42;


static int16_t mixerRun(struct Mixer *mixer) {
    int32_t total = 0;
    size_t len;
    for (len = 0; mixer->samplesIn[len] != NULL; len++) {
        total += *mixer->samplesIn[len];
    }
    if (total > INT16_MAX) total = INT16_MAX;
    if (total < INT16_MIN) total = INT16_MIN;
    return total;
}

static int16_t distRun(struct Distortion *distortion) {
    float x = (float) (*distortion->sampleIn + INT16_MAX) / (INT16_MAX - INT16_MIN);
    return (
        (INT16_MAX - INT16_MIN)
        * powf(x, *distortion->slope)
        / (powf(x, *distortion->slope) + powf(1 - x, *distortion->slope))
        + INT16_MIN
    );
}


void srandqd(int32_t seed) {
    nextRand = seed;
}

static int16_t randqd(void) {
    nextRand = 1664525 * nextRand + 1013904223;
    return nextRand;
}

static int16_t oscRun(struct Oscillator *osc) {
    if (*osc->waveform == WAV_Noise) {
        return randqd();
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
    case WAV_Sine:
        sample = amplitude * sinf(M_TAU * tOffset * freq / SAMPLE_RATE);
        break;
    case WAV_Square:
        sample = (tOffset < SAMPLE_RATE / freq / 2 ? -amplitude : amplitude);
        break;
    case WAV_Tri:
        sample = amplitude * 4 * (fabs(fmodf(tOffset, period) - period / 2.0f) - period / 4.0f) / period;
        break;
    case WAV_Saw:
        sample = amplitude * (2.0f * fmodf(tOffset, period) / period - 1);
        break;
    }
    osc->_priv.t += 1;
    return sample;
}

static int16_t ampRun(struct Amplifier *amp) {
    float sampleOut = *amp->sampleIn * *amp->gain;
    if (sampleOut > INT16_MAX) {
        return INT16_MAX;
    } else if (sampleOut < INT16_MIN) {
        return INT16_MIN;
    } else {
        return sampleOut;
    }
}

static int16_t attrRun(struct Attenuator *attr) {
    return *attr->sampleIn * (*attr->amount - INT16_MIN) / (INT16_MAX - INT16_MIN);
}

static uint32_t msToFrames(float ms) {
    return ms * SAMPLE_RATE / 1000.0f;
}


static int16_t envAdsrRun(struct EnvelopeAdsr *env) {
    uint32_t attackPeriod = msToFrames(*env->attackMs);
    uint32_t decayPeriod = msToFrames(*env->decayMs);
    uint32_t releasePeriod = msToFrames(*env->releaseMs);
    int16_t sample;

    int16_t sustain = *env->sustain;

    if (*env->gate == true && env->_priv.prevGate == false) {
        env->_priv.t = 0;
    }
    if (*env->gate == false && env->_priv.prevGate == true) {
        env->_priv.t = attackPeriod + decayPeriod;
        env->_priv.releaseSample = env->_priv.prevOut;
    }

    env->_priv.prevGate = *env->gate;

    if (*env->gate == false) {
        // release case
        if (env->_priv.t >= attackPeriod + decayPeriod
            && env->_priv.t < attackPeriod + decayPeriod + releasePeriod)
        {
            //sample = (int16_t)
            //    (env->_priv.releaseSample
            //     - (env->_priv.t - attackPeriod - decayPeriod)
            //     * (env->_priv.releaseSample - INT16_MIN)
            //     / releasePeriod);
            sample = tToRange(
                env->_priv.t,
                attackPeriod + decayPeriod,
                attackPeriod + decayPeriod + releasePeriod,
                env->_priv.releaseSample,
                INT16_MIN
            );

            env->_priv.t += 1;
        } else {
            sample = INT16_MIN;
        }

        env->_priv.prevOut = sample;
        return sample;
    }

    if (env->_priv.t < attackPeriod) {
        //sample = env->_priv.t * 2 * INT16_MAX / attackPeriod + INT16_MIN;
        sample = tToRange(env->_priv.t, 0, attackPeriod, INT16_MIN, INT16_MAX);
        env->_priv.t += 1;
    } else if (env->_priv.t < attackPeriod + decayPeriod) {
        //sample = INT16_MAX - (env->_priv.t - attackPeriod) * (INT16_MAX - sustain) / decayPeriod;
        sample = tToRange(env->_priv.t, attackPeriod, attackPeriod + decayPeriod, INT16_MAX, sustain);
        env->_priv.t += 1;
    } else {
        sample = sustain;
    } 

    env->_priv.prevOut = sample;
    return sample;
}

static int16_t envAdRun(struct EnvelopeAd *env) {
    uint32_t attackPeriod = msToFrames(*env->attackMs);
    uint32_t decayPeriod = msToFrames(*env->decayMs);
    int16_t sample = INT16_MIN;

    if (*env->gate == true && env->_priv.prevGate == false) {
        env->_priv.t = 0;
    }

    env->_priv.prevGate = *env->gate;
    
    if (env->_priv.t < attackPeriod) {
        sample = tToRange(env->_priv.t, 0, attackPeriod, INT16_MIN, INT16_MAX);
        env->_priv.t += 1;
    } else if (env->_priv.t <= attackPeriod + decayPeriod) {
        sample = tToRange(env->_priv.t, attackPeriod, attackPeriod + decayPeriod, INT16_MAX, INT16_MIN);
        env->_priv.t += 1;
    }

    return sample;
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
    case WINDOW_Rectangular:
        rectangularWindow(windowBuf, impulseLen);
        break;
    case WINDOW_Hann:
        hannWindow(windowBuf, impulseLen);
        break;
    case WINDOW_Hamming:
        hammingWindow(windowBuf, impulseLen);
        break;
    case WINDOW_Bartlett:
        bartlettWindow(windowBuf, impulseLen);
        break;
    case WINDOW_Blackman:
        blackmanWindow(windowBuf, impulseLen);
        break;
    }
}

static int16_t filterRun(struct Filter *filter) {
    filter->_priv.samplesBuf[filter->_priv.samplesBufIdx] = *filter->sampleIn;
    if (++filter->_priv.samplesBufIdx == filter->impulseLen) {
        filter->_priv.samplesBufIdx = 0;
    }

    // generate impulse response
    if (*filter->cutoff != filter->_priv.prevCutoff) {
        float cutoffFreq = sampleToFreq(*filter->cutoff);
        float responseSum = 0;

        for (size_t i = 0; i < filter->impulseLen; i++) {
            float nextImpulse =
                filter->_priv.windowBuf[i]
                * sinc(
                    M_TAU * cutoffFreq / SAMPLE_RATE
                    * (i - (filter->impulseLen - 1) / 2.0f));

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
    return sampleOut;
}

//static void synthInit(struct Synth *synth) {
//    for (size_t i = 0; i < FILTERS_LEN; i++) {
//        struct Filter *filter = &synth->filters[i];
//        createFirWindow(filter->_priv.windowBuf, filter->window, filter->impulseLen);
//    }
//}

//void synthRun(struct Synth *synth) {
//    if (synth->_priv.isInit == false) {
//        synthInit(synth);
//        synth->_priv.isInit = true;
//    }
//    for (size_t i = 0; synth->amps[i].sampleIn != NULL && i < AMPS_LEN; i++) {
//        ampRun(&synth->amps[i]);
//    }
//    for (size_t i = 0; synth->oscs[i].waveform != NULL && i < OSCS_LEN; i++) {
//        oscRun(&synth->oscs[i]);
//    }
//    for (size_t i = 0; synth->mixers[i].samplesIn != NULL && i < MIXERS_LEN; i++) {
//        mixerRun(&synth->mixers[i]);
//    }
//    for (size_t i = 0; synth->filters[i].sampleIn != NULL && i < FILTERS_LEN; i++) {
//        filterRun(&synth->filters[i]);
//    }
//    for (size_t i = 0; synth->envs[i].gate != NULL && i < ENVS_LEN; i++) {
//        envRun(&synth->envs[i]);
//    }
//    for (size_t i = 0; synth->dists[i].sampleIn != NULL && i < DISTS_LEN; i++) {
//        distRun(&synth->dists[i]);
//    }
//    for (size_t i = 0; synth->attrs[i].sampleIn != NULL && i < ATTRS_LEN; i++) {
//        attrRun(&synth->attrs[i]);
//    }
//} 

void synthInit(struct Synth *synth) {
    for (size_t i = 0; i < synth->modulesLen; i++) {
        if (synth->modules[i].tag == MODULE_Filter) {
            struct Filter *filter = synth->modules[i].ptr;
            createFirWindow(filter->_priv.windowBuf, filter->window, filter->impulseLen);
        }
    }
}

void synthRun(struct Synth *synth) {
    if (synth->_priv.isInit == false) {
        synthInit(synth);
        synth->_priv.isInit = true;
    }
    for (size_t i = 0; i < synth->modulesLen; i++) {
        void *ptr = synth->modules[i].ptr;
        switch (synth->modules[i].tag) {
        case MODULE_Oscillator:
            synth->modules[i].out = oscRun(ptr);
            break;
        case MODULE_EnvelopeAdsr:
            synth->modules[i].out = envAdsrRun(ptr);
            break;
        case MODULE_EnvelopeAd:
            synth->modules[i].out = envAdRun(ptr);
            break;
        case MODULE_Amplifier:
            synth->modules[i].out = ampRun(ptr);
            break;
        case MODULE_Distortion:
            synth->modules[i].out = distRun(ptr);
            break;
        case MODULE_Attenuator:
            synth->modules[i].out = attrRun(ptr);
            break;
        case MODULE_Mixer:
            synth->modules[i].out = mixerRun(ptr);
            break;
        case MODULE_Filter:
            synth->modules[i].out = filterRun(ptr);
            break;
        }
    }
}
