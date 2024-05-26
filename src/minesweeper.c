#include <clib/dos_protos.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <hardware/custom.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "game.h"
#include "graphics.h"
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

void startGame() {
    newGame(&game, PLAYFIELD_H_TILES*PLAYFIELD_W_TILES*.1); // 10% mines
    newGame(&game, 5); // 10% mines
    drawRemainingMines(game.unmarkedMines);
    drawTimer(game.ticks/50);
    drawPlayfield();
    trackingMouse = FALSE;
}

// int main(int argc, char **argv) {
//     printf("Hello! You passed these args:\n");
//     int i = 0;
//     while (*argv) {
//         printf("%d: %s\n", i++, *argv++);
//     }
//     return 0;
// }

UWORD *copperInstr;
#define COPPER_LINES 32
#define COPPER_FIRST_LINE 1

void cycleCopper() {
    static UWORD spectrum[] =
          {
                0x0604, 0x0605, 0x0606, 0x0607, 0x0617, 0x0618, 0x0619, 0x0629,
                0x072a, 0x073b, 0x074b, 0x074c, 0x075d, 0x076e, 0x077e, 0x088f,
                0x07af, 0x06cf, 0x05ff, 0x04fb, 0x04f7, 0x03f3, 0x07f2, 0x0bf1,
                0x0ff0, 0x0fc0, 0x0ea0, 0x0e80, 0x0e60, 0x0d40, 0x0d20, 0x0d00,
                0x0604, 0x0605, 0x0606, 0x0607, 0x0617, 0x0618, 0x0619, 0x0629,
                0x072a, 0x073b, 0x074b, 0x074c, 0x075d, 0x076e, 0x077e, 0x088f,
                0x07af, 0x06cf, 0x05ff, 0x04fb, 0x04f7, 0x03f3, 0x07f2, 0x0bf1,
                0x0ff0, 0x0fc0, 0x0ea0, 0x0e80, 0x0e60, 0x0d40, 0x0d20, 0x0d00
          }
          ;

    static int pos = 0;
    UWORD *copWord = copperInstr + 1;
    for (int i = 0; i < COPPER_LINES; i++) {
        *copWord = spectrum[pos+i];
        copWord += 4;
    }
    pos = (pos+1)%COPPER_LINES;
}

void dumpCopperList() {
    debug_print("Copperlist:","");
    UWORD *instr = GfxBase->LOFlist;
    for(;;) {
        UWORD w1 = *instr;
        UWORD w2 = *(instr+1);
        debug_print("$%08x: $%04x, $%04x", instr, w1,w2);
        if (w1 == 0xffff && w2 == 0xfffe) {
            break;
        }
        instr+=2;
    }
}

void createCopperList() {
    /*  Allocate memory for the Copper list.  */
    /*  Make certain that the initial memory is cleared.  */
    struct UCopList *uCopList = (struct UCopList *) AllocMem(sizeof(struct UCopList), MEMF_PUBLIC|MEMF_CLEAR);
    if (!uCopList) {
        fprintf(stderr, "Not enough memory to allocate copperlist");
        Exit(FALSE);
    }

    /*  Initialize the Copper list buffer.  */
    CINIT(uCopList, 2*(COPPER_LINES+1)); // CWAIT and CMOVE per color.

    /*  Load in each color.  */
    int i;
    for (i=0; i<COPPER_LINES; i++) {
        CWAIT(uCopList, COPPER_FIRST_LINE + i, 0);
        CMOVE(uCopList, custom.color[15], 0x1234);
    }
    CWAIT(uCopList, COPPER_FIRST_LINE + i, 0);
    CMOVE(uCopList, custom.color[0], palette[0]);

    CEND(uCopList); /*  End the Copper list  */

    Forbid();
    screen->ViewPort.UCopIns = uCopList;
    Permit();

    RethinkDisplay();       /*  Display the new Copper list.  */

    // No, find the first of our CMOVEs
    UWORD *instr = GfxBase->LOFlist;
    for(;;) {
        UWORD w1 = *instr;
        UWORD w2 = *(instr+1);
        if (w1 == 0x19e && w2 == 0x1234) {
            // CMOVE found!
            copperInstr = instr;
            break;
        }
        instr+=2;
    }

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

    uiCreate();

    createCopperList();
    cycleCopper();
    // debug_print("copperInstr = $%08x", copperInstr);
    // dumpCopperList();

    // Draw header part 
    border(window->RPort, HEADER_X, HEADER_Y, HEADER_W, HEADER_H, HEADER_BORDER, COL_DGRAY, COL_LGRAY, COL_GRAY);
    border(window->RPort, REMAINING_MINES_X-1, REMAINING_MINES_Y-1, 39+2,23+2,1,COL_DGRAY, COL_LGRAY, COL_GRAY);
    border(window->RPort, TIMER_X-1, TIMER_Y-1, 39+2,23+2,1,COL_DGRAY, COL_LGRAY, COL_GRAY);
    DrawImage(window->RPort, &imgFaceNormal, SMILEY_X, SMILEY_Y);

    // Draw Logo part
    DrawImage(window->RPort, &imgLogo, LOGO_X, LOGO_Y);
    // border(window->RPort, LOGO_X, LOGO_Y, LOGO_W, LOGO_H, LOGO_BORDER, COL_DGRAY, COL_LGRAY, COL_GRAY);

    // Draw playfield border and playfield
    border(
        window->RPort, 
        PLAYFIELD_X - PLAYFIELD_BORDER, 
        PLAYFIELD_Y - PLAYFIELD_BORDER, 
        PLAYFIELD_W_TILES*16 + 2*PLAYFIELD_BORDER, 
        PLAYFIELD_H_TILES*16 + 2*PLAYFIELD_BORDER, 
        PLAYFIELD_BORDER, 
        COL_DGRAY, COL_LGRAY, COL_GRAY);

    startGame();

    struct IntuiMessage *msg;
    terminate = FALSE;
    while(!terminate) {
        // cycleCopper();
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
                    int gadgetId = ((struct Gadget *) msg->IAddress)->GadgetID;
                    switch(gadgetId) {
                        case GADGET_ID_QUIT:
                            terminate = TRUE;
                            break;
                        case GADGET_ID_FACE:
                            startGame();
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
