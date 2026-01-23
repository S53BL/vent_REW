// include/lv_conf.h

#ifndef LV_CONF_H
#define LV_CONF_H

/*================
 *     GENERAL
 *================*/

#define LV_COLOR_DEPTH     16      // 16-bit za ST7789 (RGB565)
#define LV_HOR_RES_MAX     320
#define LV_VER_RES_MAX     240
#define LV_DPI             96

#define LV_USE_LOG         0
#define LV_LOG_LEVEL       LV_LOG_LEVEL_INFO

#define LV_MEM_CUSTOM      0
#define LV_MEM_SIZE        (32U * 1024U)  // 32 KB, dovolj za demo

/*================
 *   FONTS
 *================*/
#define LV_FONT_MONTSERRAT_12    1
#define LV_FONT_MONTSERRAT_14    1
#define LV_FONT_MONTSERRAT_16    1
#define LV_FONT_MONTSERRAT_18    1
#define LV_FONT_MONTSERRAT_20    1
#define LV_FONT_MONTSERRAT_22    1
#define LV_FONT_MONTSERRAT_24    1
#define LV_FONT_MONTSERRAT_28    1

/*================
 *   WIDGETS
 *================*/
#define LV_USE_BTN         1
#define LV_USE_LABEL       1
#define LV_USE_IMG         1
#define LV_USE_SLIDER      1
#define LV_USE_CHART       1
#define LV_USE_BAR         1
#define LV_USE_ANIM        1
#define LV_USE_ANIMATION   1

/*================
 *   DRAWING
 *================*/
#define LV_DRAW_SW         1
#define LV_DRAW_SW_COMPLEX 1

/*================
 *   THEME
 *================*/
#define LV_USE_THEME_DEFAULT   1
#define LV_THEME_DEFAULT_DARK  1

/*================
 *   EXTRAS
 *================*/
#define LV_USE_ANIMIMG     0
#define LV_USE_CALENDAR    0
#define LV_USE_CHART       1
#define LV_USE_COLORWHEEL  0
#define LV_USE_IMGBTN      0
#define LV_USE_KEYBOARD    0
#define LV_USE_LED         0
#define LV_USE_LIST        0
#define LV_USE_MENU        0
#define LV_USE_METER       0
#define LV_USE_MSGBOX      0
#define LV_USE_ROLLER      0
#define LV_USE_SPAN        0
#define LV_USE_SPINBOX     0
#define LV_USE_SPINNER     0
#define LV_USE_TABVIEW     0
#define LV_USE_TILEVIEW    0
#define LV_USE_WIN         0

/*================
 *   FONT FORMATS
 *================*/
#define LV_FONT_FMT_TXT_LARGE  0
#define LV_FONT_FMT_SBITMAP    0

#endif /*LV_CONF_H*/
