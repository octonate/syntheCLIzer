#include "callback.h"

void audioCallback(void *userdata, uint8_t *stream, int len) {
    struct Synth *synth = (struct Synth*) userdata;
    int16_t *stream16 = (int16_t*) stream;

    for (unsigned i = 0; i < len / sizeof (int16_t); i++) {
        synthRun(synth);
        stream16[i] = *synth->outPtr;
        tuiDrawScope(synth->scope);
    }
}
