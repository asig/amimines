#include "ui.h"

#include <exec/types.h>
#include <clib/dos_protos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include <stdio.h>

#include "images.h"
#include "layout.h"

struct Screen *screen;
struct Window *window;

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

static struct Gadget quitGadget = {
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

void uiCreate() {
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

void uiClose() {
    CloseWindow(window);
    CloseScreen(screen);
}
