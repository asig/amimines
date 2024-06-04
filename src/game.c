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

#include "game.h"

#include <stdlib.h>

void newGame(struct Game *game, USHORT mines) {
  game->running = TRUE;
  game->timerRunning = FALSE;
  game->ticksStart = 0;
  game->unmarkedMines = mines;
  game->totalMines = mines;
  game->closedTiles = PLAYFIELD_H_TILES * PLAYFIELD_W_TILES;

  for (USHORT y = 0; y < PLAYFIELD_H_TILES + 1; y++) {
    for (USHORT x = 0; x < PLAYFIELD_W_TILES + 1; x++) {
      struct Tile *t = &game->tiles[y][x];
      t->mine = 0;
      t->state = TILE_CLOSED;
      t->surroundingMines = 0;
    }
  }

  while (mines > 0) {
    int x = (rand() % PLAYFIELD_W_TILES) + 1;
    int y = (rand() % PLAYFIELD_H_TILES) + 1;
    if (!game->tiles[y][x].mine) {
      game->tiles[y][x].mine = TRUE;
      // Increase surrounding mines count for neighbours
      for (int yy = -1; yy <= 1; yy++) {
        for (int xx = -1; xx <= 1; xx++) {
          game->tiles[y + yy][x + xx].surroundingMines++;
        }
      }
      // correct the sounding mines count of the center tile
      game->tiles[y][x].surroundingMines--;
      mines--;
    }
  }
}
