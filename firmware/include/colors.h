#pragma once

#include <TFT_eSPI.h>

// ============================================================
// LCARS Color Palette (Star Trek TNG authentic)
// All values are 16-bit RGB565
// ============================================================

// Background - pure black (LCARS standard)
#define CLR_BG            TFT_BLACK

// Primary LCARS colors (from TNG reference)
#define CLR_AMBER         0xFC80    // #FF9900 - primary amber/gold
#define CLR_SALMON        0xFBCF    // #FF7777 - salmon/pink panels
#define CLR_PEACH         0xFE93    // #FFCC99 - peach/tan text & labels
#define CLR_LAVENDER      0xCC9E    // #CC99CC - lavender/purple panels
#define CLR_BLUE          0x9CDF    // #9999FF - blue panels/highlights
#define CLR_ORANGE        0xFB20    // #FF6600 - orange accent
#define CLR_RED           0xC888    // #CC4444 - red alert
#define CLR_SKY           0x9E7F    // #99CCFF - sky blue / info
#define CLR_TAN           0xD5B6    // #D4AA77 - tan bars

// Text colors
#define CLR_TEXT          CLR_PEACH  // warm LCARS text (default)
#define CLR_TEXT_BRIGHT   0xFFFF    // #FFFFFF - emphasis
#define CLR_TEXT_DIM      0x4A49    // #4C4C4C - inactive

// Functional
#define CLR_BAR_BG        0x18C3   // #1A1A1A - unfilled bar track
#define CLR_STATUS_OK     CLR_SKY
#define CLR_STATUS_ERR    CLR_RED
