.PHONY: clean

VBCCFLAGS = -dontwarn=51 -dontwarn=214 -DDEBUG

OBJS = \
  gen/images.o \
  gen/debug.o \
  gen/game.o \
  gen/graphics.o \
  gen/mem.o \
  gen/minesweeper.o \
  

all: minesweeper

clean:
	@rm -f *.o minesweeper
	@rm -f gen/*
	@rm -f src/images.c src/images.h

src/images.c: gen/imggen resources/items.png
	gen/imggen resources/items.png src/images

gen/%.o: src/%.c
	vc +kick13 $(VBCCFLAGS) $< -c -o $@

minesweeper: $(OBJS)
	vc +kick13 -lamiga $^ -o $@

gen/imggen: tools/imggen.cpp
	gcc -g $< -lstdc++ -lpng -o $@