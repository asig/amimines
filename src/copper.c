#include "copper.h"

#include <clib/dos_protos.h>
#include <clib/graphics_protos.h>
#include <exec/types.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <hardware/custom.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/exec.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "debug.h"
#include "ui.h"
#include "images.h"

extern struct Custom custom;

#define COPPER_LINES 32
#define COPPER_FIRST_LINE 1

static UWORD *copperInstr;

static UWORD redBand[2*COPPER_LINES];
static UWORD greenBand[2*COPPER_LINES];
static UWORD blueBand[2*COPPER_LINES];

/* Generated using this python code:
import math;
x_values = [2 * math.pi * i / 256 for i in range(256)]
sin_values = [int((1+math.sin(x))*16) for x in x_values]
print(sin_values)
*/
static USHORT sinTab[256] = {
    16, 16, 16, 17, 17, 17, 18, 18, 19, 19, 19, 20, 20, 21, 21, 21, 22, 22, 22, 23, 23, 23, 24, 24, 24, 25, 25, 25, 26, 26, 26, 27, 27, 27, 27, 28, 28, 28, 28, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 32, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 30, 30, 30, 30, 30, 30, 29, 29, 29, 29, 29, 28, 28, 28, 28, 27, 27, 27, 27, 26, 26, 26, 25, 25, 25, 24, 24, 24, 23, 23, 23, 22, 22, 22, 21, 21, 21, 20, 20, 19, 19, 19, 18, 18, 17, 17, 17, 16, 16, 16, 15, 15, 14, 14, 14, 13, 13, 12, 12, 12, 11, 11, 10, 10, 10, 9, 9, 9, 8, 8, 8, 7, 7, 7, 6, 6, 6, 5, 5, 5, 4, 4, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 12, 12, 12, 13, 13, 14, 14, 14, 15, 15
};

static USHORT speedTable[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -3, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static UBYTE redPos;
static UBYTE redSpeedPos;
static int redAcc;

static UBYTE greenPos;
static UBYTE greenSpeedPos;
static int greenAcc;

static UBYTE bluePos;
static UBYTE blueSpeedPos;
static int blueAcc;

void initColorBands() {
    for (int i = 0; i < COPPER_LINES/2; i++) {
        redBand[i] = 
        redBand[COPPER_LINES-1-i] =
        redBand[COPPER_LINES + i] =
        redBand[2*COPPER_LINES-1-i] = i<<8;

        greenBand[i] = 
        greenBand[COPPER_LINES-1-i] =
        greenBand[COPPER_LINES + i] =
        greenBand[2*COPPER_LINES-1-i] = i<<4;

        blueBand[i] = 
        blueBand[COPPER_LINES-1-i] =
        blueBand[COPPER_LINES + i] =
        blueBand[2*COPPER_LINES-1-i] = i;
    }
}

void copperAnimate() {
    static int pos = 0;
    UWORD *copWord = copperInstr + 1;
    UWORD *rp = &redBand[sinTab[redPos]];
    UWORD *gp = &greenBand[sinTab[greenPos]];
    UWORD *bp = &blueBand[sinTab[bluePos]];
    for (int i = 0; i < COPPER_LINES; i++) {
        *copWord = *(rp++) | *(gp++) | *(bp++);
        copWord += 4;
    }
    redPos += speedTable[redSpeedPos];
    greenPos += speedTable[greenSpeedPos];
    bluePos += speedTable[blueSpeedPos];
    redSpeedPos += redAcc;
    greenSpeedPos += greenAcc;
    blueSpeedPos += blueAcc;

    // pos = (pos+1)%COPPER_LINES;
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

void copperCreateList() {
    // Init all the tables and variables we need
    initColorBands();
    redPos = rand() & 0xff;
    redSpeedPos = rand() & 0xff;
    redAcc = (rand() % 2 + 1) * (rand()%2 ? 1 : -1); // possible accs: -2, -1, 1, 2
    greenPos = rand() & 0xff;
    greenSpeedPos = rand() & 0xff;
    greenAcc = (rand() % 2 + 1) * (rand()%2 ? 1 : -1);
    bluePos = rand() & 0xff;
    blueSpeedPos = rand() & 0xff;
    blueAcc = (rand() % 2 + 1) * (rand()%2 ? 1 : -1);

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

