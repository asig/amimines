#include <clib/dos_protos.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <hardware/custom.h>
#include <clib/graphics_protos.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "copper.h"
#include "game.h"
#include "images.h"
#include "debug.h"
#include "layout.h"
#include "ui.h"

extern struct Custom custom;
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;

struct Game game;

BOOL terminate;
SHORT tileX;
SHORT tileY;
BOOL trackingMouse;
int difficulty;

struct Image *digits[] = {
    &imgDigit0,
    &imgDigit1,
    &imgDigit2,
    &imgDigit3,
    &imgDigit4,
    &imgDigit5,
    &imgDigit6,
    &imgDigit7,
    &imgDigit8,
    &imgDigit9,
};

struct Image *tileWithCounts[] = {
    &imgTile0,
    &imgTile1,
    &imgTile2,
    &imgTile3,
    &imgTile4,
    &imgTile5,
    &imgTile6,
    &imgTile7,
    &imgTile8,
};

void drawRemainingMines(int mines) {
    if (mines < 0) {
        DrawImage(window->RPort, &imgDigitDash, REMAINING_MINES_X, REMAINING_MINES_Y);
        mines = -mines;
        DrawImage(window->RPort, digits[mines/10], REMAINING_MINES_X+1*imgDigit0.Width, REMAINING_MINES_Y);
        DrawImage(window->RPort, digits[mines % 10], REMAINING_MINES_X+2*imgDigit0.Width, REMAINING_MINES_Y);
    } else {
        DrawImage(window->RPort, digits[mines/100], REMAINING_MINES_X+0*imgDigit0.Width, REMAINING_MINES_Y);
        mines %= 100;
        DrawImage(window->RPort, digits[mines/10], REMAINING_MINES_X+1*imgDigit0.Width, REMAINING_MINES_Y);
        DrawImage(window->RPort, digits[mines % 10], REMAINING_MINES_X+2*imgDigit0.Width, REMAINING_MINES_Y);
    }
}

void drawTimer(int secs) {
    DrawImage(window->RPort, digits[secs/100], TIMER_X+0*imgDigit0.Width, TIMER_Y);
    secs %= 100;
    DrawImage(window->RPort, digits[secs/10],  TIMER_X+1*imgDigit0.Width, TIMER_Y);
    DrawImage(window->RPort, digits[secs % 10], TIMER_X+2*imgDigit0.Width, TIMER_Y);
}

void drawPlayfieldTileExplicit(USHORT x, USHORT y, struct Image *img) {
    DrawImage(window->RPort, img, PLAYFIELD_X+(x-1)*16, PLAYFIELD_Y+(y-1)*16);
}

void drawPlayfieldTile(int x, int y) {
    struct Tile *tile = &game.tiles[y][x];
    struct Image *img;
    switch (tile->state) {
        case TILE_OPEN:
            if (tile->mine) {
                img = &imgTileMineExploded;
            } else {
                img = tileWithCounts[tile->surroundingMines];
            }
            break;
        case TILE_CLOSED:
            img = &imgTileClosed;
            break;
        case TILE_MARKED_MINE:
            img = &imgTileFlagged;
            break;
        case TILE_MARKED_UNKNOWN:
            img = &imgTileMaybe;
            break;
    }
    DrawImage(window->RPort, img, PLAYFIELD_X+(x-1)*16, PLAYFIELD_Y+(y-1)*16);
}

void drawPlayfieldTileSelected(USHORT x, USHORT y) {
    struct Tile *tile = &game.tiles[y][x];
    if (tile->state == TILE_CLOSED) {
        drawPlayfieldTileExplicit(x, y, &imgTile0);
    } else {
        drawPlayfieldTile(x,y);
    }
}

void drawPlayfield() {
    for(int y = 1; y <= PLAYFIELD_H_TILES; y++) {
        for (int x = 1; x <= PLAYFIELD_W_TILES; x++) {
            drawPlayfieldTile(x,y);
        }
    }
}

BOOL mouseToTile(SHORT mouseX, SHORT mouseY, USHORT *tileX, USHORT *tileY) {
    mouseX -= PLAYFIELD_X;
    mouseY -= PLAYFIELD_Y;
    if (mouseX < 0 || mouseX >= PLAYFIELD_W_TILES * imgTile0_W || mouseY < 0 || mouseY >= PLAYFIELD_H_TILES * imgTile0_H) {
        // outside playfield
        *tileX = *tileY = 0;
        return FALSE;
    }
    *tileX = mouseX/imgTile0_W + 1;
    *tileY = mouseY/imgTile0_H + 1;
    return TRUE;
}

