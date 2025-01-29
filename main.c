#include <soundio/soundio.h>

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "engine.h"
#include "tui.h"


#define NULL_TERM_ARR(type, ...) (type[]) {__VA_ARGS__, NULL}

#define PTR(x) &(int16_t){x}
#define PTRF(x) &(float){x}


struct Userdata {
    struct Synth *synth;
    int16_t inputFreq;
    bool gate;
    bool quit;
};


void updateInput(struct Userdata *userdata) {
    int curChar = getchar();
    switch (curChar) {
    case '\0':
        break;
    case '[':
        userdata->gate = true;
        break;
    case ']':
        userdata->gate = false;
        break;
    case 'q':
        userdata->quit = true;
        break;
    default:
        userdata->gate = true;
        userdata->inputFreq = freqToSample(100 * powf(2, (curChar - 48) / 12.0f));
        break;
    }
}

void soundioCallback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
    struct Userdata *callbackData = outstream->userdata;
    struct SoundIoChannelArea *areas;

    int framesLeft = frame_count_max;
    int err;

    while (framesLeft > 0) {
        int frameCount = framesLeft;

        if ((err = soundio_outstream_begin_write(outstream, &areas, &frameCount))) {
            fprintf(stderr, "%s\n", soundio_strerror(err));
            exit(1);
        }
        
        if (!frameCount) break;

        for (int frame = 0; frame < frameCount; frame++) {
            synthRun(callbackData->synth);
            int16_t sample = *callbackData->synth->outPtr;

            for (int channel = 0; channel < outstream->layout.channel_count; channel++) {
                int16_t *samplePtr = (int16_t *)(areas[channel].ptr + areas[channel].step * frame);
                *samplePtr = sample;
            }
        }

        if ((err = soundio_outstream_end_write(outstream))) {
            fprintf(stderr, "%s\n", soundio_strerror(err));
            exit(1);
        }

        framesLeft -= frameCount;
    }

}

int main(void) {
    system("clear");
    srandqd(42);

    termInit();

    struct Userdata callbackData;

    struct Synth synth = {
        .oscs[0] = {
            .freqSample = &callbackData.inputFreq,
            .waveform = PTR(WAV_SQUARE),
            .amt = PTR(floatToAmt(0.25)),
        },
        .oscs[1] = {
            .freqSample = &callbackData.inputFreq,
            .waveform = PTR(WAV_SAW),
            .amt = PTR(floatToAmt(0.25))
        },

        .mixers[0].samplesIn = NULL_TERM_ARR(int16_t*, &synth.oscs[0].out, &synth.oscs[1].out),

        .filters[0] = {
            .sampleIn = &synth.mixers[0].out,
            .cutoff = &synth.envs[0].out,
            .impulseLen = 128,
            .window = WINDOW_HAMMING,
        },

        .envs[0] = {
            .gate = &callbackData.gate,
            .attackMs = PTRF(80),
            .decayMs = PTRF(150),
            .sustain = PTRF(freqToSample(1000)),
            .releaseMs = PTRF(1000),
        },

        .outPtr = &synth.filters[0].out,
    };

    callbackData.synth = &synth;

    int err;
    struct SoundIo *soundio = soundio_create();
    if (soundio == NULL) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    if ((err = soundio_connect(soundio))) {
        fprintf(stderr, "connection error\n");
        return 1;
    }

    soundio_flush_events(soundio);

    int deviceIdx = soundio_default_output_device_index(soundio);
    if (deviceIdx < 0) {
        fprintf(stderr, "no output device found\n");
        return 1;
    }

    struct SoundIoDevice *device = soundio_get_output_device(soundio, deviceIdx);
    if (device == NULL) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    struct SoundIoOutStream *outstream = soundio_outstream_create(device);
    outstream->format = SoundIoFormatS16NE;
    outstream->sample_rate = SAMPLE_RATE;
    outstream->write_callback = soundioCallback;
    outstream->userdata = &callbackData;
    outstream->software_latency = 0.02;

    if ((err = soundio_outstream_open(outstream))) {
        fprintf(stderr, "unable to open device: %s", soundio_strerror(err));
        return 1;
    }

    if (outstream->layout_error)
        fprintf(stderr, "unable to set channel layout: %s\n", soundio_strerror(outstream->layout_error));

    if ((err = soundio_outstream_start(outstream))) {
        fprintf(stderr, "unable to start device: %s", soundio_strerror(err));
        return 1;
    }

    while (callbackData.quit != true) {
        updateInput(&callbackData);
    }
    
    soundio_outstream_destroy(outstream);
    soundio_device_unref(device);
    soundio_destroy(soundio);

    resetTerm();

    return 0;
}
