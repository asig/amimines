#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <graphics/copper.h>
#include <devices/inputevent.h>
#include <intuition/intuition.h>
#include <hardware/custom.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "game.h"
#include "graphics.h"
#include "images.h"
#include "debug.h"

#define HEADER_X 4
#define HEADER_Y 0
#define HEADER_BORDER 4
#define HEADER_MARGIN 5
#define HEADER_H (2*HEADER_BORDER+2*HEADER_MARGIN+23+2)
#define HEADER_W (HEADER_BORDER+HEADER_MARGIN+(39+2)+HEADER_MARGIN+24+HEADER_MARGIN+(39+2)+HEADER_MARGIN+HEADER_BORDER)

#define REMAINING_MINES_X (HEADER_X+HEADER_BORDER+HEADER_MARGIN+1)
#define REMAINING_MINES_Y (HEADER_Y+HEADER_BORDER+HEADER_MARGIN+1)

#define TIMER_X (HEADER_X+HEADER_W-HEADER_BORDER-HEADER_MARGIN-39-1)
#define TIMER_Y (HEADER_Y+HEADER_BORDER+HEADER_MARGIN+1)

#define SMILEY_X (HEADER_X+HEADER_W/2-12)
#define SMILEY_Y (HEADER_Y+HEADER_H/2-12)

#define LOGO_X (HEADER_X+HEADER_W)
#define LOGO_Y 0
// #define LOGO_BORDER 4
// #define LOGO_MARGIN 5
// #define LOGO_H (2*HEADER_BORDER+2*HEADER_MARGIN+23+2)
// #define LOGO_W (320-HEADER_X-LOGO_X)
// #define LOGO_H (2*HEADER_BORDER+2*HEADER_MARGIN+23+2)
// #define LOGO_W (320-LOGO_X)

#define PLAYFIELD_BORDER 4
#define PLAYFIELD_X (HEADER_X + PLAYFIELD_BORDER)
#define PLAYFIELD_Y (HEADER_Y + HEADER_H + 20 + PLAYFIELD_BORDER)

extern struct Custom custom;
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Screen *screen;
struct Window *window;

struct Game game;

ULONG ticks;
USHORT terminate;
USHORT tileX;
USHORT tileY;
struct Tile *selectedTile;

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

void createWindow();
void doButtons(struct IntuiMessage *msg);
void drawRemainingMines(int mines);
void drawTimer(int secs);
void drawPlayfieldTile(int x, int y);
void drawPlayfield();
void createCopperList();

// void clearRemainingMines() {
//     DrawImage(window->RPort, &imgDigitEmpty, REMAINING_MINES_X, REMAINING_MINES_Y);
//     DrawImage(window->RPort, &imgDigitEmpty, REMAINING_MINES_X+1*imgDigit0.Width, REMAINING_MINES_Y);
//     DrawImage(window->RPort, &imgDigitEmpty, REMAINING_MINES_X+2*imgDigit0.Width, REMAINING_MINES_Y);
// }

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

void drawPlayfield() {
    for(int y = 1; y <= PLAYFIELD_H_TILES; y++) {
        for (int x = 1; x <= PLAYFIELD_W_TILES; x++) {
            drawPlayfieldTile(x,y);
        }
    }
}

struct NewScreen newScreen = {
    0,0,
    320, /* width */
    256, /* height (PAL version) */
    4, /* bitplances */
    1,2, /* detail pen, block pen */
    0, // ViewModes
    CUSTOMSCREEN | SCREENQUIET, // Type
    NULL, // Font
    NULL, // title
    NULL, // Gadgets
    NULL, // CustomBitmap
};

