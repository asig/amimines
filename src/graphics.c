#include "graphics.h"

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
