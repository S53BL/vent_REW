// disp.cpp - Display and UI module implementation
#include "disp.h"
#include "globals.h"
#include <Display_ST7789.h>
#include <Touch_CST328.h>
#include <LVGL_Driver.h>

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
lv_obj_t* room_container;
lv_obj_t* graph_container;
lv_obj_t* chart;
lv_obj_t* cards[6];
lv_obj_t* weather_icon;
lv_obj_t* wifi_icon;

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
    room_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(room_container, 320, 240);
    lv_obj_set_pos(room_container, 0, 0);
    lv_obj_set_style_bg_opa(room_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_clip_corner(room_container, false, 0);
    lv_obj_clear_flag(room_container, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);

    graph_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(graph_container, LV_PCT(100), LV_PCT(100));
    lv_obj_add_flag(graph_container, LV_OBJ_FLAG_HIDDEN);

    // Create room cards
    createRoomCards();
    lv_obj_invalidate(room_container); // Force redraw
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
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);

    // Row containers
    lv_obj_t* top_row = lv_obj_create(lv_scr_act());
    lv_obj_set_size(top_row, 318, 80);
    lv_obj_set_pos(top_row, 1, 1);
    lv_obj_set_style_bg_opa(top_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(top_row, 0, 0);
    lv_obj_set_style_pad_all(top_row, 0, 0);
    lv_obj_set_style_pad_left(top_row, 1, 0);
    lv_obj_set_style_pad_right(top_row, 1, 0);
    lv_obj_set_flex_flow(top_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(top_row, 4, 0);

    lv_obj_t* middle_row = lv_obj_create(lv_scr_act());
    lv_obj_set_size(middle_row, 318, 80);
    lv_obj_set_pos(middle_row, 1, 85);
    lv_obj_set_style_bg_opa(middle_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(middle_row, 0, 0);
    lv_obj_set_style_pad_all(middle_row, 0, 0);
    lv_obj_set_style_pad_left(middle_row, 1, 0);
    lv_obj_set_style_pad_right(middle_row, 1, 0);
    lv_obj_set_flex_flow(middle_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(middle_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(middle_row, 4, 0);

    lv_obj_t* bottom_row = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bottom_row, 318, 67);
    lv_obj_set_pos(bottom_row, 1, 169);
    lv_obj_set_style_bg_opa(bottom_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bottom_row, 0, 0);
    lv_obj_set_style_pad_all(bottom_row, 0, 0);
    lv_obj_set_style_pad_left(bottom_row, 1, 0);
    lv_obj_set_style_pad_right(bottom_row, 1, 0);
    lv_obj_set_flex_flow(bottom_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(bottom_row, 4, 0);

    // EXT
    cards[ROOM_EXT] = lv_btn_create(top_row);
    lv_obj_set_size(cards[ROOM_EXT], 173, 80);
    lv_obj_add_style(cards[ROOM_EXT], &style_card, 0);
    lv_obj_set_style_bg_color(cards[ROOM_EXT], lv_color_hex(BTN_EXT_COLOR), 0);
    lv_obj_add_event_cb(cards[ROOM_EXT], button_event_cb, LV_EVENT_ALL, (void*)ROOM_EXT);
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

    // TIME_WIFI
    cards[ROOM_TIME_WIFI] = lv_btn_create(top_row);
    lv_obj_set_size(cards[ROOM_TIME_WIFI], 137, 80);
    lv_obj_add_style(cards[ROOM_TIME_WIFI], &style_card, 0);
    lv_obj_set_style_bg_color(cards[ROOM_TIME_WIFI], lv_color_hex(BTN_TIME_WIFI_COLOR), 0);
    lv_obj_add_event_cb(cards[ROOM_TIME_WIFI], button_event_cb, LV_EVENT_ALL, (void*)ROOM_TIME_WIFI);

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
    cards[ROOM_WC] = lv_btn_create(middle_row);
    lv_obj_set_size(cards[ROOM_WC], 101, 80);
    lv_obj_add_style(cards[ROOM_WC], &style_card, 0);
    lv_obj_set_style_bg_color(cards[ROOM_WC], lv_color_hex(BTN_WC_COLOR), 0);
    lv_obj_add_event_cb(cards[ROOM_WC], button_event_cb, LV_EVENT_ALL, (void*)ROOM_WC);

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

    // UT
    cards[ROOM_UT] = lv_btn_create(middle_row);
    lv_obj_set_size(cards[ROOM_UT], 101, 80);
    lv_obj_add_style(cards[ROOM_UT], &style_card, 0);
    lv_obj_set_style_bg_color(cards[ROOM_UT], lv_color_hex(BTN_UT_COLOR), 0);
    lv_obj_add_event_cb(cards[ROOM_UT], button_event_cb, LV_EVENT_ALL, (void*)ROOM_UT);

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

    // KOP
    cards[ROOM_KOP] = lv_btn_create(middle_row);
    lv_obj_set_size(cards[ROOM_KOP], 101, 80);
    lv_obj_add_style(cards[ROOM_KOP], &style_card, 0);
    lv_obj_set_style_bg_color(cards[ROOM_KOP], lv_color_hex(BTN_KOP_COLOR), 0);
    lv_obj_add_event_cb(cards[ROOM_KOP], button_event_cb, LV_EVENT_ALL, (void*)ROOM_KOP);

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

    // DS
    cards[ROOM_DS] = lv_btn_create(bottom_row);
    lv_obj_set_size(cards[ROOM_DS], 252, 67);
    lv_obj_add_style(cards[ROOM_DS], &style_card, 0);
    lv_obj_set_style_bg_color(cards[ROOM_DS], lv_color_hex(BTN_DS_COLOR), 0);
    lv_obj_add_event_cb(cards[ROOM_DS], button_event_cb, LV_EVENT_ALL, (void*)ROOM_DS);

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

    // Small buttons
    lv_obj_t* skylight_btn = lv_btn_create(bottom_row);
    lv_obj_set_size(skylight_btn, 28, 67);
    lv_obj_add_style(skylight_btn, &style_card, 0);
    lv_obj_set_style_bg_color(skylight_btn, lv_color_hex(BTN_CLOSED_COLOR), 0);
    lv_obj_add_event_cb(skylight_btn, button_event_cb, LV_EVENT_ALL, (void*)6); // Placeholder

    S_label1 = lv_label_create(skylight_btn);
    lv_obj_set_style_text_font(S_label1, FONT_24, 0);
    lv_obj_add_flag(S_label1, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_label_set_text(S_label1, "S");
    lv_obj_align(S_label1, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* balcony_btn = lv_btn_create(bottom_row);
    lv_obj_set_size(balcony_btn, 28, 67);
    lv_obj_add_style(balcony_btn, &style_card, 0);
    lv_obj_set_style_bg_color(balcony_btn, lv_color_hex(BTN_CLOSED_COLOR), 0);
    lv_obj_add_event_cb(balcony_btn, button_event_cb, LV_EVENT_ALL, (void*)7); // Placeholder

    B_label1 = lv_label_create(balcony_btn);
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
    lv_obj_get_coords(skylight_btn, &coords);
    Serial.printf("Button %s: abs x=%d y=%d w=%d h=%d\n", roomNames[6], coords.x1, coords.y1, lv_obj_get_width(skylight_btn), lv_obj_get_height(skylight_btn));
    lv_obj_get_coords(balcony_btn, &coords);
    Serial.printf("Button %s: abs x=%d y=%d w=%d h=%d\n", roomNames[7], coords.x1, coords.y1, lv_obj_get_width(balcony_btn), lv_obj_get_height(balcony_btn));
}

static void button_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_current_target(e);
    int roomId = (int)lv_event_get_user_data(e);

    if (code == LV_EVENT_PRESSED) {
        lv_area_t coords;
        lv_obj_get_coords(btn, &coords);
        Serial.printf("[UI] Pressed %s at abs x1=%d y1=%d\n", roomNames[roomId], coords.x1, coords.y1);
        touch_press_time = millis();
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, btn);
        lv_anim_set_values(&a, 256, 250); // 1000/256 = 3.9, approx 1.0 to 0.98
        lv_anim_set_time(&a, 150);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_transform_zoom);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
        lv_anim_start(&a);
    } else if (code == LV_EVENT_RELEASED) {
        unsigned long duration = millis() - touch_press_time;
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, btn);
        lv_anim_set_values(&a, 250, 256);
        lv_anim_set_time(&a, 150);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_transform_zoom);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
        lv_anim_start(&a);
        if (duration < 500) {
            // Short press: on
            Serial.printf("[UI] Short press %s: ON\n", roomNames[roomId]);
            // TODO: Send MANUAL_CONTROL POST to CEW with roomId=0
        } else {
            // Long press: disable
            Serial.printf("[UI] Long press %s: DISABLE\n", roomNames[roomId]);
            // TODO: Send MANUAL_CONTROL POST to CEW with roomId=1
        }
    }
}



void updateUI() {
    updateCards();
    updateWeatherIcon();
}

void updateCards() {
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
    lv_label_set_text_fmt(TIME_WIFI_label4, "E=%.1f Wh", sensorData.energyConsumption);

    // Update WC
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

    // Update UT
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

    // Update KOP
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

    // Update DS
    lv_label_set_text_fmt(DS_label2, "%.1f°", sensorData.localTemp);
    lv_label_set_text_fmt(DS_label3, "%.1f%%", sensorData.localHumidity);
    lv_label_set_text_fmt(DS_label4, "%d ppm", (int)sensorData.localCO2);
    lv_label_set_text(DS_label5, "---");

    // Update window buttons colors - placeholder
}

void cycleGraph() {
    // Cycle currentGraphType (enum GraphType)
    // Update lv_chart series
}

void updateWeatherIcon() {
    // Update weather icon in EXT based on sensorData.weatherIcon
    if (weather_icon) {
        lv_img_set_src(weather_icon, ("S:/icons/" + sensorData.weatherIcon).c_str());
    }
}

static void ext_event_cb(lv_event_t * e) {
    lv_obj_add_flag(room_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(graph_container, LV_OBJ_FLAG_HIDDEN);
    lv_timer_create(return_to_rooms, 30000, NULL);
}

static void graph_event_cb(lv_event_t * e) {
    cycleGraph();
}

static void return_to_rooms(lv_timer_t* timer) {
    lv_obj_clear_flag(room_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(graph_container, LV_OBJ_FLAG_HIDDEN);
}
