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

static void resetKeyRepeatRate() {
    system("xset r rate 660");
    system("xset r 43");
    system("xset r 30");
}

static void setKeyRepeatRate(enum ElementType elementType) {
    switch (elementType) {
    case SLIDER:
        system("xset r rate 25");
        system("xset -r 43");
        system("xset -r 30");
        break;
    case RADIOS:
        resetKeyRepeatRate();
        break;
    }
}

void tuiInit(struct Tui *tui, int x, int y, int width, int height, char *label) {
    tui->elementsLen = 0;
    tui->x = x;
    tui->y = y;
    tui->width = width;
    tui->height = height;
    tui->label = label;

    tui->sliderClrs.fg = defaultSliderClrs.fg;
    tui->sliderClrs.bg = defaultSliderClrs.bg;
    tui->sliderClrs.fgFoc = defaultSliderClrs.fgFoc;
    tui->sliderClrs.bgFoc = defaultSliderClrs.bgFoc;
    tui->focElementIdx = 0;
}

void tuiAddRadios(struct Tui *tui, struct Radios *radios, int x, int y, char *label) {
    radios->x = tui->x + x;
    radios->y = tui->y + y;
    radios->label = label;
    radios->buttonCount = 0;
    radios->val = 0;
    radios->isFoc = 0;
    radios->selectedButtonIdx = 0;
    
    tui->elements[tui->elementsLen].ptr.radios = radios;
    tui->elements[tui->elementsLen].type = RADIOS;
    ++tui->elementsLen;
}

void tuiSetDefaultSliderClrs(struct Tui *tui, enum ColorFG fg, enum ColorBG bg, enum ColorFG fgFoc, enum ColorBG bgFoc) {
    tui->sliderClrs.fg = fg;
    tui->sliderClrs.bg = bg;
    tui->sliderClrs.fgFoc = fgFoc;
    tui->sliderClrs.bgFoc = bgFoc;
}

void sliderSetClr(struct Slider *slider, enum ColorFG fg, enum ColorBG bg, enum ColorFG fgFoc, enum ColorBG bgFoc) {
    slider->clrs.fg = fg;
    slider->clrs.bg = bg;
    slider->clrs.fgFoc = fgFoc;
    slider->clrs.bgFoc = bgFoc;
}

static void sliderDraw(struct Slider *slider) {
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
}

