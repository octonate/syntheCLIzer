#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "tui.h"

const struct ColorInfo defaultSliderClrs = {
    .fg = CLR_W,
    .bg = CLR_BG_K,
    .fgFoc = CLR_BR_W,
    .bgFoc = CLR_BG_BR_K
};

void tuiInit(struct Tui *tui, int x, int y, int width, int height, char *label) {
    tui->slidersLen = 0;
    tui->x = x;
    tui->y = y;
    tui->width = width;
    tui->height = height;
    tui->label = label;

    tui->sliderClrs.fg = defaultSliderClrs.fg;
    tui->sliderClrs.bg = defaultSliderClrs.bg;
    tui->sliderClrs.fgFoc = defaultSliderClrs.fgFoc;
    tui->sliderClrs.bgFoc = defaultSliderClrs.bgFoc;
    tui->focSliderIdx = 0;
}

void tuiSetDefaultSliderClrs(struct Tui *tui, enum ColorFG fg, enum ColorBG bg, enum ColorFG fgFoc, enum ColorBG bgFoc) {
    tui->sliderClrs.fg = fg;
    tui->sliderClrs.bg = bg;
    tui->sliderClrs.fgFoc = fgFoc;
    tui->sliderClrs.bgFoc = bgFoc;
}


void tuiAddSlider(struct Tui *tui, struct Slider *slider, int x, int y, int height, double minVal, double maxVal, char label) {
    slider->x = tui->x + x;
    slider->y = tui->y + y;
    slider->height = height;
    slider->minVal = minVal;
    slider->maxVal = maxVal;
    slider->label = label;

    slider->clrs.fg = tui->sliderClrs.fg;
    slider->clrs.bg = tui->sliderClrs.bg;
    slider->clrs.fgFoc = tui->sliderClrs.fgFoc;
    slider->clrs.bgFoc = tui->sliderClrs.bgFoc;
    
    slider->divVal = 0;
    slider->val= 0;

    tui->sliders[tui->slidersLen] = slider;
    ++tui->slidersLen;

    sliderDraw(slider);
}

void sliderSetClr(struct Slider *slider, enum ColorFG fg, enum ColorBG bg, enum ColorFG fgFoc, enum ColorBG bgFoc) {
    slider->clrs.fg = fg;
    slider->clrs.bg = bg;
    slider->clrs.fgFoc = fgFoc;
    slider->clrs.bgFoc = bgFoc;
}

void sliderDraw(struct Slider *slider) {
    if (slider == NULL) return;

    enum ColorFG curFgClr = slider->isFoc ? slider->clrs.fgFoc: slider->clrs.fg;
    enum ColorBG curBgClr = slider->isFoc ? slider->clrs.bgFoc: slider->clrs.bg;

    enum ColorFG curLabelFgClr = slider->isFoc ? CLR_BR_W : CLR_W;
    enum ColorBG curLabelBgClr = slider->isFoc ? CLR_BG_BR_K : CLR_BG_K;

    int divLen = slider->height * (barsVertLen - 1);
    int boundary = (slider->height * slider->divVal) / divLen;

    SET_CURSOR_POS(slider->x, slider->y);
    printf("%s", clrsFG[curFgClr]);
    for (int i = slider->height - 1; i >= 0; i--) {
        printf("%s", clrsBG[curBgClr]);
        printf("%s", clrsFG[curFgClr]);
        if (i == boundary) {
            printf("%s", barsVert[slider->divVal % (barsVertLen - 1)]);
        } else if (i < boundary) {
            printf("%s", barsVert[barsVertLen - 1]);
        } else if (i > boundary) {
            printf("%s", barsVert[0]);
        }
        printf("\b%s", CURSOR_DOWN);
    }
    printf("%s%s%s%c", TEXT_BOLD, clrsBG[curLabelBgClr], clrsFG[curLabelFgClr], slider->label);
    SET_CURSOR_POS(slider->x, slider->y);
    fflush(stdout);
}

void sliderIncr(struct Slider *slider, int incr) {
    if (slider == NULL) return;

    int maxDivs = slider->height * (barsVertLen - 1);
    if (incr < 0 && slider->divVal < abs(incr)) {
        slider->divVal = 0;
    } else if (incr > 0 && maxDivs - slider->divVal < incr) {
        slider->divVal = maxDivs;
    } else {
        slider->divVal += incr;
    }
    slider->val = (double) slider->divVal / maxDivs * (slider->maxVal - slider->minVal) + slider->minVal;
}

void tuiNextSlider(struct Tui *tui) {
    sliderDraw(tui->sliders[tui->focSliderIdx]);
    tui->sliders[tui->focSliderIdx]->isFoc = false;

    if (tui->focSliderIdx == tui->slidersLen - 1) {
        tui->focSliderIdx = 0;
    } else {
        ++tui->focSliderIdx;
    }

    tui->sliders[tui->focSliderIdx]->isFoc = true;

    sliderDraw(tui->sliders[tui->focSliderIdx]);
}

void tuiPrevSlider(struct Tui *tui) {
    sliderDraw(tui->sliders[tui->focSliderIdx]);
    tui->sliders[tui->focSliderIdx]->isFoc = false;

    if (tui->focSliderIdx == 0) {
        tui->focSliderIdx = tui->slidersLen - 1;
    } else {
        --tui->focSliderIdx;
    }

    tui->sliders[tui->focSliderIdx]->isFoc = true;

    sliderDraw(tui->sliders[tui->focSliderIdx]);
}

void boxDraw(int x, int y, int width, int height, enum BoxStyle style, char *label) {
    if (width < 2) return;
    
    int labelLen = strlen(label);
    if (labelLen > width - 2) {
        label[width - 2] = '\0';
        labelLen = width - 2;
    }

    SET_CURSOR_POS(x, y);
    printf("%s", TEXT_RESET);

    printf("%s", boxChars[style][UPPER_LEFT_CORNER]);
    printf("%s", label);
    for (int i = 0; i < width - labelLen - 2; i++) {
        printf("%s", boxChars[style][HOR_LINE]);
    }
    printf("%s\b", boxChars[style][UPPER_RIGHT_CORNER]);

    SET_CURSOR_POS(x, y + 1);

    for (int i = 0; i < height - 2; i++) {
        printf("%s", boxChars[style][VERT_LINE]);
        MOVE_CURSOR_RIGHT(width - 2);
        printf("%s", boxChars[style][VERT_LINE]);
        MOVE_CURSOR_LEFT(width);
        printf("%s", CURSOR_DOWN);
    }
    printf("%s", boxChars[style][LOWER_LEFT_CORNER]);
    for (int i = 0; i < width - 2; i++) {
        printf("%s", boxChars[style][HOR_LINE]);
    }
    printf("%s\b", boxChars[style][LOWER_RIGHT_CORNER]);

    SET_CURSOR_POS(x, y);
}

void tuiDraw(struct Tui *tui) {
    boxDraw(tui->x, tui->y, tui->width, tui->height, THICK, tui->label);
    for (int i = 0; i < tui->slidersLen; i++) {
        sliderDraw(tui->sliders[i]);
    }
}


static struct termios oldTerm, newTerm;
void hello() {
    tcgetattr(STDIN_FILENO, &oldTerm);
    newTerm = oldTerm;
    newTerm.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newTerm);
    printf("%s", CURSOR_HIDE);

    system("clear");
    system("xset r rate 25");
    system("xset -r 43");
    system("xset -r 30");
}

void resetTerm() {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldTerm);
    printf("%s%s", CURSOR_SHOW, TEXT_RESET);
    system("xset r rate 660");
    system("xset r 43");
    system("xset r 30");
}

