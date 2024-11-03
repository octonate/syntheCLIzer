CFLAGS = -Wall -pedantic -Wextra -Wold-style-declaration -Wno-unused-parameter -std=c99
LDLIBS = -lm -lSDL2
OBJDIR = obj

CC = gcc

all: $(OBJDIR) $(OBJDIR)/main.o $(OBJDIR)/engine.o $(OBJDIR)/tui.o $(OBJDIR)/arrays.o
	$(CC) $(LDLIBS) $(OBJDIR)/main.o $(OBJDIR)/tui.o $(OBJDIR)/engine.o $(OBJDIR)/arrays.o -o synth

$(OBJDIR)/main.o: main.c tui.h engine.h common.h
	$(CC) $(CFLAGS) $(LDLIBS) -c main.c -o $(OBJDIR)/main.o

$(OBJDIR)/tui.o: tui.c tui.h
	$(CC) $(CFLAGS) $(LDLIBS) -c tui.c -o $(OBJDIR)/tui.o

$(OBJDIR)/engine.o: engine.c engine.h common.h
	$(CC) $(CFLAGS) $(LDLIBS) -c engine.c -o $(OBJDIR)/engine.o

$(OBJDIR)/arrays.o: arrays.c tui.h
	$(CC) $(CFLAGS) $(LDLIBS) -c arrays.c -o $(OBJDIR)/arrays.o

$(OBJDIR): 
	mkdir $(OBJDIR)

clean:
	rm $(OBJDIR)/*.o
	rmdir $(OBJDIR)
