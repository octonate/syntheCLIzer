#include <cstdint>
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
static void boxDrawOutline(struct Box *box);

void scopeInit(struct Scope *scope, int x, int y, int width, int height, int16_t *in) {
    scope->x = x;
    scope->y = y;
    scope->width = width;
    scope->height = height;
    scope->in = in;
    scope->t = 0;
    memset(scope->prevIn, 0, sizeof(scope->prevIn));

    struct Box scopeBox = {
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .isFoc = false,
        .label = "",
        .style = THIN,
    };
    boxDrawOutline(&scopeBox);
}

void scopeDraw(struct Scope *scope) {
    int y = (double) *scope->in / (INT16_MAX - INT16_MIN) * scope->height;
    SET_CURSOR_POS(scope->x, scope->y + y);
    printf("*");
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


void boxAddRadios(struct Box *box, struct Radios *radios, int x, int y, char *label) {
    radios->x = box->x + x;
    radios->y = box->y + y;
    radios->label = label;
    radios->buttonCount = 0;
    radios->val = 0;
    radios->selectedButtonIdx = 0;

    if (box->elementsLen == 0  && box->isFoc) {
        radios->isFoc = true;
        setKeyRepeatRate(RADIOS);
    } else {
        radios->isFoc = false;
    }
    
    box->elements[box->elementsLen].ptr.radios = radios;
    box->elements[box->elementsLen].type = RADIOS;
    ++box->elementsLen;
}

void boxSetDefaultSliderClrs(struct Box *box, enum ColorFG fg, enum ColorBG bg, enum ColorFG fgFoc, enum ColorBG bgFoc) {
    box->sliderClrs.fg = fg;
    box->sliderClrs.bg = bg;
    box->sliderClrs.fgFoc = fgFoc;
    box->sliderClrs.bgFoc = bgFoc;
}

void sliderSetClr(struct Slider *slider, enum ColorFG fg, enum ColorBG bg, enum ColorFG fgFoc, enum ColorBG bgFoc) {
    slider->clrs.fg = fg;
    slider->clrs.bg = bg;
    slider->clrs.fgFoc = fgFoc;
    slider->clrs.bgFoc = bgFoc;
}

static void sliderDraw(struct Slider *slider) {
    SET_CURSOR_POS(1, 1);
    printf("slider-.isFoc = %d", slider->isFoc);
    enum ColorFG curFgClr = slider->isFoc ? slider->clrs.fgFoc : slider->clrs.fg;
    enum ColorBG curBgClr = slider->isFoc ? slider->clrs.bgFoc : slider->clrs.bg;

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
   radiosDraw(radios); 
}

static void elementDraw(struct Element element) {
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

void boxAddSlider(struct Box *box, struct Slider *slider, int x, int y, int height, double minVal, double maxVal, char label) {
    slider->x = box->x + x;
    slider->y = box->y + y;
    slider->height = height;
    slider->minVal = minVal;
    slider->maxVal = maxVal;
    slider->label = label;

    slider->clrs.fg = box->sliderClrs.fg;
    slider->clrs.bg = box->sliderClrs.bg;
    slider->clrs.fgFoc = box->sliderClrs.fgFoc;
    slider->clrs.bgFoc = box->sliderClrs.bgFoc;
    
    int maxDivs = height * (barsVertLen - 1);
    slider->divVal = maxDivs / 2;
    slider->val = (double) slider->divVal / maxDivs * (maxVal - minVal) + minVal;

    if (box->elementsLen == 0) {
        box->elements[box->elementsLen].isFoc = true;
        slider->isFoc = true;
        setKeyRepeatRate(SLIDER);
    } else {
        box->elements[0].isFoc = false;
    }

    box->elements[box->elementsLen].ptr.slider = slider;
    box->elements[box->elementsLen].type = SLIDER;
    elementDraw(box->elements[box->elementsLen]);

    ++box->elementsLen;
}


void boxIncrFocElement(struct Box *box) {
    if (box->elementsLen == 0) return;
    struct Element curElement = box->elements[box->focElementIdx];
    switch (curElement.type) {
    case SLIDER:
        sliderIncr(curElement.ptr.slider, 1);
        break;
    case RADIOS:
        radiosSelectButtonUp(curElement.ptr.radios);
        break;
    }
    elementDraw(curElement);
}

void boxDecrFocElement(struct Box *box) {
    if (box->elementsLen == 0) return;
    struct Element curElement = box->elements[box->focElementIdx];
    switch (curElement.type) {
    case SLIDER:
        sliderIncr(curElement.ptr.slider, -1);
        break;
    case RADIOS:
        radiosSelectButtonDown(curElement.ptr.radios);
        break;
    }
    elementDraw(curElement);
}

void boxNextElement(struct Box *box) {
    if (box->elementsLen == 0) return;

    box->elements[box->focElementIdx].isFoc = false;
    elementDraw(box->elements[box->focElementIdx]);

    if (box->focElementIdx == box->elementsLen - 1) {
        box->focElementIdx = 0;
    } else {
        ++box->focElementIdx;
    }

    box->elements[box->focElementIdx].isFoc = true;

    setKeyRepeatRate(box->elements[box->focElementIdx].type);
    elementDraw(box->elements[box->focElementIdx]);
}

void boxPrevElement(struct Box *box) {
    if (box->elementsLen == 0) return;

    box->elements[box->focElementIdx].isFoc = false;
    elementDraw(box->elements[box->focElementIdx]);

    if (box->focElementIdx == 0) {
        box->focElementIdx = box->elementsLen - 1;
    } else {
        --box->focElementIdx;
    }

    box->elements[box->focElementIdx].isFoc = true;

    setKeyRepeatRate(box->elements[box->focElementIdx].type);
    elementDraw(box->elements[box->focElementIdx]);
}

static void boxDrawOutline(struct Box *box) {
    if (box->width < 2) return;
    
    int labelLen = strlen(box->label);
    if (labelLen > box->width - 2) {
        labelLen = box->width - 2;
    }

    SET_CURSOR_POS(box->x, box->y);
    printf("%s", TEXT_RESET);
    printf("%s", box->isFoc ? clrsFG[CLR_BR_W] : clrsFG[CLR_BR_K]);

    printf("%s", outlineChars[box->style][UPPER_LEFT_CORNER]);
    printf("%.*s", labelLen, box->label);
    for (int i = 0; i < box->width - labelLen - 2; i++) {
        printf("%s", outlineChars[box->style][HOR_LINE]);
    }
    printf("%s\b", outlineChars[box->style][UPPER_RIGHT_CORNER]);

    SET_CURSOR_POS(box->x, box->y + 1);

    for (int i = 0; i < box->height - 2; i++) {
        printf("%s", outlineChars[box->style][VERT_LINE]);
        MOVE_CURSOR_RIGHT(box->width - 2);
        printf("%s", outlineChars[box->style][VERT_LINE]);
        MOVE_CURSOR_LEFT(box->width);
        printf("%s", CURSOR_DOWN);
    }
    printf("%s", outlineChars[box->style][LOWER_LEFT_CORNER]);
    for (int i = 0; i < box->width - 2; i++) {
        printf("%s", outlineChars[box->style][HOR_LINE]);
    }
    printf("%s\b", outlineChars[box->style][LOWER_RIGHT_CORNER]);

    SET_CURSOR_POS(box->x, box->y);

    fflush(stdout);
}

void tuiInit(struct Tui *tui, char *label) {
    tui->label = label;
    tui->boxesLen = 0;
    tui->focBoxIdx = 0;
}

void boxToggleFocus(struct Box *box) {
    if (box->elementsLen > 0) {
        box->elements[box->focElementIdx].isFoc = box->isFoc ? false : true;
        elementDraw(box->elements[box->focElementIdx]);
    }
    box->isFoc = box->isFoc ? false : true;
    boxDrawOutline(box);
}

void tuiNextBox(struct Tui *tui) {
    boxToggleFocus(tui->boxes[tui->focBoxIdx]);

    if (tui->focBoxIdx == tui->boxesLen - 1) {
        tui->focBoxIdx = 0;
    } else {
        ++tui->focBoxIdx;
    }

    struct Box *focBox = tui->boxes[tui->focBoxIdx];
    boxToggleFocus(focBox);
    setKeyRepeatRate(focBox->elements[focBox->focElementIdx].type);
}

void tuiPrevBox(struct Tui *tui) {
    boxToggleFocus(tui->boxes[tui->focBoxIdx]);

    if (tui->focBoxIdx == 0) {
        tui->focBoxIdx = tui->boxesLen - 1;
    } else {
        --tui->focBoxIdx;
    }

    struct Box *focBox = tui->boxes[tui->focBoxIdx];
    boxToggleFocus(focBox);
    setKeyRepeatRate(focBox->elements[focBox->focElementIdx].type);
}

void tuiAddBox(struct Tui *tui, struct Box *box, int x, int y, int width, int height, char *label, enum OutlineStyle style) {
    box->elementsLen = 0;
    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;
    box->label = label;
    box->style = style;

    box->sliderClrs.fg = defaultSliderClrs.fg;
    box->sliderClrs.bg = defaultSliderClrs.bg;
    box->sliderClrs.fgFoc = defaultSliderClrs.fgFoc;
    box->sliderClrs.bgFoc = defaultSliderClrs.bgFoc;
    box->focElementIdx = 0;

    box->isFoc = tui->boxesLen == 0 ? true : false;

    boxDrawOutline(box);

    tui->boxes[tui->boxesLen] = box;
    ++tui->boxesLen;
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

