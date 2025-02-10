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

float easingExp(float t, float amt) {
    float a = (1 - (1 / amt));
    float base = a * a;

    if (amt == 0.5f) {
        return t;
    } else if (amt >= 1) {
        return t > 0 ? 1 : 0;
    } else if (amt <= 0) {
        return t <= 1 ? 0 : 1;
    } else {
        return (expf(t * logf(base)) - 1) / (base - 1);
    }
}

int16_t tToRangeEased(float t, float tInitial, float tFinal, int16_t yInitial, int16_t yFinal, float easingAmt) {
    if (t >= tFinal) return yFinal;
    if (t <= tInitial) return yInitial;

    return
        (yFinal - yInitial)
        * easingExp((1 / (tFinal - tInitial)) * (t - tInitial), easingAmt)
        + yInitial;
}

int16_t tToRange(float t, float tInitial, float tFinal, int16_t yInitial, int16_t yFinal) {
    if (t >= tFinal) return yFinal;
    if (t <= tInitial) return yInitial;

    float slope = (yFinal - yInitial) / (tFinal - tInitial);
    return slope * (t - tInitial) + yInitial;
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

static void printStage(enum EnvelopeStage stage) {
    switch (stage) {
    case STAGE_Pending:
        printf("%s\n", "Pending");
        break;
    case STAGE_Attack:
        printf("%s\n", "Attack");
        break;
    case STAGE_Decay:
        printf("%s\n", "Decay");
        break;
    case STAGE_Decay2:
        printf("%s\n", "Decay2");
        break;
    case STAGE_Sustain:
        printf("%s\n", "Sustain");
        break;
    case STAGE_Release:
        printf("%s\n", "Release");
        break;
    case STAGE_Finished:
        printf("%s\n", "Finished");
        break;
    }
}

static int16_t envAdRun(struct EnvelopeAd *env) {
    uint32_t attackPeriod = msToFrames(*env->attackMs);
    uint32_t decayPeriod = msToFrames(*env->decayMs);
    enum EnvelopeStage nextStage = env->_priv.stage;
    int16_t sample = INT16_MIN;

    switch(env->_priv.stage) {
    case STAGE_Pending:
        if (*env->gate == true) {
            nextStage = STAGE_Attack;
        }
        break;
    case STAGE_Attack:
        sample = tToRangeEased(env->_priv.t, 0, attackPeriod, INT16_MIN, INT16_MAX, *env->easing);
        if (++env->_priv.t > attackPeriod) {
            nextStage = STAGE_Decay;
        }
        break;
    case STAGE_Decay:
        sample = tToRangeEased(env->_priv.t, 0, decayPeriod, INT16_MAX, INT16_MIN, *env->easing);
        if (++env->_priv.t > decayPeriod) {
            nextStage = STAGE_Finished;
        }
        break;
    case STAGE_Finished:
        if (*env->gate == false) {
            nextStage = STAGE_Pending;
        }
        break;
    default:
        break;
    }

    if (nextStage != env->_priv.stage) {
        env->_priv.t = 0;
    }

    env->_priv.stage = nextStage;
    return sample;
}

static int16_t envArRun(struct EnvelopeAr *env) {
    uint32_t attackPeriod = msToFrames(*env->attackMs);
    uint32_t releasePeriod = msToFrames(*env->releaseMs);
    enum EnvelopeStage nextStage = env->_priv.stage;
    int16_t sample = INT16_MIN;

    switch (env->_priv.stage) {
    case STAGE_Pending:
        if (*env->gate == true) {
            nextStage = STAGE_Attack;
        }
        break;
    case STAGE_Attack:
        sample = tToRangeEased(env->_priv.t, 0, attackPeriod, INT16_MIN, INT16_MAX, *env->easing);
        if (++env->_priv.t > attackPeriod) {
            nextStage = STAGE_Sustain;
        }
        break;
    case STAGE_Sustain:
        sample = INT16_MAX;
        if (*env->gate == false) {
            nextStage = STAGE_Release;
        }
        break;
    case STAGE_Release:
        sample = tToRangeEased(env->_priv.t, 0, releasePeriod, INT16_MAX, INT16_MIN, *env->easing);
        if (++env->_priv.t > releasePeriod) {
            nextStage = STAGE_Pending;
        }
        if (*env->gate == true) {
            nextStage = STAGE_Attack;
        }
        break;
    default: break;
    }

    if (nextStage != env->_priv.stage) {
        env->_priv.t = 0;
    }

    env->_priv.stage = nextStage;
    return sample;
}

static int16_t envAdrRun(struct EnvelopeAdr *env) {
    uint32_t attackPeriod = msToFrames(*env->attackMs);
    uint32_t decayPeriod = msToFrames(*env->decayMs);
    uint32_t releasePeriod = msToFrames(*env->releaseMs);
    enum EnvelopeStage nextStage = env->_priv.stage;
    int16_t sample = INT16_MIN;

    printStage(env->_priv.stage);

    switch (env->_priv.stage) {
    case STAGE_Pending:
        if (*env->gate == true) {
            nextStage = STAGE_Attack;
        }
        break;
    case STAGE_Attack:
        sample = tToRangeEased(env->_priv.t, 0, attackPeriod, INT16_MIN, INT16_MAX, *env->easing);
        if (++env->_priv.t > attackPeriod) {
            nextStage = STAGE_Decay;
        }
        if (*env->gate == false) {
            env->_priv.releaseSample = sample;
            nextStage = STAGE_Release;
        }
        break;
    case STAGE_Decay:
        sample = tToRangeEased(env->_priv.t, 0, decayPeriod, INT16_MAX, INT16_MIN, *env->easing);
        if (++env->_priv.t > decayPeriod) {
            nextStage = STAGE_Finished;
        }
        if (*env->gate == false) {
            env->_priv.releaseSample = sample;
            nextStage = STAGE_Release;
        }
        break;
    case STAGE_Release:
        sample = tToRangeEased(env->_priv.t, 0, releasePeriod, env->_priv.releaseSample, INT16_MIN, *env->easing);
        if (++env->_priv.t > releasePeriod) {
            nextStage = STAGE_Pending;
        }
        if (*env->gate == true) {
            nextStage = STAGE_Attack;
        }
        break;
    case STAGE_Finished:
        if (*env->gate == false) {
            nextStage = STAGE_Pending;
        }
        break;
    default: break;
    }

    if (nextStage != env->_priv.stage) {
        env->_priv.t = 0;
    }

    env->_priv.stage = nextStage;
    return sample;
}

static int16_t envAdsrRun(struct EnvelopeAdsr *env) {
    uint32_t attackPeriod = msToFrames(*env->attackMs);
    uint32_t decayPeriod = msToFrames(*env->decayMs);
    uint32_t releasePeriod = msToFrames(*env->releaseMs);
    enum EnvelopeStage nextStage = env->_priv.stage;
    int16_t sample = INT16_MIN;

    switch (env->_priv.stage) {
    case STAGE_Pending:
        if (*env->gate == true) {
            nextStage = STAGE_Attack;
        }
        break;
    case STAGE_Attack:
        sample = tToRangeEased(env->_priv.t, 0, attackPeriod, INT16_MIN, INT16_MAX, *env->easing);
        if (++env->_priv.t > attackPeriod) {
            nextStage = STAGE_Decay;
        }
        if (*env->gate == false) {
            env->_priv.releaseSample = sample;
            nextStage = STAGE_Release;
        }
        break;
    case STAGE_Decay:
        sample = tToRangeEased(env->_priv.t, 0, decayPeriod, INT16_MAX, *env->sustain, *env->easing);
        if (++env->_priv.t > decayPeriod) {
            nextStage = STAGE_Sustain;
        }
        if (*env->gate == false) {
            env->_priv.releaseSample = sample;
            nextStage = STAGE_Release;
        }
        break;
    case STAGE_Sustain:
        sample = *env->sustain;
        if (*env->gate == false) {
            env->_priv.releaseSample = sample;
            nextStage = STAGE_Release;
        }
        break;
    case STAGE_Release:
        sample = tToRangeEased(env->_priv.t, 0, releasePeriod, env->_priv.releaseSample, INT16_MIN, *env->easing);
        if (++env->_priv.t > releasePeriod) {
            nextStage = STAGE_Pending;
        }
        if (*env->gate == true) {
            nextStage = STAGE_Attack;
        }
        break;
    default: break;
    }

    if (nextStage != env->_priv.stage) {
        env->_priv.t = 0;
    }

    env->_priv.stage = nextStage;
    return sample;
}

static int16_t envAdbdrRun(struct EnvelopeAdbdr *env) {
    uint32_t attackPeriod = msToFrames(*env->attackMs);
    uint32_t decay1Period = msToFrames(*env->decay1Ms);
    uint32_t decay2Period = msToFrames(*env->decay2Ms);
    uint32_t releasePeriod = msToFrames(*env->releaseMs);
    enum EnvelopeStage nextStage = env->_priv.stage;
    int16_t sample = INT16_MIN;

    switch (env->_priv.stage) {
    case STAGE_Pending:
        if (*env->gate == true) {
            nextStage = STAGE_Attack;
        }
        break;
    case STAGE_Attack:
        sample = tToRangeEased(env->_priv.t, 0, attackPeriod, INT16_MIN, INT16_MAX, *env->easing);
        if (++env->_priv.t > attackPeriod) {
            nextStage = STAGE_Decay;
        }
        if (*env->gate == false) {
            env->_priv.releaseSample = sample;
            nextStage = STAGE_Release;
        }
        break;
    case STAGE_Decay:
        sample = tToRangeEased(env->_priv.t, 0, decay1Period, INT16_MAX, *env->breakPoint, *env->easing);
        if (++env->_priv.t > decay1Period) {
            nextStage = STAGE_Decay2;
        }
        if (*env->gate == false) {
            env->_priv.releaseSample = sample;
            nextStage = STAGE_Release;
        }
        break;
    case STAGE_Decay2:
        sample = tToRangeEased(env->_priv.t, 0, decay2Period, *env->breakPoint, INT16_MIN, *env->easing);
        if (++env->_priv.t > decay2Period) {
            nextStage = STAGE_Finished;
        }
        if (*env->gate == false) {
            env->_priv.releaseSample = sample;
            nextStage = STAGE_Release;
        }
        break;
    case STAGE_Release:
        sample = tToRangeEased(env->_priv.t, 0, releasePeriod, env->_priv.releaseSample, INT16_MIN, *env->easing);
        if (++env->_priv.t > releasePeriod) {
            nextStage = STAGE_Pending;
        }
        if (*env->gate == true) {
            nextStage = STAGE_Attack;
        }
        break;
    case STAGE_Finished:
        if (*env->gate == false) {
            nextStage = STAGE_Pending;
        }
        break;
    default: break;
    }

    if (nextStage != env->_priv.stage) {
        env->_priv.t = 0;
    }

    env->_priv.stage = nextStage;
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
        case MODULE_EnvelopeAd:
            synth->modules[i].out = envAdRun(ptr);
            break;
        case MODULE_EnvelopeAr:
            synth->modules[i].out = envArRun(ptr);
            break;
        case MODULE_EnvelopeAdr:
            synth->modules[i].out = envAdrRun(ptr);
            break;
        case MODULE_EnvelopeAdsr:
            synth->modules[i].out = envAdsrRun(ptr);
            break;
        case MODULE_EnvelopeAdbdr:
            synth->modules[i].out = envAdbdrRun(ptr);
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
