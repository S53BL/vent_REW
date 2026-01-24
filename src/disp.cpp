// disp.cpp - Display and UI module implementation
#include "disp.h"
#include "globals.h"
#include "http.h"
#include <Display_ST7789.h>
#include <Touch_CST328.h>
#include <LVGL_Driver.h>
#include <WiFi.h>

// WiFi icons (16x16 px, placeholders - replace with actual PNG converted data)
const uint8_t green_wifi_bin[] = { /* TODO: Replace with actual green WiFi icon bin data */ };
const lv_img_dsc_t green_wifi = {
    {0, 0, 0, LV_IMG_CF_TRUE_COLOR},
    sizeof(green_wifi_bin),
    green_wifi_bin
};

const uint8_t red_wifi_bin[] = { /* TODO: Replace with actual red WiFi icon bin data */ };
const lv_img_dsc_t red_wifi = {
    {0, 0, 0, LV_IMG_CF_TRUE_COLOR},
    sizeof(red_wifi_bin),
    red_wifi_bin
};

// LVGL objects definitions
lv_obj_t* graph_container;
lv_obj_t* chart;
lv_obj_t* cards[8];
lv_obj_t* weather_icon;
lv_obj_t* wifi_icon;
lv_obj_t* bulb_icons[3]; // WC, UT, KOP bulb icons

const char* roomNames[8] = {"EXT", "TIME_WIFI", "WC", "UT", "KOP", "DS", "S", "B"};

// EXT labels
lv_obj_t* EXT_label1;
lv_obj_t* EXT_label2;
lv_obj_t* EXT_label3;
lv_obj_t* EXT_label4;

// TIME_WIFI labels
lv_obj_t* TIME_WIFI_label1;
lv_obj_t* TIME_WIFI_label2;
lv_obj_t* TIME_WIFI_label3;
lv_obj_t* TIME_WIFI_label4;

// WC labels
lv_obj_t* WC_label1;
lv_obj_t* WC_label2;

// UT labels
lv_obj_t* UT_label1;
lv_obj_t* UT_label2;
lv_obj_t* UT_label3;

// KOP labels
lv_obj_t* KOP_label1;
lv_obj_t* KOP_label2;
lv_obj_t* KOP_label3;

// DS labels
lv_obj_t* DS_label1;
lv_obj_t* DS_label2;
lv_obj_t* DS_label3;
lv_obj_t* DS_label4;
lv_obj_t* DS_label5;

// S and B labels
lv_obj_t* S_label1;
lv_obj_t* B_label1;

static lv_style_t style_card;
static lv_style_t style_text_main;
static lv_style_t style_text_secondary;

static void ext_event_cb(lv_event_t * e);
static void graph_event_cb(lv_event_t * e);
static void return_to_rooms(lv_timer_t* timer);
static void anim_zoom_cb(void * obj, int32_t v);
void updateWiFiIcon();

void createRoomCards();

bool initDisplay() {
    Serial.println("  Backlight_Init...");
    Backlight_Init();
    Serial.println("  LCD_Init...");
    LCD_Init();
    Serial.println("  Set_Backlight...");
    Set_Backlight(100);
    Serial.println("  Lvgl_Init...");
    Lvgl_Init();
    lv_obj_clean(lv_scr_act()); // to remove all demo objects on main screen

    // Set background
    Serial.println("  Setting background color...");
    lv_obj_t* screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xBDBDBD), 0); // RAL 7035 light gray for more visible gap
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_border_opa(screen, LV_OPA_TRANSP, 0);

    // Initialize styles
    Serial.println("  Initializing styles...");
    lv_style_init(&style_card);
    lv_style_set_radius(&style_card, CARD_RADIUS);
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
    lv_style_set_border_width(&style_card, 0);
    lv_style_set_pad_all(&style_card, 10);
    lv_style_set_pad_column(&style_card, 4);
    lv_style_set_pad_row(&style_card, 0);
    lv_style_set_shadow_width(&style_card, 8);
    lv_style_set_shadow_color(&style_card, lv_color_hex(0x333333));
    lv_style_set_shadow_opa(&style_card, LV_OPA_60);
    lv_style_set_shadow_ofs_x(&style_card, 3);
    lv_style_set_shadow_ofs_y(&style_card, 3);
    lv_style_set_shadow_spread(&style_card, 0);

    lv_style_init(&style_text_main);
    // lv_style_set_text_font(&style_text_main, lv_font_default);

    lv_style_init(&style_text_secondary);
    // lv_style_set_text_font(&style_text_secondary, lv_font_default);



    // Create UI containers
    Serial.println("  Creating UI containers...");
    graph_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(graph_container, LV_PCT(100), LV_PCT(100));
    lv_obj_add_flag(graph_container, LV_OBJ_FLAG_HIDDEN);

    // Create room cards
    createRoomCards();
    lv_obj_invalidate(lv_scr_act()); // Force redraw
    lv_refr_now(NULL); // Clear and refresh

    // Create graph content
    chart = lv_chart_create(graph_container);
    lv_obj_set_size(chart, 300, 200);
    lv_obj_center(chart);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, GRAPH_POINTS);
    lv_obj_add_event_cb(graph_container, graph_event_cb, LV_EVENT_CLICKED, NULL);
    // Add series for TEMP, HUM, CO2, FAN

    Serial.println("  Display init complete");
    return true;
}

