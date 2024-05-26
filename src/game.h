#ifndef GAME_H
#define GAME_H

#include <exec/types.h>

// Playfield size in tiles
#define PLAYFIELD_W_TILES 19
#define PLAYFIELD_H_TILES 13

// Tile states
#define TILE_CLOSED         0
#define TILE_OPEN           1
#define TILE_MARKED_MINE    2
#define TILE_MARKED_UNKNOWN 3

// When using unsigned int instead of unsigned char, VBCC seems
// to ignore the bitsize...
struct Tile {
    unsigned char mine : 1; // >0 if there is a mine in this field
    // unsigned char showNumber: 1; // >0: Show the number of mines
    unsigned char state: 2; // State the item is in
    unsigned char surroundingMines: 3; // Number of mines surrounding this tile.
};

struct Tile2 {
    unsigned char open : 1;
    unsigned char open2 : 1;
} ;

struct Game {
    BOOL running;
    ULONG timerRunning;
    ULONG ticks;
    USHORT totalMines;
    SHORT unmarkedMines;
    SHORT closedTiles;
    struct Tile  tiles[PLAYFIELD_H_TILES+2][PLAYFIELD_W_TILES+2]; // including sentinels
};

void newGame(struct Game *game, USHORT mines);

#endif
