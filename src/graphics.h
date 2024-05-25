#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <proto/graphics.h>

//#include <intuition/intuition.h>

#define COL_GRAY 0
#define COL_DGRAY 4
#define COL_LGRAY 6
#define COL_BLACK 2
#define COL_WHITE 12

void border(struct RastPort *rp, int x, int y, int w, int h, int d, int colorTopLeft, int colorBottomRight, int colorBetween);

#endif