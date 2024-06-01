/*
 * Copyright (c) 2024 Andreas Signer <asigner@gmail.com>
 *
 * This file is part of AmiMines.
 *
 * AmiMines is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * AmiMines is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AmiMines.  If not, see <https://www.gnu.org/licenses/>.
 */

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