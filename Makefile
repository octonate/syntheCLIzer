CFLAGS = -Wall -pedantic -Wextra -Wold-style-declaration -Wno-unused-parameter -std=c99
LDLIBS = -lm -lSDL2
CC = gcc

OBJDIR = obj
OBJECTS = main.o engine.o tui.o arrays.o callback.o

all: $(OBJDIR) synth

synth: $(addprefix $(OBJDIR)/, $(OBJECTS))
	$(CC) $(LDLIBS) $^ -o synth

$(OBJDIR)/main.o: main.c tui.h engine.h common.h
	$(CC) $(CFLAGS) $(LDLIBS) -c $< -o $@

$(OBJDIR)/tui.o: tui.c tui.h
	$(CC) $(CFLAGS) $(LDLIBS) -c $< -o $@

$(OBJDIR)/engine.o: engine.c engine.h common.h
	$(CC) $(CFLAGS) $(LDLIBS) -c $< -o $@

$(OBJDIR)/arrays.o: arrays.c tui.h
	$(CC) $(CFLAGS) $(LDLIBS) -c $< -o $@

$(OBJDIR)/callback.o: callback.c engine.h tui.h callback.h
	$(CC) $(CFLAGS) $(LDLIBS) -c $< -o $@

$(OBJDIR): 
	mkdir $(OBJDIR)

clean:
	rm $(OBJDIR)/*.o
	rmdir $(OBJDIR)
	rm synth