static void sliderIncr(struct Slider *slider, int incr) {
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

static void radiosDraw(struct Radios *radios) {
    SET_CURSOR_POS(radios->x, radios->y);
    printf("%s", TEXT_RESET);

    for (int i = 0; i < radios->buttonCount; i++) {
        SET_CURSOR_POS(radios->x, radios->y + i);
        printf("%s", radios->buttonList[i].isSelected ? "* " : "  ");
        printf("%s", radios->buttonList[i].name);
    }
    SET_CURSOR_POS(radios->x, radios->y + radios->buttonCount);
    printf("%s%s", radios->isFoc ? clrsBG[CLR_BG_BR_K] : clrsBG[CLR_BG_K], radios->label);
}

static void radiosSelectButtonDown(struct Radios *radios) {
    radios->buttonList[radios->selectedButtonIdx].isSelected = false;

    if (radios->selectedButtonIdx == radios->buttonCount - 1) {
        radios->selectedButtonIdx = 0;
    } else {
        ++radios->selectedButtonIdx;
    }

    radios->buttonList[radios->selectedButtonIdx].isSelected = true;
    radios->val = radios->buttonList[radios->selectedButtonIdx].val;
}

static void radiosSelectButtonUp(struct Radios *radios) {
    radios->buttonList[radios->selectedButtonIdx].isSelected = false;

    if (radios->selectedButtonIdx == 0) {
        radios->selectedButtonIdx = radios->buttonCount - 1;
    } else {
        --radios->selectedButtonIdx;
    }

    radios->buttonList[radios->selectedButtonIdx].isSelected = true;
    radios->val = radios->buttonList[radios->selectedButtonIdx].val;
}


void radiosAddButton(struct Radios *radios, char *name, int val) {
    radios->buttonList[radios->buttonCount].name = name;
    radios->buttonList[radios->buttonCount].val = val;

    if (radios->buttonCount == 0) {
        radios->buttonList[radios->buttonCount].isSelected = true;
    } else {
        radios->buttonList[radios->buttonCount].isSelected = false;
    }

    ++radios->buttonCount;
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
    slider->val = INT16_MIN;

    tui->elements[tui->elementsLen].ptr.slider = slider;
    tui->elements[tui->elementsLen].type = SLIDER;
    ++tui->elementsLen;

    sliderDraw(slider);
}

void elementDraw(struct Element element) {
    switch (element.type) {
    case SLIDER:
        element.ptr.slider->isFoc = element.isFoc;
        sliderDraw(element.ptr.slider);
        break;
    case RADIOS:
        element.ptr.radios->isFoc = element.isFoc;
        radiosDraw(element.ptr.radios);
        break;
    }

    fflush(stdout);
}

void elementIncr(struct Element element) {
    switch (element.type) {
    case SLIDER:
        sliderIncr(element.ptr.slider, 1);
        break;
    case RADIOS:
        radiosSelectButtonUp(element.ptr.radios);
        break;
    }
    elementDraw(element);
}

void elementDecr(struct Element element) {
    switch (element.type) {
    case SLIDER:
        sliderIncr(element.ptr.slider, -1);
        break;
    case RADIOS:
        radiosSelectButtonDown(element.ptr.radios);
        break;
    }
    elementDraw(element);
}

void tuiNextElement(struct Tui *tui) {
    tui->elements[tui->focElementIdx].isFoc = false;
    elementDraw(tui->elements[tui->focElementIdx]);

    if (tui->focElementIdx == tui->elementsLen - 1) {
        tui->focElementIdx = 0;
    } else {
        ++tui->focElementIdx;
    }

    tui->elements[tui->focElementIdx].isFoc = true;

    setKeyRepeatRate(tui->elements[tui->focElementIdx].type);
    elementDraw(tui->elements[tui->focElementIdx]);
}

void tuiPrevElement(struct Tui *tui) {
    tui->elements[tui->focElementIdx].isFoc = false;
    elementDraw(tui->elements[tui->focElementIdx]);

    if (tui->focElementIdx == 0) {
        tui->focElementIdx = tui->elementsLen - 1;
    } else {
        --tui->focElementIdx;
    }

    tui->elements[tui->focElementIdx].isFoc = true;
    setKeyRepeatRate(tui->elements[tui->focElementIdx].type);

    elementDraw(tui->elements[tui->focElementIdx]);
}

static void boxDraw(int x, int y, int width, int height, enum BoxStyle style, char *label) {
    if (width < 2) return;
    
    int labelLen = strlen(label);
    if (labelLen > width - 2) {
        labelLen = width - 2;
    }

    SET_CURSOR_POS(x, y);
    printf("%s", TEXT_RESET);

    printf("%s", boxChars[style][UPPER_LEFT_CORNER]);
    printf("%.*s", labelLen, label);
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
    boxDraw(tui->x, tui->y, tui->width, tui->height, THIN, tui->label);
    //for (int i = 0; i < tui->slidersLen; i++) {
    //    sliderDraw(tui->sliders[i]);
    //}
}


static struct termios oldTerm, newTerm;
void hello() {
    tcgetattr(STDIN_FILENO, &oldTerm);
    newTerm = oldTerm;
    newTerm.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newTerm);
    printf("%s", CURSOR_HIDE);

    system("clear");
}


void resetTerm() {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldTerm);
    printf("%s%s", CURSOR_SHOW, TEXT_RESET);
    resetKeyRepeatRate();
}

