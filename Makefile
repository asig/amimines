.PHONY: clean adf pre-build

AMIGA_CC = vc
AMIGA_CC_FLAGS = +kick13 -dontwarn=51 -dontwarn=214 -DDEBUG

AMIGA_LD = vc
AMIGA_LD_FLAGS = +kick13 -lamiga


HOST_CC = g++
HOST_CC_FLAGS = -g

BUILDDIR = build
TOOLSDIR = $(BUILDDIR)/tools
OBJDIR = $(BUILDDIR)/obj
OBJS = \
  $(OBJDIR)/images.o \
  $(OBJDIR)/copper.o \
  $(OBJDIR)/debug.o \
  $(OBJDIR)/game.o \
  $(OBJDIR)/mem.o \
  $(OBJDIR)/ui.o \
  $(OBJDIR)/main.o \
  

all: $(BUILDDIR)/amimines

clean:
	@rm -rf $(BUILDDIR)/*

pre-build:
	@mkdir -p $(TOOLSDIR)
	@mkdir -p $(OBJDIR)

src/images.c: $(TOOLSDIR)/imggen resources/elements.iff
	$(TOOLSDIR)/imggen resources/elements.iff src/images

$(OBJDIR)/%.o: src/%.c
	$(AMIGA_CC) $(AMIGA_CC_FLAGS) $< -c -o $@

$(BUILDDIR)/amimines: pre-build $(OBJS)
	$(AMIGA_LD) $(AMIGA_LD_FLAGS) $(OBJS) -o $@

$(TOOLSDIR)/imggen: tools/imggen.cpp tools/imgloader.cpp tools/imgloader.h
	$(HOST_CC) $(HOST_CC_FLAGS) tools/imggen.cpp tools/imgloader.cpp -lstdc++ -lpng -o $@

$(TOOLSDIR)/infogen: tools/infogen.cpp tools/imgloader.cpp tools/imgloader.h
	$(HOST_CC) $(HOST_CC_FLAGS) tools/infogen.cpp tools/imgloader.cpp -lstdc++ -lpng -o $@

adf: minesweeper resources/icons.iff $(TOOLSDIR)/infogen
	mkdir -p build/adf
	$(TOOLSDIR)/infogen --type TOOL --stacksize 10240 --icon resources/icons.iff@0,0,64,32 --icon resources/icons.iff@96,0,64,32 --x 10 --y 10 build/adf/minesweeper.info

