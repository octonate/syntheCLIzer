#ifndef TUI_H
#define TUI_H

#include "common.h"

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

enum BoxStyle {
    THIN,
    THICK,
    ROUND,
    DOUBLE,
    STYLE_COUNT
};

enum BoxChar {
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

struct Element {
    void *ptr;
    enum ElementType type;
    bool isFoc;
};

struct Tui {
    struct Element elements[LIST_BUF_SIZE];
    struct ColorInfo sliderClrs;
    int focElementIdx;
    int elementsLen;
    int x;
    int y;
    int width;
    int height;
    char *label;
};

extern const char *barsVert[];
extern const int barsVertLen;
extern const char *barsHor[];

extern const char *clrsFG[];
extern const char *clrsBG[];
extern const char *boxChars[STYLE_COUNT][BOX_CHAR_COUNT];

void tuiInit(struct Tui *tui, int x, int y, int width, int height, char *label);
void tuiSetDefaultSliderClrs(struct Tui *tui, enum ColorFG fg, enum ColorBG bg, enum ColorFG fgFoc, enum ColorBG bgFoc);
void tuiDraw(struct Tui *tui);
void tuiNextElement(struct Tui *tui);
void tuiPrevElement(struct Tui *tui);

void tuiAddSlider(struct Tui *tui, struct Slider *slider, int x, int y, int height, double minVal, double maxVal, char label);

void sliderSetClr(struct Slider *slider, enum ColorFG fg, enum ColorBG bg, enum ColorFG fgFoc, enum ColorBG bgFoc);
void elementDraw(struct Element element);
void elementIncr(struct Element element);
void elementDecr(struct Element element);

void hello();
void resetTerm();


#endif //TUI_H
