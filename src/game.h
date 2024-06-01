/*
 * Copyright (c) 2024 Andreas Signer <asigner@gmail.com>
 *
 * This file is part of AmiMines.
 *
 * AmiMines is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * AmiMines is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AmiMines.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GAME_H
#define GAME_H

#include <exec/types.h>

// Playfield size in tiles
#define PLAYFIELD_W_TILES 19
#define PLAYFIELD_H_TILES 13

// Tile states
#define TILE_CLOSED 0
#define TILE_OPEN 1
#define TILE_MARKED_MINE 2
#define TILE_MARKED_UNKNOWN 3

// When using unsigned int instead of unsigned char, VBCC seems
// to ignore the bitsize...
struct Tile {
  unsigned char mine : 1;             // >0 if there is a mine in this field
  unsigned char state : 2;            // State the item is in
  unsigned char surroundingMines : 3; // Number of mines surrounding this tile.
};

struct Tile2 {
  unsigned char open : 1;
  unsigned char open2 : 1;
};

struct Game {
  BOOL running;
  ULONG timerRunning;
  ULONG ticks;
  USHORT totalMines;
  SHORT unmarkedMines;
  SHORT closedTiles;
  struct Tile tiles[PLAYFIELD_H_TILES + 2]
                   [PLAYFIELD_W_TILES + 2]; // including sentinels
};

void newGame(struct Game *game, USHORT mines);

#endif
