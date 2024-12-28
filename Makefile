CFLAGS = -Wall -pedantic -Wextra -Wold-style-declaration -Wno-unused-parameter -Wstrict-prototypes -std=c99 -fsanitize=undefined
LDLIBS = -lm -lSDL2
CC = clang
OBJDIR = .obj
BIN = synth

all: $(BIN)
	-mv *.o $(OBJDIR)

VPATH = $(OBJDIR)
OBJS = main.o engine.o tui.o arrays.o callback.o

main.o: tui.h engine.h common.h callback.h
engine.o: engine.h common.h
tui.o: tui.h
arrays.o: tui.h
callback.o: callback.h

$(BIN): $(OBJS)
	$(CC) $(LDLIBS) $(CFLAGS) $^ -o $(BIN)

%.o: %.c | $(OBJDIR)
	$(CC) $(LDLIBS) $(CFLAGS) -c $< -o $@

$(OBJDIR): 
	mkdir $(OBJDIR)

clean:
	rm $(OBJDIR)/*.o
	rmdir $(OBJDIR)
	rm $(BIN)

.PHONY: all clean
