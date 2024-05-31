.PHONY: clean adf pre-build

AMIGA_CC = vc
AMIGA_CC_FLAGS = +kick13 -dontwarn=51 -dontwarn=214 #-DDEBUG

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
  $(OBJDIR)/ui.o \
  $(OBJDIR)/main.o \
  

all: AmiMines.adf

clean:
	@rm -rf $(BUILDDIR)/*
	@rm -rf AmiMines.adf

pre-build:
	@mkdir -p $(TOOLSDIR)
	@mkdir -p $(OBJDIR)

src/images.c: $(TOOLSDIR)/imggen resources/elements.iff
	$(TOOLSDIR)/imggen resources/elements.iff src/images

$(OBJDIR)/%.o: src/%.c
	$(AMIGA_CC) $(AMIGA_CC_FLAGS) $< -c -o $@

$(BUILDDIR)/AmiMines: pre-build $(OBJS)
	$(AMIGA_LD) $(AMIGA_LD_FLAGS) $(OBJS) -o $@

$(TOOLSDIR)/imggen: tools/imggen.cpp tools/imgloader.cpp tools/imgloader.h
	$(HOST_CC) $(HOST_CC_FLAGS) tools/imggen.cpp tools/imgloader.cpp -lstdc++ -lpng -o $@

$(TOOLSDIR)/infogen: tools/infogen.cpp tools/imgloader.cpp tools/imgloader.h
	$(HOST_CC) $(HOST_CC_FLAGS) tools/infogen.cpp tools/imgloader.cpp -lstdc++ -lpng -o $@

$(BUILDDIR)/AmiMines.info: pre-build resources/icons.iff $(TOOLSDIR)/infogen
	$(TOOLSDIR)/infogen \
	    --type TOOL \
		--stacksize 32768 \
		--icon resources/icons.iff@0,0,64,32 \
		--icon resources/icons.iff@96,0,64,32 \
		--x 60 \
		--y 30 $@

$(BUILDDIR)/Disk.info: pre-build resources/icons.iff $(TOOLSDIR)/infogen
	$(TOOLSDIR)/infogen \
	    --type DISK \
		--icon resources/icons.iff@0,36,43,21 \
		--icon resources/icons.iff@43,36,43,21 \
		--x 50 \
		--y 10 \
		--drawer 30,40,200,100 \
		$@

AmiMines.adf: $(BUILDDIR)/AmiMines $(BUILDDIR)/AmiMines.info $(BUILDDIR)/Disk.info
	rm -rf $@
	xdftool $@ \
	  create \
	  + format 'AmiMines' ofs \
	  + boot install boot1x \
	  + makedir c \
	  + write resources/adf/Stack c/ \
	  + makedir s \
	  + write resources/adf/startup-sequence s/ \
	  + write $(BUILDDIR)/Disk.info / \
	  + write $(BUILDDIR)/AmiMines / \
	  + write $(BUILDDIR)/AmiMines.info /
		
