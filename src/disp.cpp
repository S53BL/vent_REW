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
lv_obj_t* wc_label;
lv_obj_t* wc_value_label;
lv_obj_t* ut_label;
lv_obj_t* ut_temp_label;
lv_obj_t* ut_hum_label;
lv_obj_t* kop_label;
lv_obj_t* kop_temp_label;
lv_obj_t* kop_hum_label;
lv_obj_t* ds_label1;
lv_obj_t* ds_label2;
lv_obj_t* ds_label3;
lv_obj_t* time_wifi_label;

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
    lv_obj_t* ext_label1 = lv_label_create(cards[ROOM_EXT]);
    lv_obj_set_style_text_font(ext_label1, FONT_MAIN, 0);
    lv_label_set_text(ext_label1, "23.6°");
    lv_obj_align(ext_label1, LV_ALIGN_LEFT_MID, 10, 5);

    lv_obj_t* ext_label2 = lv_label_create(cards[ROOM_EXT]);
    lv_obj_set_style_text_font(ext_label2, FONT_MAIN, 0);
    lv_label_set_text(ext_label2, "77.3%");
    lv_obj_align(ext_label2, LV_ALIGN_LEFT_MID, 10, 19);

    lv_obj_t* ext_label3 = lv_label_create(cards[ROOM_EXT]);
    lv_obj_set_style_text_font(ext_label3, FONT_MAIN, 0);
    lv_label_set_text(ext_label3, "1013 hPa");
    lv_obj_align(ext_label3, LV_ALIGN_LEFT_MID, 10, 33);

    lv_obj_t* ext_label4 = lv_label_create(cards[ROOM_EXT]);
    lv_obj_set_style_text_font(ext_label4, FONT_MAIN, 0);
    lv_label_set_text(ext_label4, "300 lx");
    lv_obj_align(ext_label4, LV_ALIGN_LEFT_MID, 10, 47);

    // TIME_WIFI
    cards[ROOM_TIME_WIFI] = lv_btn_create(top_row);
    lv_obj_set_size(cards[ROOM_TIME_WIFI], 137, 80);
    lv_obj_add_style(cards[ROOM_TIME_WIFI], &style_card, 0);
    lv_obj_set_style_bg_color(cards[ROOM_TIME_WIFI], lv_color_hex(BTN_TIME_WIFI_COLOR), 0);
    lv_obj_add_event_cb(cards[ROOM_TIME_WIFI], button_event_cb, LV_EVENT_ALL, (void*)ROOM_TIME_WIFI);

    // TIME_WIFI labels
    lv_obj_t* time_label1 = lv_label_create(cards[ROOM_TIME_WIFI]);
    lv_obj_set_style_text_font(time_label1, FONT_MAIN, 0);
    lv_label_set_text(time_label1, "11:32");
    lv_obj_align(time_label1, LV_ALIGN_LEFT_MID, 10, 5);

    lv_obj_t* time_label2 = lv_label_create(cards[ROOM_TIME_WIFI]);
    lv_obj_set_style_text_font(time_label2, FONT_MAIN, 0);
    lv_label_set_text(time_label2, "22.1.26");
    lv_obj_align(time_label2, LV_ALIGN_LEFT_MID, 10, 19);

    lv_obj_t* time_label3 = lv_label_create(cards[ROOM_TIME_WIFI]);
    lv_obj_set_style_text_font(time_label3, FONT_MAIN, 0);
    lv_label_set_text(time_label3, "P=78 W");
    lv_obj_align(time_label3, LV_ALIGN_LEFT_MID, 10, 33);

    lv_obj_t* time_label4 = lv_label_create(cards[ROOM_TIME_WIFI]);
    lv_obj_set_style_text_font(time_label4, FONT_MAIN, 0);
    lv_label_set_text(time_label4, "E=12450 Wh (OFF)");
    lv_obj_align(time_label4, LV_ALIGN_LEFT_MID, 10, 47);

    // WiFi icon placeholder
    wifi_icon = lv_img_create(cards[ROOM_TIME_WIFI]);
    lv_obj_align(wifi_icon, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

    // WC
    cards[ROOM_WC] = lv_btn_create(middle_row);
    lv_obj_set_size(cards[ROOM_WC], 101, 80);
    lv_obj_add_style(cards[ROOM_WC], &style_card, 0);
    lv_obj_set_style_bg_color(cards[ROOM_WC], lv_color_hex(BTN_WC_COLOR), 0);
    lv_obj_add_event_cb(cards[ROOM_WC], button_event_cb, LV_EVENT_ALL, (void*)ROOM_WC);

    wc_label = lv_label_create(cards[ROOM_WC]);
    lv_obj_set_style_text_font(wc_label, FONT_LABEL, 0);
    lv_label_set_text(wc_label, "WC");
    lv_obj_align(wc_label, LV_ALIGN_LEFT_MID, 10, 3);

    wc_value_label = lv_label_create(cards[ROOM_WC]);
    lv_obj_set_style_text_font(wc_value_label, FONT_MAIN, 0);
    lv_label_set_text(wc_value_label, "1013 hPa");
    lv_obj_align(wc_value_label, LV_ALIGN_LEFT_MID, 10, 25);

    // UT
    cards[ROOM_UT] = lv_btn_create(middle_row);
    lv_obj_set_size(cards[ROOM_UT], 101, 80);
    lv_obj_add_style(cards[ROOM_UT], &style_card, 0);
    lv_obj_set_style_bg_color(cards[ROOM_UT], lv_color_hex(BTN_UT_COLOR), 0);
    lv_obj_add_event_cb(cards[ROOM_UT], button_event_cb, LV_EVENT_ALL, (void*)ROOM_UT);

    ut_label = lv_label_create(cards[ROOM_UT]);
    lv_obj_set_style_text_font(ut_label, FONT_LABEL, 0);
    lv_label_set_text(ut_label, "UT");
    lv_obj_align(ut_label, LV_ALIGN_LEFT_MID, 10, 3);

    ut_temp_label = lv_label_create(cards[ROOM_UT]);
    lv_obj_set_style_text_font(ut_temp_label, FONT_MAIN, 0);
    lv_label_set_text(ut_temp_label, "22.1°");
    lv_obj_align(ut_temp_label, LV_ALIGN_LEFT_MID, 10, 25);

    ut_hum_label = lv_label_create(cards[ROOM_UT]);
    lv_obj_set_style_text_font(ut_hum_label, FONT_MAIN, 0);
    lv_label_set_text(ut_hum_label, "45.9%");
    lv_obj_align(ut_hum_label, LV_ALIGN_LEFT_MID, 10, 39);

    // KOP
    cards[ROOM_KOP] = lv_btn_create(middle_row);
    lv_obj_set_size(cards[ROOM_KOP], 101, 80);
    lv_obj_add_style(cards[ROOM_KOP], &style_card, 0);
    lv_obj_set_style_bg_color(cards[ROOM_KOP], lv_color_hex(BTN_KOP_COLOR), 0);
    lv_obj_add_event_cb(cards[ROOM_KOP], button_event_cb, LV_EVENT_ALL, (void*)ROOM_KOP);

    kop_label = lv_label_create(cards[ROOM_KOP]);
    lv_obj_set_style_text_font(kop_label, FONT_LABEL, 0);
    lv_label_set_text(kop_label, "KOP");
    lv_obj_align(kop_label, LV_ALIGN_LEFT_MID, 10, 3);

    kop_temp_label = lv_label_create(cards[ROOM_KOP]);
    lv_obj_set_style_text_font(kop_temp_label, FONT_MAIN, 0);
    lv_label_set_text(kop_temp_label, "24.2°");
    lv_obj_align(kop_temp_label, LV_ALIGN_LEFT_MID, 10, 25);

    kop_hum_label = lv_label_create(cards[ROOM_KOP]);
    lv_obj_set_style_text_font(kop_hum_label, FONT_MAIN, 0);
    lv_label_set_text(kop_hum_label, "65.1%");
    lv_obj_align(kop_hum_label, LV_ALIGN_LEFT_MID, 10, 39);

    // DS
    cards[ROOM_DS] = lv_btn_create(bottom_row);
    lv_obj_set_size(cards[ROOM_DS], 252, 67);
    lv_obj_add_style(cards[ROOM_DS], &style_card, 0);
    lv_obj_set_style_bg_color(cards[ROOM_DS], lv_color_hex(BTN_DS_COLOR), 0);
    lv_obj_add_event_cb(cards[ROOM_DS], button_event_cb, LV_EVENT_ALL, (void*)ROOM_DS);

    ds_label1 = lv_label_create(cards[ROOM_DS]);
    lv_obj_set_style_text_font(ds_label1, FONT_LABEL, 0);
    lv_label_set_text(ds_label1, "DS");
    lv_obj_align(ds_label1, LV_ALIGN_LEFT_MID, 10, 3);

    ds_label2 = lv_label_create(cards[ROOM_DS]);
    lv_obj_set_style_text_font(ds_label2, FONT_MAIN, 0);
    lv_label_set_text(ds_label2, "24.8° 52.1%");
    lv_obj_align(ds_label2, LV_ALIGN_LEFT_MID, 10, 25);

    ds_label3 = lv_label_create(cards[ROOM_DS]);
    lv_obj_set_style_text_font(ds_label3, FONT_MAIN, 0);
    lv_label_set_text(ds_label3, "845 ppm");
    lv_obj_align(ds_label3, LV_ALIGN_LEFT_MID, 10, 39);

    // Small buttons
    lv_obj_t* skylight_btn = lv_btn_create(bottom_row);
    lv_obj_set_size(skylight_btn, 28, 67);
    lv_obj_add_style(skylight_btn, &style_card, 0);
    lv_obj_set_style_bg_color(skylight_btn, lv_color_hex(BTN_CLOSED_COLOR), 0);
    lv_obj_add_event_cb(skylight_btn, button_event_cb, LV_EVENT_ALL, (void*)6); // Placeholder

    lv_obj_t* s_label = lv_label_create(skylight_btn);
    lv_obj_set_style_text_font(s_label, FONT_SMALL, 0);
    lv_label_set_text(s_label, "S");
    lv_obj_align(s_label, LV_ALIGN_CENTER, 0, 33);

    lv_obj_t* balcony_btn = lv_btn_create(bottom_row);
    lv_obj_set_size(balcony_btn, 28, 67);
    lv_obj_add_style(balcony_btn, &style_card, 0);
    lv_obj_set_style_bg_color(balcony_btn, lv_color_hex(BTN_CLOSED_COLOR), 0);
    lv_obj_add_event_cb(balcony_btn, button_event_cb, LV_EVENT_ALL, (void*)7); // Placeholder

    lv_obj_t* b_label = lv_label_create(balcony_btn);
    lv_obj_set_style_text_font(b_label, FONT_SMALL, 0);
    lv_label_set_text(b_label, "B");
    lv_obj_align(b_label, LV_ALIGN_CENTER, 0, 33);

    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(NULL);
}

