# AmiMines

## Overview
AmiMines as a Minesweeper clone for the 
[Commdore Amiga](https://en.wikipedia.org/wiki/Amiga). As you most probably
already now, Minesweeper is a classic puzzle game where the player must clear
a rectangular board containing hidden mines without detonating any of them,
using clues about the number of neighboring mines in each cell.

## Gameplay
The game is played on a grid of cells, some of which contain mines. The
objective is to clear all non-mined cells and mark all mines correctly.
Clicking on a cell will either reveal a number, indicating the count of mines
in the adjacent cells, or reveal a mine, ending the game.

If a cell with no neighboring mines is revealed, it will recursively reveal
all adjacent cells with numbers.

Right-clicking on a cell marks it as a mine (visualzed by a flag); 
right-clicking the cell again marks it as a suspected mine (indicated by a
question mark).

### Rules

#### Starting the Game

To start a new game, either change the difficulty, or press the "Smiley"
button. The number of mines hidden on the playfield varies based on the
difficuly level.

#### Numbers

Each number on a revealed cell represents the count of mines in the
adjacent eight cells.

#### Winning the Game

The game is won when all non-mined cells are revealed and all mines are
correctly marked.

#### Losing the Game

The game is lost if a mine is revealed by clicking on it.


## Building

### Dependencies

To build the game yourself, you need:
- The `vbcc` cross-compiler. If you're running Linux, it might be easiest to
  get it from https://github.com/asig/vbcc, as this comes already
  preconfigured with the 68000 backend, Amiga headers and libs, and the
  config to build for Kickstart 1.3

- A basic C++ development setup for your system, including libpng. This is
  needed because of the `imggen` and `infogen` tools that are needed during
  the build process. If you're running Debian or Ubuntu, just run 
  `sudo apt install build-essential libpng-dev`)

- `xdftool` to build the ADF image. `xdftool` is part of the 
  [Amitools suite](https://github.com/cnvogelg/amitools) and can be installed
  with `pip3 install amitools`. If you're running Debian, you probably want
  to add add `--break-system-packages`.

### Building the binary
After having installed all the dependicies, just run `make` to build the ADF
image.

The `Makefile` expects `vbcc` and `g++`, but is reasonably simple so that
switching to another compiler should not be a big deal.

### Logo

The "AmiMines" logo in the game was inspired by the font
[Jetlab Stretch Heavy Low](https://www.myfonts.com/de/products/stretch-heavy-low-jetlab-66040)

## License
Copyright (c) 2024 Andreas Signer.  
Licensed under [GPLv3](https://www.gnu.org/licenses/gpl-3.0).