#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <graphics/gfxbase.h>
#include <devices/inputevent.h>
#include <intuition/intuition.h>

#include <stdio.h>

struct IntuitionBase *IntuitionBase;

struct NewScreen newScreen = {
    0,0,
    320, /* width */
    256, /* height (PAL version) */
    3, /* bitplances */
    1,2, /* detail pen, block pen */
    0, // ViewModes
    CUSTOMSCREEN | SCREENQUIET, // Type
    NULL, // Font
    NULL, // title
    NULL, // Gadgets
    NULL, // CustomBitmap
};

struct NewWindow newWindow = {
    40, 40, /* X- und Y-Position */
    280, 120, /* Breite, H he */
    1, 2, /* Farben (0-7) */
    CLOSEWINDOW | MOUSEMOVE | MOUSEBUTTONS,
    ACTIVATE | BORDERLESS | RMBTRAP | WINDOWCLOSE, // | SMART_REFRESH | ACTIVATE | WINDOWSIZING | SIZEBRIGHT | WINDOWDRAG | WINDOWDEPTH | BORDERLESS,
    NULL,
    NULL,
    "*** Hallo ***",
    NULL, /* Hier kommt sp ter der Screen-Pointer rein */
    NULL,
    190, 20,
    640, 256,
    CUSTOMSCREEN
};

void doButtons(struct IntuiMessage *msg);


char prt_buf[512];

int main(int argc, char **argv) {
    struct Screen *screen;
    struct Window *window;

    IntuitionBase = (struct IntuitionBase*)OpenLibrary("intuition.library", 0);
    if (!IntuitionBase) {
        fprintf(stderr, "Can't open intuition.library!");
        Exit(FALSE);
    }

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

    UBYTE *bitmap = screen->BitMap.Planes[0];
    for(int i = 0; i < 255; i++) {
        bitmap[i]=i;
    }
    
    struct IntuiMessage *msg;
    USHORT done = FALSE;
    while(!done) {
        Wait((1L<<window->UserPort->mp_SigBit));
        printf("event class: %ld\n", msg->Class);
        msg = (struct IntuiMessage *)GetMsg(window->UserPort);
        ULONG cls = msg->Class; 
        ReplyMsg((struct Message *)msg);
        switch (cls) {
        case CLOSEWINDOW:
            done = TRUE;
            break;
        /* NOTE NOTE NOTE:  If the mouse queue backs up a lot, Intuition
        ** will start dropping MOUSEMOVE messages off the end until the
        ** queue is serviced.  This may cause the program to lose some
        ** of the MOUSEMOVE events at the end of the stream.
        **
        ** Look in the window structure if you need the true position
        ** of the mouse pointer at any given time.  Look in the
        ** MOUSEBUTTONS message if you need position when it clicked.
        ** An alternate to this processing would be to set a flag that
        ** a mousemove event arrived, then print the position of the
        ** mouse outside of the "while (GetMsg())" loop.  This allows
        ** a single processing call for many mouse events, which speeds
        ** up processing A LOT!  Something like:
        **
        ** while (GetMsg())
        **    {
        **    if (class == IDCMP_MOUSEMOVE)
        **        mouse_flag = TRUE;
        **    ReplyMsg();   NOTE: copy out all needed fields first !
        **    }
        ** if (mouse_flag)
        **    {
        **    process_mouse_event();
        **    mouse_flag = FALSE;
        **    }
        **
        ** You can also use IDCMP_INTUITICKS for slower paced messages
        ** (all messages have mouse coordinates.)
        */
        // case MOUSEMOVE:
        //     /* Show the current position of the mouse relative to the
        //     ** upper left hand corner of our window
        //     */
        //     sprintf(prt_buf, "X%5d Y%5d", msg->MouseX, msg->MouseY);
        //     SetWindowTitles(window, prt_buf, prt_buf);
        //     break;
        case MOUSEBUTTONS:
            doButtons(msg);
            break;
        }
    }

    CloseWindow(window);
    CloseScreen(screen);
    CloseLibrary((struct Library*)IntuitionBase);

    return 0;
}


/*
** Show what mouse buttons where pushed
*/
void doButtons(struct IntuiMessage *msg) {
/* Yes, qualifiers can apply to the mouse also.  That is how
** we get the shift select on the Workbench.  This shows how
** to see if a specific bit is set within the qualifier
*/
if (msg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
    printf("Shift ");

switch (msg->Code)
    {
    case SELECTDOWN:
        printf("Left Button Down at X%d Y%d\n", msg->MouseX, msg->MouseY);
        break;
    case SELECTUP:
        printf("Left Button Up   at X%d Y%d\n", msg->MouseX, msg->MouseY);
        break;
    case MENUDOWN:
        printf("Right Button down at X%d Y%d\n", msg->MouseX, msg->MouseY);
        break;
    case MENUUP:
        printf("Right Button Up   at X%d Y%d\n", msg->MouseX, msg->MouseY);
        break;
    }
printf("\n");
}

// int main(int argc, char **argv) {
//     printf("Hello! You passed these args:\n");
//     int i = 0;
//     while (*argv) {
//         printf("%d: %s\n", i++, *argv++);
//     }
//     return 0;
// }
