#ifndef UI_H
#define UI_H

#define GADGET_ID_QUIT 1
#define GADGET_ID_FACE 2

extern struct Screen *screen;
extern struct Window *window;

void uiCreate();
void uiClose();

#endif