void diedAt(USHORT tx, USHORT ty) {
    game.running = FALSE;
    game.timerRunning = FALSE;
    drawPlayfieldTileExplicit(tx,ty, &imgTileMineExploded);
    DrawImage(window->RPort, &imgFaceSad, SMILEY_X, SMILEY_Y);

    // Reveal all non-selected mines, highlight all false marked mines
    for (USHORT y = 1; y < PLAYFIELD_H_TILES + 1; y++) {
       for (USHORT x = 1; x < PLAYFIELD_W_TILES + 1; x++) {
            struct Tile* t = &game.tiles[y][x];
            if (t->state == TILE_MARKED_MINE && !t->mine) {
                drawPlayfieldTileExplicit(x,y, &imgTileMineWrong);      
            } else if ((t->state == TILE_CLOSED || t->state == TILE_MARKED_UNKNOWN) && t->mine) {                
                drawPlayfieldTileExplicit(x,y, &imgTileMine);
            }
       }
    }
}

void openEmpty(USHORT x, USHORT y) {
    struct Tile *tile = &game.tiles[y][x];
    if (tile->state == TILE_OPEN || x<1 || x > PLAYFIELD_W_TILES || y<1 || y> PLAYFIELD_H_TILES) {
        return;
    }
    if (tile->state == TILE_MARKED_MINE) {
        game.unmarkedMines++;
    }
    tile->state = TILE_OPEN;
    game.closedTiles--;
    drawPlayfieldTile(x,y);
    if (tile->surroundingMines == 0) {
        openEmpty(x-1,y-1); openEmpty(x,y-1); openEmpty(x+1,y-1);
        openEmpty(x-1,y);                     openEmpty(x+1,y);
        openEmpty(x-1,y+1); openEmpty(x,y+1); openEmpty(x+1,y+1);
    }
}

void openTile(USHORT x, USHORT y) {
    struct Tile *tile = &game.tiles[y][x];
    if (tile->mine) {
        // WE ARE DEAD!
        tile->state = TILE_OPEN;
        diedAt(x,y);
    } else {
        openEmpty(x, y);
    }
}

void gameWon() {
    // Reveal remaining unmarked mines
    for (USHORT y = 1; y < PLAYFIELD_H_TILES + 1; y++) {
       for (USHORT x = 1; x < PLAYFIELD_W_TILES + 1; x++) {
            struct Tile* t = &game.tiles[y][x];
            if (t->state == TILE_CLOSED) {
                t->state = TILE_MARKED_MINE;
                drawPlayfieldTileExplicit(x,y, &imgTileFlagged);
            }
       }
    }

    game.running = FALSE;
    game.timerRunning = FALSE;
    DrawImage(window->RPort, &imgFaceGlasses, SMILEY_X, SMILEY_Y);
}

void handleLmbDown(struct IntuiMessage *msg) {
    // LMB. Check if we click a tile
    if (mouseToTile(msg->MouseX, msg->MouseY, &tileX, &tileY)) {
        // Click is in a tile. Check the state
        struct Tile* t = &game.tiles[tileY][tileX];
        if (t->state != TILE_CLOSED) {
            // Ah, nothing to do :-(
            return;
        }
        trackingMouse = TRUE;
        DrawImage(window->RPort, &imgFaceO, SMILEY_X, SMILEY_Y);
        drawPlayfieldTileExplicit(tileX, tileY, &imgTile0);
    }
}

void handleLmbUp(struct IntuiMessage *msg) {
    DrawImage(window->RPort, &imgFaceNormal, SMILEY_X, SMILEY_Y);
    if (!trackingMouse) {
        return;
    }
    trackingMouse = FALSE;

    game.timerRunning = TRUE;

    if (tileX == 0 || tileY == 0) {
        // mouse outside playfield
        return;
    }

    openTile(tileX,tileY);
}

void handleRmbDown(struct IntuiMessage *msg) {
    // RMB. Check if we click a tile
    USHORT tx, ty;
    if (mouseToTile(msg->MouseX, msg->MouseY, &tx, &ty)) {
        // Click is in a tile. Check the state
        struct Tile *t = &game.tiles[ty][tx];
        BOOL update = TRUE;
        switch(t->state) {
            case TILE_CLOSED:
                t->state = TILE_MARKED_MINE;
                game.unmarkedMines--;
                break;
            case TILE_MARKED_MINE:
                t->state = TILE_MARKED_UNKNOWN;
                game.unmarkedMines++;
                break;
            case TILE_MARKED_UNKNOWN:
                t->state = TILE_CLOSED;
                break;
            default:
                update = FALSE;
        }
        if (update) {
            drawPlayfieldTile(tx, ty);
            drawRemainingMines(game.unmarkedMines);
        }
    }
}

