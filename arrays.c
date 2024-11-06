#include <stdbool.h>
#include "tui.h"

const char *barsVert[] = { " ", "\u2581", "\u2582", "\u2583", "\u2584", "\u2585", "\u2586", "\u2587", "\u2588" };
const int barsVertLen = sizeof (barsVert) / sizeof (barsVert[0]);
const char *barsHor[] = { " ", "\u258F", "\u258E", "\u258D", "\u258C", "\u258B", "\u258A", "\u2589", "\u2588" };

const char *outlineChars[STYLE_COUNT][BOX_CHAR_COUNT] = {
    { "\u2500", "\u2502", "\u250c", "\u2510", "\u2514", "\u2518" }, 
    { "\u2501", "\u2503", "\u250f", "\u2513", "\u2517", "\u251b" }, 
    { "\u2500", "\u2502", "\u256d", "\u256e", "\u2570", "\u256f" }, 
    { "\u2550", "\u2551", "\u2554", "\u2557", "\u255a", "\u255d" }, 
};

const char *clrsFG[] = {
    //regular
    "\033[22;30m",
    "\033[22;31m",
    "\033[22;32m",
    "\033[22;33m",
    "\033[22;34m",
    "\033[22;35m",
    "\033[22;36m",
    "\033[22;37m",

    //dim
    "\033[2;30m",
    "\033[2;31m",
    "\033[2;32m",
    "\033[2;33m",
    "\033[2;34m",
    "\033[2;35m",
    "\033[2;36m",
    "\033[2;37m",

    //bright
    "\033[22;90m",
    "\033[22;91m",
    "\033[22;92m",
    "\033[22;93m",
    "\033[22;94m",
    "\033[22;95m",
    "\033[22;96m",
    "\033[22;97m",
};

const char *clrsBG[] = {
    //regular
    "\033[40m",
    "\033[41m",
    "\033[42m",
    "\033[43m",
    "\033[44m",
    "\033[45m",
    "\033[46m",
    "\033[47m",

    //bright
    "\033[100m",
    "\033[101m",
    "\033[102m",
    "\033[103m",
    "\033[104m",
    "\033[105m",
    "\033[106m",
    "\033[107m",
};

const char *waveformIcons[] = {
    "\u223f",
    "\u2440",
    
};