#define GADGET_QUIT 1
#define GADGET_FACE 2
struct Gadget quitGadget = {
    NULL, /* NextGadget */
    320-btnQuit_W, 0, /* LeftEdge, TopEdge */
    btnQuit_W, btnQuit_H, /* Width, Height */
    GADGIMAGE|GADGHIMAGE, /* Flags */
    RELVERIFY, /* Activation */
    BOOLGADGET, /* Gadget Type */
    (APTR)&btnQuit, /* GadgetRender */
    (APTR)&btnQuitPressed, /* Select Render */
    NULL, /* GadgetText */
    0, /* MutualExclude */
    NULL, /* Speciallnfo */
    GADGET_QUIT, /* GadgetID */
    NULL, /* UserData */
};
struct Gadget faceGadget = {
    &quitGadget, /* NextGadget */
    SMILEY_X, SMILEY_Y, /* LeftEdge, TopEdge */
    imgFaceNormal_W, imgFaceNormal_H, /* Width, Height */
    GADGIMAGE|GADGHIMAGE, /* Flags */
    RELVERIFY, /* Activation */
    BOOLGADGET, /* Gadget Type */
    (APTR)&imgFaceNormal, /* GadgetRender */
    (APTR)&imgFacePressed, /* Select Render */
    NULL, /* GadgetText */
    0, /* MutualExclude */
    NULL, /* Speciallnfo */
    GADGET_FACE, /* GadgetID */
    NULL, /* UserData */
};
struct NewWindow newWindow = {
    0, 0, /* X- und Y-Position */
    320, 256, /* Breite, H he */
    1, 2, /* Farben (0-15) */
    CLOSEWINDOW | MOUSEBUTTONS | GADGETUP, // | INTUITICKS,
    ACTIVATE | BORDERLESS | RMBTRAP , // | SMART_REFRESH | ACTIVATE | WINDOWSIZING | SIZEBRIGHT | WINDOWDRAG | WINDOWDEPTH | BORDERLESS,
    &faceGadget,
    NULL,
    NULL, // Title
    NULL, // Screen
    NULL,
    320, 256, // min size
    320, 256, // max size
    CUSTOMSCREEN
};

void createWindow() {
    screen = OpenScreen(&newScreen);
    if (!screen) {
        fprintf(stderr, "Can't open screen!");
        Exit(FALSE);
    }
    newWindow.Screen = screen;
    window = OpenWindow(&newWindow);
    if (!window) {
        fprintf(stderr, "Can't open window!");
        Exit(FALSE);
    }
    LoadRGB4(&screen->ViewPort, palette, NUM_COLS);
}

BOOL mouseToTile(SHORT mouseX, SHORT mouseY, USHORT *tileX, USHORT *tileY) {
    int tX = (mouseX - PLAYFIELD_X)/imgTile0_W;
    int tY = (mouseY - PLAYFIELD_Y)/imgTile0_H;
    if (tX >= 0 && tX < PLAYFIELD_W_TILES && tY >= 0 && tY < PLAYFIELD_H_TILES) {
        *tileX = tX + 1;
        *tileY = tY + 1;
        return TRUE;
    }
    return FALSE;
}

void diedAt(USHORT tx, USHORT ty) {
    game.running = FALSE;
    drawPlayfieldTileExplicit(tx,ty, &imgTileMineExploded);
    DrawImage(window->RPort, &imgFaceSad, SMILEY_X, SMILEY_Y);

    // Reveal all non-selected mines, highlight all false marked mines
    for (USHORT y = 1; y < PLAYFIELD_H_TILES; y++) {
       for (USHORT x = 1; x < PLAYFIELD_W_TILES; x++) {
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
    tile->state = TILE_OPEN;
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

void handleLmbDown(struct IntuiMessage *msg) {
    // LMB. Check if we click a tile
    if (mouseToTile(msg->MouseX, msg->MouseY, &tileX, &tileY)) {
        // Click is in a tile. Check the state
        selectedTile = &game.tiles[tileY][tileX];
        if (selectedTile->state != TILE_CLOSED) {
            // Ah, nothing to do :-(
            selectedTile = NULL;
            return;
        }
        DrawImage(window->RPort, &imgFaceO, SMILEY_X, SMILEY_Y);
        drawPlayfieldTileExplicit(tileX, tileY, &imgTile0);
    }
}

void handleLmbUp(struct IntuiMessage *msg) {
    DrawImage(window->RPort, &imgFaceNormal, SMILEY_X, SMILEY_Y);
    if (!selectedTile) {
        return;
    }

    game.timerRunning = TRUE;

    USHORT tX, tY;
    if (mouseToTile(msg->MouseX, msg->MouseY, &tX, &tY)) {
        if (tileX == tX && tileY == tY) {
            openTile(tileX,tileY);
        }
    }
    selectedTile = NULL;
    // drawPlayfieldTile(tileX, tileY);
}

void startGame() {
    newGame(&game, PLAYFIELD_H_TILES*PLAYFIELD_W_TILES*.1); // 10% mines
    drawRemainingMines(game.unmarkedMines);
    drawTimer(game.ticks/50);
    drawPlayfield();
    selectedTile = NULL;
}

int main(int argc, char **argv) {
    printf("LOGO X1: %d\n", HEADER_X+HEADER_W);
    printf("LOGO Y2: %d\n", PLAYFIELD_Y - PLAYFIELD_BORDER);


    srand(time(NULL));
    debug_init();

    ticks = 0;
    
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

    createWindow();

    createCopperList();

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
        WaitTOF();
        
        if (game.timerRunning) {
            game.ticks++;
            drawTimer(game.ticks/50);
            drawRemainingMines(game.unmarkedMines);
        }

        if (selectedTile) {
            // Check if the mouse is still over the tile
            BOOL sameTile = FALSE;
            USHORT tX, tY;
            if (mouseToTile(window->MouseX, window->MouseY, &tX, &tY)) {
                sameTile = tX == tileX && tY == tileY;
            } 
            drawPlayfieldTileExplicit(tileX, tileY, sameTile ? &imgTile0 : &imgTileClosed);
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
                        case GADGET_QUIT:
                            terminate = TRUE;
                            break;
                        case GADGET_FACE:
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
                }
                break;
            }
            ReplyMsg((struct Message *)msg);
        }
    }

    // Remove Copperlist
    struct ViewPort *viewPort = &screen->ViewPort; //ViewPortAddress(window);     /*  Get a pointer to the ViewPort.  */
    FreeVPortCopLists(viewPort);
    RemakeDisplay();

    CloseWindow(window);
    CloseScreen(screen);
    CloseLibrary((struct Library*)GfxBase);
    CloseLibrary((struct Library*)IntuitionBase);

    printf("Minesweeper ran for %ld ticks\n", ticks);

    debug_shutdown();

    struct Tile tile[4];
    struct Tile2 tile2[4];
    printf("Tile: size = %d\n", sizeof(tile));
    printf("Tile2: size = %d\n", sizeof(tile2));
    return 0;
}

