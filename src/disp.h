// disp.h - Display and UI module header
#ifndef DISP_H
#define DISP_H

#include "config.h"
#include <lvgl.h>

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
