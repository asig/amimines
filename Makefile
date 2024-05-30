.PHONY: clean

VBCCFLAGS = -dontwarn=51 -dontwarn=214 -DDEBUG

OBJS = \
  gen/images.o \
  gen/copper.o \
  gen/debug.o \
  gen/game.o \
  gen/mem.o \
  gen/ui.o \
  gen/minesweeper.o \
  

all: minesweeper

clean:
	@rm -f *.o minesweeper
	@rm -f gen/*
	@rm -f src/images.c src/images.h

src/images.c: gen/imggen resources/elements.iff
	gen/imggen resources/elements.iff src/images

gen/%.o: src/%.c
	vc +kick13 $(VBCCFLAGS) $< -c -o $@

minesweeper: $(OBJS)
	vc +kick13 -lamiga $^ -o $@

gen/imggen: tools/imggen.cpp tools/imgloader.cpp tools/imgloader.h
	gcc -g tools/imggen.cpp tools/imgloader.cpp -lstdc++ -lpng -o $@

gen/infogen: tools/infogen.cpp tools/imgloader.cpp tools/imgloader.h
	gcc -g tools/infogen.cpp tools/imgloader.cpp -lstdc++ -lpng -o $@

adf: minesweeper resources/icons.iff gen/infogen
	mkdir -p build/adf
	gen/infogen --type TOOL --stacksize 10240 --icon resources/icons.iff@0,0,64,32 --icon resources/icons.iff@96,0,64,32 --x 10 --y 10 build/adf/minesweeper.info

