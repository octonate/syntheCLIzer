CFLAGS = -Wall -pedantic -Wextra -Wold-style-declaration -Wno-unused-parameter -std=c99
LDLIBS = -lm -lSDL2
CC = gcc

OBJDIR = obj
OBJECTS = main.o engine.o tui.o arrays.o callback.o

all: $(OBJDIR) synth

synth: $(addprefix $(OBJDIR)/, $(OBJECTS))
	$(CC) $(LDLIBS) $^ -o synth

$(OBJDIR)/main.o: tui.h engine.h common.h
$(OBJDIR)/tui.o: tui.h
$(OBJDIR)/engine.o: engine.h common.h
$(OBJDIR)/arrays.o: tui.h
$(OBJDIR)/callback.o: engine.h tui.h callback.h

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(LDLIBS) -c $< -o $@


$(OBJDIR): 
	mkdir $(OBJDIR)

clean:
	rm $(OBJDIR)/*.o
	rmdir $(OBJDIR)
	rm synth