static void button_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    int roomId = (int)lv_event_get_user_data(e);

    if (code == LV_EVENT_PRESSED) {
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
            Serial.printf("[UI] Short press room %d: ON\n", roomId);
            // TODO: Send MANUAL_CONTROL POST to CEW with roomId=0
        } else {
            // Long press: disable
            Serial.printf("[UI] Long press room %d: DISABLE\n", roomId);
            // TODO: Send MANUAL_CONTROL POST to CEW with roomId=1
        }
    }
}



void updateUI() {
    updateCards();
    updateWeatherIcon();
}

void updateCards() {
    // Update EXT - placeholder, in real update from sensorData
    // Labels already set to placeholder

    // Update TIME_WIFI - placeholder, in real update from NTP and sensorData
    // Labels already set to placeholder
    // TODO: Add (OFF) if CE offline

    // Update WC
    if (sensorData.bathroomFan) {
        uint32_t remaining = sensorData.offTimes[0] - millis();
        if (remaining > 0) {
            lv_label_set_text_fmt(wc_label, "%d", remaining / 1000);
            lv_obj_align(wc_label, LV_ALIGN_CENTER, 0, 0);
        } else {
            lv_label_set_text(wc_label, "OFF");
            lv_obj_align(wc_label, LV_ALIGN_LEFT_MID, 10, 0);
        }
    } else {
        lv_label_set_text(wc_label, "1013 hPa");
        lv_obj_align(wc_label, LV_ALIGN_LEFT_MID, 10, 0);
    }

    // Update UT
    lv_label_set_text_fmt(ut_label, "22.1°\n45.9%%");
    lv_obj_align(ut_label, LV_ALIGN_LEFT_MID, 10, 0);

    // Update KOP
    lv_label_set_text_fmt(kop_label, "24.2°\n65.1%%");
    lv_obj_align(kop_label, LV_ALIGN_LEFT_MID, 10, 0);

    // Update DS
    if (sensorData.dsFan) {
        uint32_t remaining = sensorData.offTimes[2] - millis();
        if (remaining > 0) {
            lv_label_set_text_fmt(ds_label3, "%d", remaining / 1000);
            lv_obj_align(ds_label3, LV_ALIGN_CENTER, 0, 0);
            lv_label_set_text(ds_label2, "");
        } else {
            lv_label_set_text(ds_label3, "OFF");
            lv_obj_align(ds_label3, LV_ALIGN_LEFT_MID, 10, 0);
            lv_label_set_text(ds_label2, "24.8° 52.1%");
        }
    } else {
        lv_label_set_text(ds_label2, "24.8° 52.1%");
        lv_label_set_text(ds_label3, "845 ppm");
        lv_obj_align(ds_label3, LV_ALIGN_LEFT_MID, 10, 0);
    }

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
