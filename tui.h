#ifndef TUI_H
#define TUI_H

#include "engine.h"

#define LIST_BUF_SIZE 64

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

enum ScopeTriggerMode {
    TRIG_RISING_EDGE,
    TRIG_FALLING_EDGE,
};

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
    OUTLINE_THIN,
    OUTLINE_THICK,
    OUTLINE_ROUND,
    OUTLINE_DOUBLE,
    OUTLINE_ASCII,

    OUTLINE_STYLE_COUNT
};

enum OutlineChar {
    OUTLINE_HOR_LINE,
    OUTLINE_VERT_LINE,
    OUTLINE_UPPER_LEFT_CORNER,
    OUTLINE_UPPER_RIGHT_CORNER,
    OUTLINE_LOWER_LEFT_CORNER,
    OUTLINE_LOWER_RIGHT_CORNER,

    OUTLINE_CHAR_COUNT
};

enum ElementType {
    ELEMENT_SLIDER,
    ELEMENT_RADIOS,
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
    int16_t *in;
    double *triggerVal;
    int x;
    int y;
    int width;
    int height;
    int horScale;
    int xPos;
    enum ScopeTriggerMode trigMode;
    int16_t prevIn;
    int16_t t;
    bool canTrigger;
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
extern const char *outlineChars[OUTLINE_STYLE_COUNT][OUTLINE_CHAR_COUNT];

void tuiAddScope(struct Scope *scope, int16_t *in, int x, int y, int width, int height, int horScale, double *triggerVal, enum ScopeTriggerMode trigMode);
void tuiDrawScope(struct Scope *scope);

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

void radiosAddButton(struct Radios *radios, char *name, int val);
void sliderSetClr(struct Slider *slider, enum ColorFG fg, enum ColorBG bg, enum ColorFG fgFoc, enum ColorBG bgFoc);

void termInit(void);
void resetTerm(void);


#endif //TUI_H
