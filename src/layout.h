#ifndef LAYOUT_H_
#define LAYOUT_H

#include "images.h"

// Header and stats
#define HEADER_X 4
#define HEADER_Y 0
#define HEADER_BORDER 3
#define HEADER_MARGIN 3
#define HEADER_H (2 * HEADER_BORDER + 2 * HEADER_MARGIN + 23 + 2)
#define HEADER_W                                                               \
  (HEADER_BORDER + HEADER_MARGIN + (39 + 2) + HEADER_MARGIN + 24 +             \
   HEADER_MARGIN + (39 + 2) + HEADER_MARGIN + HEADER_BORDER)

#define REMAINING_MINES_X (HEADER_X + HEADER_BORDER + HEADER_MARGIN + 1)
#define REMAINING_MINES_Y (HEADER_Y + HEADER_BORDER + HEADER_MARGIN + 1)

#define TIMER_X (HEADER_X + HEADER_W - HEADER_BORDER - HEADER_MARGIN - 39 - 1)
#define TIMER_Y (HEADER_Y + HEADER_BORDER + HEADER_MARGIN + 1)

#define SMILEY_X (HEADER_X + HEADER_W / 2 - 12)
#define SMILEY_Y (HEADER_Y + HEADER_H / 2 - 12)

// Logo and level checkboxes
#define LOGO_X (320 - imgLogo_W)
#define LOGO_Y 0

#define CHECKBOX_NOVICE_X (LOGO_X + 22)
#define CHECKBOX_NOVICE_Y (LOGO_Y + 31)

#define CHECKBOX_INTERMEDIATE_X (LOGO_X + 68)
#define CHECKBOX_INTERMEDIATE_Y (LOGO_Y + 31)

#define CHECKBOX_EXPERT_X (LOGO_X + 114)
#define CHECKBOX_EXPERT_Y (LOGO_Y + 31)

// Max sizes for playfield
#define PLAYFIELD_BORDER 4
#define PLAYFIELD_X (HEADER_X + PLAYFIELD_BORDER)
#define PLAYFIELD_Y (256 - PLAYFIELD_H_TILES * imgTile0_H - PLAYFIELD_BORDER)

#endif