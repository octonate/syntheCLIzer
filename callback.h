#ifndef CALLBACK_H
#define CALLBACK_H

#include <stdbool.h>
#include "tui.h"
#include "engine.h"

struct Userdata {
    int curChar;
    struct Tui *tui;
    struct Synth *synth;
    bool quit;
};

void audioCallback(void *userdata, uint8_t *stream, int len);

#endif //CALLBACK_H
