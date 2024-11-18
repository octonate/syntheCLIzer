CFLAGS = -Wall -pedantic -Wextra -Wold-style-declaration -Wno-unused-parameter -std=c99
LDLIBS = -lm -lSDL2
CC = gcc
OBJDIR = obj

VPATH = $(OBJDIR)
OBJECTS = main.o engine.o tui.o arrays.o callback.o

all: synth | $(OBJDIR)
	-mv *.o $(OBJDIR)

synth: $(OBJECTS)
	$(CC) $(LDLIBS) $^ -o synth

main.o: tui.h engine.h common.h
tui.o: tui.h
engine.o: engine.h common.h
arrays.o: tui.h
callback.o: engine.h tui.h callback.h

%.o: %.c
	$(CC) $(CFLAGS) $(LDLIBS) -c $< -o $@


$(OBJDIR): 
	mkdir $(OBJDIR)

clean:
	rm $(OBJDIR)/*.o
	rmdir $(OBJDIR)
	rm synth

.PHONY: all clean
