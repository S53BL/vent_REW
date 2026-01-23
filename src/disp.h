// disp.h - Display and UI module header
#ifndef DISP_H
#define DISP_H

#include "config.h"
#include <lvgl.h>

// Room ID enum
enum RoomId { ROOM_EXT = 0, ROOM_TIME_WIFI, ROOM_WC, ROOM_UT, ROOM_KOP, ROOM_DS, ROOM_S, ROOM_B };

// LVGL objects
extern lv_obj_t* tabview;
extern lv_obj_t* tab_rooms;
extern lv_obj_t* tab_graphs;
extern lv_obj_t* chart;
extern lv_obj_t* cards[8];
extern lv_obj_t* weather_icon;

// Function declarations
bool initDisplay();
void updateUI();
void createTabs();
void updateCards();
void cycleGraph();
void updateWeatherIcon();

#endif // DISP_H
