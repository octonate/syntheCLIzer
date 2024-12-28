#include "callback.h"


void audioCallback(void *userdata, uint8_t *stream, int len) {
    struct Userdata *data = (struct Userdata *)userdata;
    struct Synth *synth = data->synth;
    struct Tui *tui = data->tui;

    int16_t *stream16 = (int16_t *)stream;

    for (unsigned i = 0; i < len / sizeof (int16_t); i++) {
        //switch (data->curChar) {
        //case '\0':
        //    break;
        //case '[':
        //    synth->input->gate = true;
        //    break;
        //case ']':
        //    synth->input->gate = false;
        //    break;
        //case 'q':
        //    data->quit = true;
        //    break;
        //case 'k':
        //    boxIncrFocElement(tui->boxes[tui->focBoxIdx]);
        //    break;
        //case 'j':
        //    boxDecrFocElement(tui->boxes[tui->focBoxIdx]);
        //    break;
        //case 'h':
        //    boxPrevElement(tui->boxes[tui->focBoxIdx]);
        //    break;
        //case 'l':
        //    boxNextElement(tui->boxes[tui->focBoxIdx]);
        //    break;
        //case 'H':
        //    tuiPrevBox(tui);
        //    break;
        //case 'L':
        //    tuiNextBox(tui);
        //    break;
        //default:
        //    synth->input->gate = true;
        //    synth->input->val = freqToSample(100 * pow(2, (double) (data->curChar - 48) / 12));
        //}

        data->curChar = '\0';
        synthRun(synth);
        stream16[i] = *synth->outPtr;
        tuiDrawScope(synth->scope);
    }
}
