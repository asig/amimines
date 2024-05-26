#ifndef UI_H
#define UI_H

#define COL_GRAY 0
#define COL_DGRAY 4
#define COL_LGRAY 6
#define COL_BLACK 2
#define COL_WHITE 12

#define GADGET_ID_QUIT 1
#define GADGET_ID_FACE 2
#define GADGET_ID_DIFFICULTY 3

#define DIFFUCLTY_NOVICE 0
#define DIFFUCLTY_INTERMEDIATE 1
#define DIFFUCLTY_EXPERT 2

extern struct Screen *screen;
extern struct Window *window;

struct RadioGroup {
    struct Gadget **gadgets;
    int nofGadgets;
    struct Gadget *selected;
};

extern struct RadioGroup uiDifficultyRadioGroup;

void uiUpdateRadioGroup(struct RadioGroup *grp, struct Gadget *newSelection);

void uiCreate();
void uiClose();

#endif