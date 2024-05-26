#include "ui.h"

#include <exec/types.h>
#include <clib/dos_protos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include <stdio.h>

#include "images.h"
#include "layout.h"
#include "game.h"
#include "debug.h"

struct Screen *screen;
struct Window *window;
struct RadioGroup uiDifficultyRadioGroup;

static struct NewScreen newScreen = {
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

static struct Gadget checkboxExpertGadget = {
    NULL, /* NextGadget */
    LOGO_X + 114, LOGO_Y+31, /* LeftEdge, TopEdge */
    checkboxUnselected_W, checkboxUnselected_H, /* Width, Height */
    GADGIMAGE|GADGHIMAGE, /* Flags */
    RELVERIFY|TOGGLESELECT, /* Activation */
    BOOLGADGET, /* Gadget Type */
    (APTR)&checkboxUnselected, /* GadgetRender */
    (APTR)&checkboxSelected, /* Select Render */
    NULL, /* GadgetText */
    0, /* MutualExclude */
    NULL, /* Speciallnfo */
    GADGET_ID_DIFFICULTY, /* GadgetID */
    (APTR)DIFFUCLTY_EXPERT, /* UserData */
};

static struct Gadget checkboxIntermediateGadget = {
    &checkboxExpertGadget, /* NextGadget */
    LOGO_X + 68, LOGO_Y+31, /* LeftEdge, TopEdge */
    checkboxUnselected_W, checkboxUnselected_H, /* Width, Height */
    GADGIMAGE|GADGHIMAGE, /* Flags */
    RELVERIFY|TOGGLESELECT, /* Activation */
    BOOLGADGET, /* Gadget Type */
    (APTR)&checkboxUnselected, /* GadgetRender */
    (APTR)&checkboxSelected, /* Select Render */
    NULL, /* GadgetText */
    0, /* MutualExclude */
    NULL, /* Speciallnfo */
    GADGET_ID_DIFFICULTY, /* GadgetID */
    (APTR)DIFFUCLTY_INTERMEDIATE, /* UserData */
};

static struct Gadget checkboxNoviceGadget = {
    &checkboxIntermediateGadget, /* NextGadget */
    LOGO_X + 22, LOGO_Y+31, /* LeftEdge, TopEdge */
    checkboxUnselected_W, checkboxUnselected_H, /* Width, Height */
    GADGIMAGE|GADGHIMAGE, /* Flags */
    RELVERIFY|TOGGLESELECT, /* Activation */
    BOOLGADGET, /* Gadget Type */
    (APTR)&checkboxUnselected, /* GadgetRender */
    (APTR)&checkboxSelected, /* Select Render */
    NULL, /* GadgetText */
    0, /* MutualExclude */
    NULL, /* Speciallnfo */
    GADGET_ID_DIFFICULTY, /* GadgetID */
    (APTR)DIFFUCLTY_NOVICE, /* UserData */
};

static struct Gadget quitGadget = {
    &checkboxNoviceGadget, /* NextGadget */
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
    GADGET_ID_QUIT, /* GadgetID */
    NULL, /* UserData */
};

static struct Gadget faceGadget = {
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
    GADGET_ID_FACE, /* GadgetID */
    NULL, /* UserData */
};

static struct NewWindow newWindow = {
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

static struct Gadget *difficultyGadgets[] = {
    &checkboxNoviceGadget,
    &checkboxIntermediateGadget,
    &checkboxExpertGadget,
};

void border(struct RastPort *rp, int x, int y, int w, int h, int d, int colorTopLeft, int colorBottomRight, int colorBetween) {
    int x1 = x;
    int y1 = y;
    int x2 = x+w-1;
    int y2 = y+h-1;
    SetAPen(rp, colorTopLeft);
    for (int i = 0; i < d; i++) {
        Move(rp, x1+i, y2-i);
        Draw(rp, x1+i, y1+i);
        Draw(rp, x2-i, y1+i);
    }
    SetAPen(rp, colorBottomRight);
    for (int i = 0; i < d; i++) {
        Move(rp, x1+i, y2-i);
        Draw(rp, x2-i, y2-i);
        Draw(rp, x2-i, y1+i);
    }
    SetAPen(rp, colorBetween);
    Move(rp, x1, y2);
    Draw(rp, x1+d, y2-d);
    Move(rp, x2, y1);
    Draw(rp, x2-d, y1+d);
}

void uiUpdateRadioGroup(struct RadioGroup *grp, struct Gadget *newSelection) {
    // Remove selection from previous selection
    struct Gadget *g = grp->selected;
    RemoveGadget(window, g);
    if (newSelection != grp->selected) {
        g->Flags &= ~SELECTED; 
    } else {
        // User clicked selected item again -> make sure it stays selected
        g->Flags |= SELECTED; 
    }
    AddGadget(window, g, -1);

    RefreshGadgets(grp->gadgets[0], window, NULL);

    grp->selected = newSelection;
}

void uiCreate(int startDifficulty) {
    difficultyGadgets[startDifficulty]->Flags |= SELECTED;
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

    uiDifficultyRadioGroup.gadgets = difficultyGadgets;
    uiDifficultyRadioGroup.nofGadgets = sizeof(difficultyGadgets)/sizeof(struct Gadget *);
    uiDifficultyRadioGroup.selected = difficultyGadgets[startDifficulty];    

    RefreshGadgets(window->FirstGadget, window, NULL);
}

void uiClose() {
    CloseWindow(window);
    CloseScreen(screen);
}