// int main(int argc, char **argv) {
//     printf("Hello! You passed these args:\n");
//     int i = 0;
//     while (*argv) {
//         printf("%d: %s\n", i++, *argv++);
//     }
//     return 0;
// }

void dumpCopperList() {
    UWORD *instr = GfxBase->LOFlist;
    
    for(;;) {
        UWORD w1 = *(instr++);
        UWORD w2 = *(instr++);
        if ((w1 & 1) == 0) {        
            printf("MOVE $%04x, $%04x\n", w1, w2);            
        } else if ((w1 & 1) == 1) {
             printf("MOVE $%04x, $%04x\n", w1, w2);                        
        }
    }

}

void createCopperList() {
register USHORT   i, scanlines_per_color;
         WORD     ret_val    = RETURN_OK;
struct   ViewPort *viewPort;
struct   UCopList *uCopList  = NULL;

UWORD    spectrum[] =
          {
                0x0604, 0x0605, 0x0606, 0x0607, 0x0617, 0x0618, 0x0619,
                0x0629, 0x072a, 0x073b, 0x074b, 0x074c, 0x075d, 0x076e,
                0x077e, 0x088f, 0x07af, 0x06cf, 0x05ff, 0x04fb, 0x04f7,
                0x03f3, 0x07f2, 0x0bf1, 0x0ff0, 0x0fc0, 0x0ea0, 0x0e80,
                0x0e60, 0x0d40, 0x0d20, 0x0d00
          };

#define NUMCOLORS 32

        /*  Allocate memory for the Copper list.  */
        /*  Make certain that the initial memory is cleared.  */
        uCopList = (struct UCopList *) AllocMem(sizeof(struct UCopList), MEMF_PUBLIC|MEMF_CLEAR);
        if (!uCopList) {
            fprintf(stderr, "Not enough memory to allocate copperlist");
            Exit(FALSE);
        }

        /*  Initialize the Copper list buffer.  */
        CINIT(uCopList, NUMCOLORS);

        scanlines_per_color = screen->Height/NUMCOLORS;

        /*  Load in each color.  */
        for (i=0; i<NUMCOLORS; i++) {
            CWAIT(uCopList, i, 0);
            CMOVE(uCopList, custom.color[0], spectrum[i]);
        }
        CWAIT(uCopList, i, 0);
        CMOVE(uCopList, custom.color[0], palette[0]);

        CEND(uCopList); /*  End the Copper list  */

        viewPort = &screen->ViewPort; //ViewPortAddress(window);     /*  Get a pointer to the ViewPort.  */
        Forbid();       /*  Forbid task switching while changing the Copper list.  */
        viewPort->UCopIns=uCopList;
        Permit();       /*  Permit task switching again.  */

        RethinkDisplay();       /*  Display the new Copper list.  */

        // dumpCopperList();
}
