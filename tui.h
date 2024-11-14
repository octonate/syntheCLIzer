#ifndef TUI_H
#define TUI_H

#include "common.h"
#include "engine.h"

#define KEYCODE_H 43
#define KEYCODE_L 30

#define TEXT_BOLD "\033[1m"
#define TEXT_RESET "\033[0m"

#define CURSOR_HIDE "\033[?25l"
#define CURSOR_SHOW "\033[?25h"
#define CURSOR_UP "\033[A"
#define CURSOR_DOWN "\033[B"

#define SET_CURSOR_POS(x, y) printf("\033[%d;%dH", y, x)
#define MOVE_CURSOR_RIGHT(x) printf("\033[%dC", x)
#define MOVE_CURSOR_LEFT(x) printf("\033[%dD", x)

#define MAX_RADIO_BUTTONS 8


enum ColorFG {
    CLR_K,
    CLR_R,
    CLR_G,
    CLR_Y,
    CLR_B,
    CLR_M,
    CLR_C,
    CLR_W,
    
    CLR_DIM_K,
    CLR_DIM_R,
    CLR_DIM_G,
    CLR_DIM_Y,
    CLR_DIM_B,
    CLR_DIM_M,
    CLR_DIM_C,
    CLR_DIM_W,
    
    CLR_BR_K,
    CLR_BR_R,
    CLR_BR_G,
    CLR_BR_Y,
    CLR_BR_B,
    CLR_BR_M,
    CLR_BR_C,
    CLR_BR_W,

    CLR_FG_COUNT
};

enum ColorBG {
    CLR_BG_K,
    CLR_BG_R,
    CLR_BG_G,
    CLR_BG_Y,
    CLR_BG_B,
    CLR_BG_M,
    CLR_BG_C,
    CLR_BG_W,

    CLR_BG_BR_K,
    CLR_BG_BR_R,
    CLR_BG_BR_G,
    CLR_BG_BR_Y,
    CLR_BG_BR_B,
    CLR_BG_BR_M,
    CLR_BG_BR_C,
    CLR_BG_BR_W,

    CLR_BG_COUNT
};

enum OutlineStyle {
    THIN,
    THICK,
    ROUND,
    DOUBLE,

    STYLE_COUNT
};

enum OutlineChar {
    HOR_LINE,
    VERT_LINE,
    UPPER_LEFT_CORNER,
    UPPER_RIGHT_CORNER,
    LOWER_LEFT_CORNER,
    LOWER_RIGHT_CORNER,

    BOX_CHAR_COUNT
};

enum ElementType {
    SLIDER,
    RADIOS,
    SCOPE,
};

struct ColorInfo {
    enum ColorFG fg;
    enum ColorBG bg;
    enum ColorFG fgFoc;
    enum ColorBG bgFoc;
};

struct Slider {
    int x;
    int y;
    int height;
    double minVal;
    double maxVal;
    double val;
    int divVal;
    char label;
    bool isFoc;
    struct ColorInfo clrs;
};

struct RadioButton {
    char *name;
    int val;
    bool isSelected;
};

struct Radios {
    struct RadioButton buttonList[MAX_RADIO_BUTTONS];
    char *label;
    int buttonCount;
    int selectedButtonIdx;
    int x;
    int y;
    int val;
    bool isFoc;
};

struct Scope {
    int x;
    int y;
    int width;
    int height;
    int16_t *in;
    int16_t *t;
};

struct Element {
    union {
        struct Slider *slider;
        struct Radios *radios;
    } ptr;

    enum ElementType type;
    bool isFoc;
};

struct Box {
    struct Element elements[LIST_BUF_SIZE];
    struct ColorInfo sliderClrs;
    enum OutlineStyle style;
    char *label;
    int focElementIdx;
    int elementsLen;
    int x;
    int y;
    int width;
    int height;
    bool isFoc;
};

struct Tui {
    struct Box *boxes[LIST_BUF_SIZE];
    char *label;
    int focBoxIdx;
    int boxesLen;
};

extern const char *barsVert[];
extern const int barsVertLen;
extern const char *barsHor[];

extern const char *clrsFG[];
extern const char *clrsBG[];
extern const char *outlineChars[STYLE_COUNT][BOX_CHAR_COUNT];

void tuiInit(struct Tui *tui, char *label);
void tuiNextBox(struct Tui *tui);
void tuiPrevBox(struct Tui *tui);
void tuiAddBox(struct Tui *tui, struct Box *box, int x, int y, int width, int height, char *label, enum OutlineStyle style);

void boxNextElement(struct Box *box);
void boxPrevElement(struct Box *box);
void boxIncrFocElement(struct Box *box);
void boxDecrFocElement(struct Box *box);
void boxSetDefaultSliderClrs(struct Box *box, enum ColorFG fg, enum ColorBG bg, enum ColorFG fgFoc, enum ColorBG bgFoc);

void boxAddSlider(struct Box *box, struct Slider *slider, int x, int y, int height, double minVal, double maxVal, char label);
void boxAddRadios(struct Box *box, struct Radios *radios, int x, int y, char *label);
void boxAddScope(struct Box *box, struct Scope *scope, int x, int y, int width, int height, int16_t *in, int16_t *t);

void radiosAddButton(struct Radios *radios, char *name, int val);
void sliderSetClr(struct Slider *slider, enum ColorFG fg, enum ColorBG bg, enum ColorFG fgFoc, enum ColorBG bgFoc);

void hello();
void resetTerm();


#endif //TUI_H
