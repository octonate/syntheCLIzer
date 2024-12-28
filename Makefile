CFLAGS = -Wall -Wno-unused-command-line-argument -pedantic -pedantic-errors -Wextra -Wstrict-prototypes -std=c11 -fsanitize=undefined
LDLIBS = -lm -lSDL2
CC = clang
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

clean:
	rm $(OBJDIR)/*.o
	rmdir $(OBJDIR)
	rm $(BIN)

.PHONY: all clean
