CFLAGS = -Wall -Wno-unused-command-line-argument -pedantic -pedantic-errors -Wextra -Wstrict-prototypes -std=c99 -O3
DBGFLAGS = -fsanitize=undefined
LDLIBS = -lm -lSDL2
CC = gcc
OBJDIR = .obj
BIN = synth

all: $(BIN)
	-mv *.o $(OBJDIR)

VPATH = $(OBJDIR)
OBJS = main.o engine.o tui.o arrays.o

main.o: tui.h engine.h
engine.o: engine.h
tui.o: tui.h
arrays.o: tui.h

$(BIN): $(OBJS)
	$(CC) $(LDLIBS) $(CFLAGS) $^ -o $(BIN)

%.o: %.c | $(OBJDIR)
	$(CC) $(LDLIBS) $(CFLAGS) -c $< -o $@

$(OBJDIR): 
	mkdir $(OBJDIR)

debug: CFLAGS += $(DBGFLAGS)
debug: all

clean:
	rm $(OBJDIR)/*.o
	rmdir $(OBJDIR)
	rm $(BIN)

.PHONY: all clean debug
