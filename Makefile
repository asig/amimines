.PHONY: clean

OBJS = \
  gen/minesweeper.o \
  gen/images.o \

all: minesweeper

clean:
	@rm -f *.o minesweeper
	@rm -f gen/*
	@rm src/images.c src/images.h

src/images.c: gen/imggen resources/items.png
	tools/imggen resources/items.png src/images

gen/%.o: src/%.c
	vc +kick13 $< -c -o $@

minesweeper: $(OBJS)
	vc +kick13 -lamiga $^ -o $@

gen/imggen: tools/imggen.cpp
	gcc -g $< -lstdc++ -lpng -o $@