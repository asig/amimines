#include "game.h"

#include <stdlib.h>

void newGame(struct Game *game, USHORT mines) {
	game->running = TRUE;
	game->timerRunning = FALSE;
	game->ticks = 0;
	game->unmarkedMines = mines;

	for(USHORT y = 0; y < PLAYFIELD_H_TILES + 1; y++) {
		for(USHORT x = 0; x < PLAYFIELD_W_TILES+1; x++) {
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
			for(int yy = -1; yy <=1; yy++) {
				for(int xx = -1; xx <=1; xx++) {
					game->tiles[y+yy][x+xx].surroundingMines++;
				}
			}
			// correct the sounding mines count of the center tile
			game->tiles[y][x].surroundingMines--;
			mines--;
		}
	}
}
			