static void button_event_cb(lv_event_t * e);

void createRoomCards() {
    // EXT
    cards[ROOM_EXT] = lv_btn_create(lv_scr_act());
    lv_obj_set_user_data(cards[ROOM_EXT], (void*)ROOM_EXT);
    lv_obj_set_pos(cards[ROOM_EXT], 2, 1);
    lv_obj_set_size(cards[ROOM_EXT], 173, 80);
    lv_obj_add_style(cards[ROOM_EXT], &style_card, 0);
    lv_obj_set_style_bg_color(cards[ROOM_EXT], lv_color_hex(BTN_EXT_COLOR), 0);


    lv_obj_add_event_cb(cards[ROOM_EXT], button_event_cb, LV_EVENT_ALL, reinterpret_cast<void*>(ROOM_EXT));
    lv_obj_add_event_cb(cards[ROOM_EXT], ext_event_cb, LV_EVENT_CLICKED, NULL);

    // EXT labels
    EXT_label1 = lv_label_create(cards[ROOM_EXT]);
    lv_obj_set_style_text_font(EXT_label1, FONT_14, 0);
    lv_obj_add_flag(EXT_label1, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(EXT_label1, "00.0°");
    lv_obj_align(EXT_label1, LV_ALIGN_TOP_LEFT, 5, 0);

    EXT_label2 = lv_label_create(cards[ROOM_EXT]);
    lv_obj_set_style_text_font(EXT_label2, FONT_14, 0);
    lv_obj_add_flag(EXT_label2, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(EXT_label2, "00.0%");
    lv_obj_align(EXT_label2, LV_ALIGN_TOP_LEFT, 5, 15);

    EXT_label3 = lv_label_create(cards[ROOM_EXT]);
    lv_obj_set_style_text_font(EXT_label3, FONT_14, 0);
    lv_obj_add_flag(EXT_label3, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(EXT_label3, "0000 hPa");
    lv_obj_align(EXT_label3, LV_ALIGN_TOP_LEFT, 5, 30);

    EXT_label4 = lv_label_create(cards[ROOM_EXT]);
    lv_obj_set_style_text_font(EXT_label4, FONT_14, 0);
    lv_obj_add_flag(EXT_label4, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(EXT_label4, "000 lx");
    lv_obj_align(EXT_label4, LV_ALIGN_TOP_LEFT, 5, 45);

    // Weather icon in EXT card
    weather_icon = lv_img_create(cards[ROOM_EXT]);
    lv_obj_align(weather_icon, LV_ALIGN_BOTTOM_RIGHT, -5, -5);

    // TIME_WIFI
    cards[ROOM_TIME_WIFI] = lv_btn_create(lv_scr_act());
    lv_obj_set_user_data(cards[ROOM_TIME_WIFI], (void*)ROOM_TIME_WIFI);
    lv_obj_set_pos(cards[ROOM_TIME_WIFI], 181, 1);
    lv_obj_set_size(cards[ROOM_TIME_WIFI], 137, 80);
    lv_obj_add_style(cards[ROOM_TIME_WIFI], &style_card, 0);
    lv_obj_set_style_bg_color(cards[ROOM_TIME_WIFI], lv_color_hex(BTN_TIME_WIFI_COLOR), 0);

    lv_obj_add_event_cb(cards[ROOM_TIME_WIFI], button_event_cb, LV_EVENT_ALL, reinterpret_cast<void*>(ROOM_TIME_WIFI));

    // TIME_WIFI labels
    TIME_WIFI_label1 = lv_label_create(cards[ROOM_TIME_WIFI]);
    lv_obj_set_style_text_font(TIME_WIFI_label1, FONT_14, 0);
    lv_obj_add_flag(TIME_WIFI_label1, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(TIME_WIFI_label1, "00:00");
    lv_obj_align(TIME_WIFI_label1, LV_ALIGN_TOP_LEFT, 5, 0);

    TIME_WIFI_label2 = lv_label_create(cards[ROOM_TIME_WIFI]);
    lv_obj_set_style_text_font(TIME_WIFI_label2, FONT_14, 0);
    lv_obj_add_flag(TIME_WIFI_label2, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(TIME_WIFI_label2, "00.00.00");
    lv_obj_align(TIME_WIFI_label2, LV_ALIGN_TOP_LEFT, 5, 15);

    TIME_WIFI_label3 = lv_label_create(cards[ROOM_TIME_WIFI]);
    lv_obj_set_style_text_font(TIME_WIFI_label3, FONT_14, 0);
    lv_obj_add_flag(TIME_WIFI_label3, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(TIME_WIFI_label3, "P=000 W");
    lv_obj_align(TIME_WIFI_label3, LV_ALIGN_TOP_LEFT, 5, 30);

    TIME_WIFI_label4 = lv_label_create(cards[ROOM_TIME_WIFI]);
    lv_obj_set_style_text_font(TIME_WIFI_label4, FONT_14, 0);
    lv_obj_add_flag(TIME_WIFI_label4, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(TIME_WIFI_label4, "E=0000 Wh");
    lv_obj_align(TIME_WIFI_label4, LV_ALIGN_TOP_LEFT, 5, 45);

    // WiFi icon placeholder
    wifi_icon = lv_img_create(cards[ROOM_TIME_WIFI]);
    lv_obj_align(wifi_icon, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

    // WC
    cards[ROOM_WC] = lv_btn_create(lv_scr_act());
    lv_obj_set_user_data(cards[ROOM_WC], (void*)ROOM_WC);
    lv_obj_set_pos(cards[ROOM_WC], 2, 85);
    lv_obj_set_size(cards[ROOM_WC], 101, 80);
    lv_obj_add_style(cards[ROOM_WC], &style_card, 0);
    lv_obj_set_style_bg_color(cards[ROOM_WC], lv_color_hex(BTN_WC_COLOR), 0);

    lv_obj_add_event_cb(cards[ROOM_WC], button_event_cb, LV_EVENT_ALL, reinterpret_cast<void*>(ROOM_WC));

    WC_label1 = lv_label_create(cards[ROOM_WC]);
    lv_obj_set_style_text_font(WC_label1, FONT_24, 0);
    lv_obj_add_flag(WC_label1, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(WC_label1, "WC");
    lv_obj_align(WC_label1, LV_ALIGN_TOP_LEFT, 5, 0);

    WC_label2 = lv_label_create(cards[ROOM_WC]);
    lv_obj_set_style_text_font(WC_label2, FONT_14, 0);
    lv_obj_add_flag(WC_label2, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(WC_label2, "0000 hPa");
    lv_obj_align(WC_label2, LV_ALIGN_TOP_LEFT, 5, 45);

    // WC bulb icon (wcLight=3, index 2)
    bulb_icons[2] = lv_img_create(cards[ROOM_WC]);
    lv_obj_align(bulb_icons[2], LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_flag(bulb_icons[2], LV_OBJ_FLAG_HIDDEN); // Default hidden

    // UT
    cards[ROOM_UT] = lv_btn_create(lv_scr_act());
    lv_obj_set_user_data(cards[ROOM_UT], (void*)ROOM_UT);
    lv_obj_set_pos(cards[ROOM_UT], 109, 85);
    lv_obj_set_size(cards[ROOM_UT], 101, 80);
    lv_obj_add_style(cards[ROOM_UT], &style_card, 0);
    lv_obj_set_style_bg_color(cards[ROOM_UT], lv_color_hex(BTN_UT_COLOR), 0);

    lv_obj_add_event_cb(cards[ROOM_UT], button_event_cb, LV_EVENT_ALL, reinterpret_cast<void*>(ROOM_UT));

    UT_label1 = lv_label_create(cards[ROOM_UT]);
    lv_obj_set_style_text_font(UT_label1, FONT_24, 0);
    lv_obj_add_flag(UT_label1, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(UT_label1, "UT");
    lv_obj_align(UT_label1, LV_ALIGN_TOP_LEFT, 5, 0);

    UT_label2 = lv_label_create(cards[ROOM_UT]);
    lv_obj_set_style_text_font(UT_label2, FONT_14, 0);
    lv_obj_add_flag(UT_label2, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(UT_label2, "00.0°");
    lv_obj_align(UT_label2, LV_ALIGN_TOP_LEFT, 5, 30);

    UT_label3 = lv_label_create(cards[ROOM_UT]);
    lv_obj_set_style_text_font(UT_label3, FONT_14, 0);
    lv_obj_add_flag(UT_label3, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(UT_label3, "00.0%");
    lv_obj_align(UT_label3, LV_ALIGN_TOP_LEFT, 5, 45);

    // UT bulb icon (utilityLight=1, index 0)
    bulb_icons[0] = lv_img_create(cards[ROOM_UT]);
    lv_obj_align(bulb_icons[0], LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_flag(bulb_icons[0], LV_OBJ_FLAG_HIDDEN); // Default hidden

    // KOP
    cards[ROOM_KOP] = lv_btn_create(lv_scr_act());
    lv_obj_set_user_data(cards[ROOM_KOP], (void*)ROOM_KOP);
    lv_obj_set_pos(cards[ROOM_KOP], 216, 85);
    lv_obj_set_size(cards[ROOM_KOP], 101, 80);
    lv_obj_add_style(cards[ROOM_KOP], &style_card, 0);
    lv_obj_set_style_bg_color(cards[ROOM_KOP], lv_color_hex(BTN_KOP_COLOR), 0);

    lv_obj_add_event_cb(cards[ROOM_KOP], button_event_cb, LV_EVENT_ALL, reinterpret_cast<void*>(ROOM_KOP));

    KOP_label1 = lv_label_create(cards[ROOM_KOP]);
    lv_obj_set_style_text_font(KOP_label1, FONT_24, 0);
    lv_obj_add_flag(KOP_label1, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(KOP_label1, "KOP");
    lv_obj_align(KOP_label1, LV_ALIGN_TOP_LEFT, 5, 0);

    KOP_label2 = lv_label_create(cards[ROOM_KOP]);
    lv_obj_set_style_text_font(KOP_label2, FONT_14, 0);
    lv_obj_add_flag(KOP_label2, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(KOP_label2, "00.0°");
    lv_obj_align(KOP_label2, LV_ALIGN_TOP_LEFT, 5, 30);

    KOP_label3 = lv_label_create(cards[ROOM_KOP]);
    lv_obj_set_style_text_font(KOP_label3, FONT_14, 0);
    lv_obj_add_flag(KOP_label3, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(KOP_label3, "00.0%");
    lv_obj_align(KOP_label3, LV_ALIGN_TOP_LEFT, 5, 45);

    // KOP bulb icon (bathroomLight=2, index 1)
    bulb_icons[1] = lv_img_create(cards[ROOM_KOP]);
    lv_obj_align(bulb_icons[1], LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_flag(bulb_icons[1], LV_OBJ_FLAG_HIDDEN); // Default hidden

    // DS
    cards[ROOM_DS] = lv_btn_create(lv_scr_act());
    lv_obj_set_user_data(cards[ROOM_DS], (void*)ROOM_DS);
    lv_obj_set_pos(cards[ROOM_DS], 2, 169);
    lv_obj_set_size(cards[ROOM_DS], 252, 67);
    lv_obj_add_style(cards[ROOM_DS], &style_card, 0);
    lv_obj_set_style_bg_color(cards[ROOM_DS], lv_color_hex(BTN_DS_COLOR), 0);

    lv_obj_add_event_cb(cards[ROOM_DS], button_event_cb, LV_EVENT_ALL, reinterpret_cast<void*>(ROOM_DS));

    DS_label1 = lv_label_create(cards[ROOM_DS]);
    lv_obj_set_style_text_font(DS_label1, FONT_24, 0);
    lv_obj_add_flag(DS_label1, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(DS_label1, "DS");
    lv_obj_align(DS_label1, LV_ALIGN_TOP_LEFT, 5, 10);

    DS_label2 = lv_label_create(cards[ROOM_DS]);
    lv_obj_set_style_text_font(DS_label2, FONT_14, 0);
    lv_obj_add_flag(DS_label2, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(DS_label2, "00.0°");
    lv_obj_align(DS_label2, LV_ALIGN_TOP_LEFT, 75, 10);

    DS_label3 = lv_label_create(cards[ROOM_DS]);
    lv_obj_set_style_text_font(DS_label3, FONT_14, 0);
    lv_obj_add_flag(DS_label3, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(DS_label3, "00.0%");
    lv_obj_align(DS_label3, LV_ALIGN_TOP_LEFT, 75, 30);

    DS_label4 = lv_label_create(cards[ROOM_DS]);
    lv_obj_set_style_text_font(DS_label4, FONT_14, 0);
    lv_obj_add_flag(DS_label4, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(DS_label4, "000 ppm");
    lv_obj_align(DS_label4, LV_ALIGN_TOP_LEFT, 140, 10);

    DS_label5 = lv_label_create(cards[ROOM_DS]);
    lv_obj_set_style_text_font(DS_label5, FONT_14, 0);
    lv_obj_add_flag(DS_label5, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(DS_label5, "000");
    lv_obj_align(DS_label5, LV_ALIGN_TOP_LEFT, 140, 30);

    // S
    cards[ROOM_S] = lv_btn_create(lv_scr_act());
    lv_obj_set_user_data(cards[ROOM_S], (void*)ROOM_S);
    lv_obj_set_pos(cards[ROOM_S], 258, 169);
    lv_obj_set_size(cards[ROOM_S], 28, 67);
    lv_obj_add_style(cards[ROOM_S], &style_card, 0);
    lv_obj_set_style_bg_color(cards[ROOM_S], lv_color_hex(BTN_CLOSED_COLOR), 0);

    lv_obj_add_event_cb(cards[ROOM_S], button_event_cb, LV_EVENT_ALL, reinterpret_cast<void*>(ROOM_S));

    S_label1 = lv_label_create(cards[ROOM_S]);
    lv_obj_set_style_text_font(S_label1, FONT_24, 0);
    lv_obj_add_flag(S_label1, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(S_label1, "S");
    lv_obj_align(S_label1, LV_ALIGN_CENTER, 0, 0);

    // B
    cards[ROOM_B] = lv_btn_create(lv_scr_act());
    lv_obj_set_user_data(cards[ROOM_B], (void*)ROOM_B);
    lv_obj_set_pos(cards[ROOM_B], 290, 169);
    lv_obj_set_size(cards[ROOM_B], 28, 67);
    lv_obj_add_style(cards[ROOM_B], &style_card, 0);
    lv_obj_set_style_bg_color(cards[ROOM_B], lv_color_hex(BTN_CLOSED_COLOR), 0);

    lv_obj_add_event_cb(cards[ROOM_B], button_event_cb, LV_EVENT_ALL, reinterpret_cast<void*>(ROOM_B));

    B_label1 = lv_label_create(cards[ROOM_B]);
    lv_obj_set_style_text_font(B_label1, FONT_24, 0);
    lv_obj_add_flag(B_label1, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(B_label1, "B");
    lv_obj_align(B_label1, LV_ALIGN_CENTER, 0, 0);

    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(NULL);

    lv_refr_now(NULL); // force layout update before prints

    // Debug prints after flex resolve
    lv_area_t coords;
    lv_obj_get_coords(cards[ROOM_EXT], &coords);
    Serial.printf("Button %s: abs x=%d y=%d w=%d h=%d\n", roomNames[ROOM_EXT], coords.x1, coords.y1, lv_obj_get_width(cards[ROOM_EXT]), lv_obj_get_height(cards[ROOM_EXT]));
    lv_obj_get_coords(cards[ROOM_TIME_WIFI], &coords);
    Serial.printf("Button %s: abs x=%d y=%d w=%d h=%d\n", roomNames[ROOM_TIME_WIFI], coords.x1, coords.y1, lv_obj_get_width(cards[ROOM_TIME_WIFI]), lv_obj_get_height(cards[ROOM_TIME_WIFI]));
    lv_obj_get_coords(cards[ROOM_WC], &coords);
    Serial.printf("Button %s: abs x=%d y=%d w=%d h=%d\n", roomNames[ROOM_WC], coords.x1, coords.y1, lv_obj_get_width(cards[ROOM_WC]), lv_obj_get_height(cards[ROOM_WC]));
    lv_obj_get_coords(cards[ROOM_UT], &coords);
    Serial.printf("Button %s: abs x=%d y=%d w=%d h=%d\n", roomNames[ROOM_UT], coords.x1, coords.y1, lv_obj_get_width(cards[ROOM_UT]), lv_obj_get_height(cards[ROOM_UT]));
    lv_obj_get_coords(cards[ROOM_KOP], &coords);
    Serial.printf("Button %s: abs x=%d y=%d w=%d h=%d\n", roomNames[ROOM_KOP], coords.x1, coords.y1, lv_obj_get_width(cards[ROOM_KOP]), lv_obj_get_height(cards[ROOM_KOP]));
    lv_obj_get_coords(cards[ROOM_DS], &coords);
    Serial.printf("Button %s: abs x=%d y=%d w=%d h=%d\n", roomNames[ROOM_DS], coords.x1, coords.y1, lv_obj_get_width(cards[ROOM_DS]), lv_obj_get_height(cards[ROOM_DS]));
    lv_obj_get_coords(cards[ROOM_S], &coords);
    Serial.printf("Button %s: abs x=%d y=%d w=%d h=%d\n", roomNames[ROOM_S], coords.x1, coords.y1, lv_obj_get_width(cards[ROOM_S]), lv_obj_get_height(cards[ROOM_S]));
    lv_obj_get_coords(cards[ROOM_B], &coords);
    Serial.printf("Button %s: abs x=%d y=%d w=%d h=%d\n", roomNames[ROOM_B], coords.x1, coords.y1, lv_obj_get_width(cards[ROOM_B]), lv_obj_get_height(cards[ROOM_B]));
}

static void button_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_current_target(e);
    int roomId = reinterpret_cast<intptr_t>(lv_event_get_user_data(e));

    if (code == LV_EVENT_PRESSED) {
        lv_anim_del(btn, (lv_anim_exec_xcb_t)lv_obj_set_style_transform_zoom);
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, btn);
        lv_anim_set_values(&a, 256, 240);
        lv_anim_set_time(&a, 150);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_transform_zoom);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
        lv_anim_start(&a);
    } else if (code == LV_EVENT_SHORT_CLICKED) {
        Serial.printf("[%lu] gumb [%s] kratek pritisk\n", millis(), roomNames[roomId]);
        // Perform short action (e.g., turn on fan)
        if (roomId >= ROOM_WC && roomId <= ROOM_DS) {
            String roomStr = (roomId == ROOM_WC) ? "wc" : (roomId == ROOM_UT) ? "ut" : (roomId == ROOM_KOP) ? "kop" : "ds";
            sendManualControl(roomStr, "manual");
        }
        lv_anim_del(btn, (lv_anim_exec_xcb_t)lv_obj_set_style_transform_zoom);
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, btn);
        lv_anim_set_values(&a, 240, 256);
        lv_anim_set_time(&a, 150);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_transform_zoom);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
        lv_anim_start(&a);
    } else if (code == LV_EVENT_LONG_PRESSED) {
        Serial.printf("[%lu] gumb [%s] dolg pritisk\n", millis(), roomNames[roomId]);
        // Perform long action (e.g., disable fan)
        if (roomId >= ROOM_WC && roomId <= ROOM_DS) {
            String roomStr = (roomId == ROOM_WC) ? "wc" : (roomId == ROOM_UT) ? "ut" : (roomId == ROOM_KOP) ? "kop" : "ds";
            sendManualControl(roomStr, "toggle");
        }
        lv_anim_del(btn, (lv_anim_exec_xcb_t)lv_obj_set_style_transform_zoom);
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, btn);
        lv_anim_set_values(&a, 240, 256);
        lv_anim_set_time(&a, 150);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_transform_zoom);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
        lv_anim_start(&a);
    }
}



void updateUI() {
    updateCards();
    updateWeatherIcon();
    updateWiFiIcon();
}

void updateCards() {
    bool ceOffline = sensorData.errorFlags[0] & ERR_HTTP;

    // Update EXT
    lv_label_set_text_fmt(EXT_label1, "%.1f°", sensorData.extTemp);
    lv_label_set_text_fmt(EXT_label2, "%.1f%%", sensorData.extHumidity);
    lv_label_set_text_fmt(EXT_label3, "%d hPa", (int)sensorData.extPressure);
    lv_label_set_text_fmt(EXT_label4, "%d lx", (int)sensorData.extLux);

    // Update TIME_WIFI
    if (timeSynced) {
        lv_label_set_text(TIME_WIFI_label1, myTZ.dateTime("H:i").c_str());
        lv_label_set_text(TIME_WIFI_label2, myTZ.dateTime("d.m.Y").c_str());
    } else {
        lv_label_set_text(TIME_WIFI_label1, "--");
        lv_label_set_text(TIME_WIFI_label2, "--");
    }
    lv_label_set_text_fmt(TIME_WIFI_label3, "P=%.1f W", sensorData.currentPower);
    if (ceOffline) {
        lv_label_set_text(TIME_WIFI_label4, "E=0000 Wh (OFF)");
    } else {
        lv_label_set_text_fmt(TIME_WIFI_label4, "E=%.1f Wh", sensorData.energyConsumption);
    }

    // Update WC
    if (ceOffline) {
        lv_label_set_text(WC_label1, "CE offline");
        lv_obj_set_style_bg_color(cards[ROOM_WC], lv_color_hex(0x666666), 0); // Gray
        lv_label_set_text(WC_label2, "");
    } else {
        lv_obj_set_style_bg_color(cards[ROOM_WC], lv_color_hex(BTN_WC_COLOR), 0);
        if (sensorData.fanStates[0]) {
            uint32_t remaining = sensorData.offTimes[0] - myTZ.now();
            if (remaining > 0) {
                lv_label_set_text_fmt(WC_label1, "%d", remaining);
            } else {
                lv_label_set_text(WC_label1, "WC");
            }
        } else {
            lv_label_set_text(WC_label1, "WC");
        }
        lv_label_set_text_fmt(WC_label2, "%d hPa", (int)sensorData.bathroomPressure);
    }

    // Update UT
    if (ceOffline) {
        lv_label_set_text(UT_label1, "CE offline");
        lv_obj_set_style_bg_color(cards[ROOM_UT], lv_color_hex(0x666666), 0); // Gray
        lv_label_set_text(UT_label2, "");
        lv_label_set_text(UT_label3, "");
    } else {
        lv_obj_set_style_bg_color(cards[ROOM_UT], lv_color_hex(BTN_UT_COLOR), 0);
        if (sensorData.fanStates[1]) {
            uint32_t remaining = sensorData.offTimes[1] - myTZ.now();
            if (remaining > 0) {
                lv_label_set_text_fmt(UT_label1, "%d", remaining);
            } else {
                lv_label_set_text(UT_label1, "UT");
            }
        } else {
            lv_label_set_text(UT_label1, "UT");
        }
        lv_label_set_text_fmt(UT_label2, "%.1f°", sensorData.utTemp);
        lv_label_set_text_fmt(UT_label3, "%.1f%%", sensorData.utHumidity);
    }

    // Update KOP
    if (ceOffline) {
        lv_label_set_text(KOP_label1, "CE offline");
        lv_obj_set_style_bg_color(cards[ROOM_KOP], lv_color_hex(0x666666), 0); // Gray
        lv_label_set_text(KOP_label2, "");
        lv_label_set_text(KOP_label3, "");
    } else {
        lv_obj_set_style_bg_color(cards[ROOM_KOP], lv_color_hex(BTN_KOP_COLOR), 0);
        if (sensorData.fanStates[2]) {
            uint32_t remaining = sensorData.offTimes[2] - myTZ.now();
            if (remaining > 0) {
                lv_label_set_text_fmt(KOP_label1, "%d", remaining);
            } else {
                lv_label_set_text(KOP_label1, "KOP");
            }
        } else {
            lv_label_set_text(KOP_label1, "KOP");
        }
        lv_label_set_text_fmt(KOP_label2, "%.1f°", sensorData.bathroomTemp);
        lv_label_set_text_fmt(KOP_label3, "%.1f%%", sensorData.bathroomHumidity);
    }

    // Update DS
    if (ceOffline) {
        lv_label_set_text(DS_label1, "CE offline");
        lv_obj_set_style_bg_color(cards[ROOM_DS], lv_color_hex(0x666666), 0); // Gray
        lv_label_set_text(DS_label2, "");
        lv_label_set_text(DS_label3, "");
        lv_label_set_text(DS_label4, "");
        lv_label_set_text(DS_label5, "");
    } else {
        lv_obj_set_style_bg_color(cards[ROOM_DS], lv_color_hex(BTN_DS_COLOR), 0);
        lv_label_set_text(DS_label1, "DS");
        lv_label_set_text_fmt(DS_label2, "%.1f°", sensorData.localTemp);
        lv_label_set_text_fmt(DS_label3, "%.1f%%", sensorData.localHumidity);
        lv_label_set_text_fmt(DS_label4, "%d ppm", (int)sensorData.localCO2);
        lv_label_set_text(DS_label5, "---");
    }

    // Update light bulb icons (only if SD is available)
    if (!(sensorData.errorFlags[0] & ERR_SD)) {
        // UT bulb icon (utilityLight=1, inputs[1])
        if (sensorData.inputs[1] == 1 && bulb_icons[0]) {
            lv_img_set_src(bulb_icons[0], "S:/ico/32/bulb-32.png");
            lv_obj_clear_flag(bulb_icons[0], LV_OBJ_FLAG_HIDDEN);
        } else if (bulb_icons[0]) {
            lv_obj_add_flag(bulb_icons[0], LV_OBJ_FLAG_HIDDEN);
        }

        // KOP bulb icon (bathroomLight=2, inputs[2])
        if (sensorData.inputs[2] == 1 && bulb_icons[1]) {
            lv_img_set_src(bulb_icons[1], "S:/ico/32/bulb-32.png");
            lv_obj_clear_flag(bulb_icons[1], LV_OBJ_FLAG_HIDDEN);
        } else if (bulb_icons[1]) {
            lv_obj_add_flag(bulb_icons[1], LV_OBJ_FLAG_HIDDEN);
        }

        // WC bulb icon (wcLight=3, inputs[3])
        if (sensorData.inputs[3] == 1 && bulb_icons[2]) {
            lv_img_set_src(bulb_icons[2], "S:/ico/32/bulb-32.png");
            lv_obj_clear_flag(bulb_icons[2], LV_OBJ_FLAG_HIDDEN);
        } else if (bulb_icons[2]) {
            lv_obj_add_flag(bulb_icons[2], LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Update window buttons colors - placeholder
}

void cycleGraph() {
    // Cycle currentGraphType (enum GraphType)
    // Update lv_chart series
}

String getWeatherIconFilename(int weatherCode, float lux) {
    bool isDay = (lux > 20.0f);
    String iconPath = "S:/ico/48/";

    switch (weatherCode) {
        case 0:  // Jasno nebo
            return iconPath + (isDay ? "sun-48.png" : "night-moon-48.png");
        case 1:  // Večinoma jasno
            return iconPath + (isDay ? "haze-48.png" : "night-moon-48.png");
        case 2:  // Delno oblačno
            return iconPath + (isDay ? "partly-cloudy-day-48.png" : "cloudy-night-48.png");
        case 3:  // Popolnoma oblačno
            return iconPath + "cloudy-night-48.png";
        case 45: // Megla
            return iconPath + "fog-48.png";
        case 48: // Megla z nabiranjem ivja
            return iconPath + (isDay ? "haze-48.png" : "fog-48.png");
        case 51: // Rosenje: Šibka intenzivnost
        case 53: // Rosenje: Zmerna intenzivnost
            return iconPath + "Drizzle-cloud-48.png";
        case 55: // Rosenje: Gosta intenzivnost
            return iconPath + "drizzle-wet-48.png";
        case 56: // Zmrzovalno rosenje: Šibka intenzivnost
        case 66: // Zmrzovalni dež: Šibka intenzivnost
            return iconPath + "light-snow-48.png";
        case 57: // Zmrzovalno rosenje: Gosta intenzivnost
        case 67: // Zmrzovalni dež: Močna intenzivnost
            return iconPath + "snow-48.png";
        case 61: // Dež: Šibka intenzivnost
        case 80: // Plohe dežja: Šibke
            return iconPath + (isDay ? "partly-cloudy-day-48.png" : "Drizzle-cloud-48.png");
        case 63: // Dež: Zmerna intenzivnost
        case 81: // Plohe dežja: Zmerne
            return iconPath + (isDay ? "rain-48.png" : "rain-48.png");
        case 65: // Dež: Močna intenzivnost
        case 82: // Plohe dežja: Silovite
            return iconPath + (isDay ? "heavy-rain-48.png" : "heavy-rain-48.png");
        case 71: // Sneženje: Šibka intenzivnost
        case 73: // Sneženje: Zmerna intenzivnost
        case 77: // Snežne zrnca
        case 85: // Plohe snega: Šibke
        case 86: // Plohe snega: Močne
            return iconPath + (isDay ? "light-snow-48.png" : "light-snow-48.png");
        case 75: // Sneženje: Močna intenzivnost
            return iconPath + (isDay ? "snow-storm-48.png" : "snow-storm-48.png");
        case 95: // Nevihta: Šibka ali zmerna
        case 96: // Nevihta s šibko točo
            return iconPath + (isDay ? "light-storm-48.png" : "light-storm-48.png");
        case 99: // Nevihta z močno točo
            return iconPath + (isDay ? "heavy-storm-48.png" : "heavy-storm-48.png");
        default: // Fallback za neznane kode
            return iconPath + (isDay ? "partly-cloudy-day-48.png" : "cloudy-night-48.png");
    }
}

void updateWeatherIcon() {
    // Update weather icon in EXT based on sensorData.weatherCode and extLux
    if (weather_icon && sensorData.weatherCode >= 0) {
        String iconPath = getWeatherIconFilename(sensorData.weatherCode, sensorData.extLux);
        lv_img_set_src(weather_icon, iconPath.c_str());
        Serial.printf("[Weather] Code: %d, Lux: %.1f, Icon: %s\n",
                     sensorData.weatherCode, sensorData.extLux, iconPath.c_str());
    }
}

void updateWiFiIcon() {
    // Update WiFi icon in TIME_WIFI based on WiFi status
    if (wifi_icon) {
        String iconPath = "S:/ico/32/";
        if (WiFi.status() == WL_CONNECTED && !(sensorData.errorFlags[0] & ERR_WIFI)) {
            iconPath += "wifi_on.png";
        } else {
            iconPath += "wifi_off.png";
        }
        lv_img_set_src(wifi_icon, iconPath.c_str());
        Serial.printf("[WiFi] Status: %d, ErrorFlags: %d, Icon: %s\n",
                     WiFi.status(), sensorData.errorFlags[0], iconPath.c_str());
    }
}

static void ext_event_cb(lv_event_t * e) {
    for (int i = 0; i < 8; i++) {
        lv_obj_add_flag(cards[i], LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_clear_flag(graph_container, LV_OBJ_FLAG_HIDDEN);
    lv_timer_create(return_to_rooms, 30000, NULL);
}

static void graph_event_cb(lv_event_t * e) {
    cycleGraph();
}

static void return_to_rooms(lv_timer_t* timer) {
    for (int i = 0; i < 8; i++) {
        lv_obj_clear_flag(cards[i], LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_add_flag(graph_container, LV_OBJ_FLAG_HIDDEN);
}

static void anim_zoom_cb(void * obj, int32_t v) {
    lv_obj_set_style_transform_zoom((lv_obj_t *)obj, v, 0);
}