void startGame(int d) {
    difficulty = d;
    int mines;
    switch(d) {
        case DIFFUCLTY_NOVICE:
            mines = PLAYFIELD_H_TILES*PLAYFIELD_W_TILES*.1;  // 10% mines
            break;
        case DIFFUCLTY_INTERMEDIATE:
            mines = PLAYFIELD_H_TILES*PLAYFIELD_W_TILES*.15;  // 15% mines
            break;
        case DIFFUCLTY_EXPERT:
            mines = PLAYFIELD_H_TILES*PLAYFIELD_W_TILES*.2;  // 20% mines
            break;
    }
    newGame(&game, mines);
    drawRemainingMines(game.unmarkedMines);
    drawTimer(game.ticks/50);
    drawPlayfield();
    trackingMouse = FALSE;
}

int main(int argc, char **argv) {
    printf("LOGO X1: %d\n", HEADER_X+HEADER_W);
    printf("LOGO Y2: %d\n", PLAYFIELD_Y - PLAYFIELD_BORDER);

    srand(time(NULL));
    debug_init();
   
    IntuitionBase = (struct IntuitionBase*)OpenLibrary("intuition.library", 0);
    if (!IntuitionBase) {
        fprintf(stderr, "Can't open intuition.library!");
        Exit(FALSE);
    }

    GfxBase = (struct GfxBase*)OpenLibrary("graphics.library", 0);
    if (!GfxBase) {
        fprintf(stderr, "Can't open graphics.library!");
        Exit(FALSE);
    }

    uiCreate(DIFFUCLTY_NOVICE);

    copperCreateList();
    copperAnimate();

    startGame(DIFFUCLTY_NOVICE);

    struct IntuiMessage *msg;
    terminate = FALSE;
    while(!terminate) {
        copperAnimate();
        WaitTOF();        

        if (game.timerRunning) {
            game.ticks++;
            drawTimer(game.ticks/50);
            drawRemainingMines(game.unmarkedMines);
        }

        if (game.running) {
            if (game.closedTiles == game.totalMines) {
                // Under every closed tile there must be a mine -> we won!
                gameWon();
            }
        }        

        if (trackingMouse) {
            // Handle mouse moves
            BOOL sameTile = FALSE;
            USHORT tX, tY;
            if (mouseToTile(window->MouseX, window->MouseY, &tX, &tY)) {
                // Current positiion is in the playfield
                sameTile = tX == tileX && tY == tileY;                
                if (sameTile) {
                    drawPlayfieldTileSelected(tileX, tileY);
                } else {
                    if (tileX > 0 && tileY > 0) {
                        // Only close old tile if it was actually on the playfield
                        drawPlayfieldTile(tileX, tileY);
                    }
                    drawPlayfieldTileSelected(tX, tY);
                }
            } else {
                // current position is not in the playfield
                if (tileX > 0 && tileY > 0) {
                    // Only close old tile if it was actually on the playfield
                    drawPlayfieldTile(tileX, tileY);
                }
            }
            tileX = tX;
            tileY = tY;
        }

        // Process messages
        while(msg = (struct IntuiMessage *)GetMsg(window->UserPort)) {
            ULONG cls = msg->Class; 
            // printf("class = %08x\n", cls);
            switch (cls) {
            case CLOSEWINDOW:
                terminate = TRUE;
                break;
            case GADGETUP:
                {
                    struct Gadget *gadget = (struct Gadget *) msg->IAddress;
                    int gadgetId = gadget->GadgetID;
                    switch(gadgetId) {
                        case GADGET_ID_QUIT:
                            terminate = TRUE;
                            break;
                        case GADGET_ID_FACE:
                            startGame(difficulty);
                            break;
                        case GADGET_ID_DIFFICULTY:
                            uiUpdateRadioGroup(&uiDifficultyRadioGroup, gadget);
                            startGame((int)gadget->UserData);
                            break;
                        default:
                            printf("*** Unknown gadget %d clicked", gadgetId);
                    }
                }
            case MOUSEBUTTONS:
                if (!game.running) {
                    break;
                }
                switch(msg->Code) {
                    case SELECTDOWN:
                        handleLmbDown(msg);
                        break;
                    case SELECTUP:
                        // LMB
                        handleLmbUp(msg);
                        break;
                    case MENUDOWN:
                        handleRmbDown(msg);
                        break;
                }
                break;
            }
            ReplyMsg((struct Message *)msg);
        }
    }

    // Remove Copperlist
    struct ViewPort *viewPort = &screen->ViewPort;
    FreeVPortCopLists(viewPort);
    RemakeDisplay();

    uiClose();

    CloseLibrary((struct Library*)GfxBase);
    CloseLibrary((struct Library*)IntuitionBase);

    debug_shutdown();

    return 0;
}
