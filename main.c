#include <soundio/soundio.h>

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "engine.h"
#include "tui.h"

#define NULL_TERM_ARR(type, ...) (type[]) {__VA_ARGS__, NULL}
#define MODULE(T, ...) (struct SynthModule){ .tag = MODULE_ ## T, .ptr = &(struct T){__VA_ARGS__ }}

#define PTR(x) &(int16_t){x}
#define PTRF(x) &(float){x}

typedef struct Oscillator Oscillator;

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

    struct Userdata callbackData = {0};

    struct SynthModule modules[] = {
        [0] = MODULE(Oscillator,
            .freqSample = &callbackData.inputFreq,
            .waveform = PTR(WAV_Square),
            .amt = PTR(floatToAmt(0.25)),
        ),
        [1] = MODULE(Oscillator,
            .freqSample = &callbackData.inputFreq,
            .waveform = PTR(WAV_Saw),
            .amt = &modules[4].out,
        ),
        [2] = MODULE(Mixer,
            .samplesIn = NULL_TERM_ARR(int16_t*, &modules[0].out, &modules[1].out),
        ),
        [3] = MODULE(Filter,
            .sampleIn = &modules[2].out,
            .cutoff = &modules[4].out,
            .impulseLen = 128,
            .window = WINDOW_Blackman,
        ),
        [4] = MODULE(EnvelopeAdsr,
            .gate = &callbackData.gate,
            .attackMs = PTRF(500),
            .decayMs = PTRF(500),
            .sustain = PTRF(floatToAmt(0.2)),
            .releaseMs = PTRF(2000),
            .easing = PTRF(0.8)
        )
    };

    struct Synth synth = {
        .modules = modules,
        .modulesLen = sizeof(modules) / sizeof(modules[0]),
        .outPtr = &modules[1].out
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
