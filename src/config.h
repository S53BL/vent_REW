// config.h - Central configuration for vent_REW
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <lvgl.h>

// Pin definitions (from Waveshare ESP32-S3-Touch-LCD-2.8 specs)
#define EXAMPLE_PIN_NUM_MOSI            45
#define EXAMPLE_PIN_NUM_SCLK            40
#define EXAMPLE_PIN_NUM_CS              42
#define EXAMPLE_PIN_NUM_DC              41
#define EXAMPLE_PIN_NUM_RST             39
#define EXAMPLE_PIN_NUM_BK_LIGHT        5

#define SDA_PIN                         11
#define SCL_PIN                         10

#define TP_SDA                          1
#define TP_SCL                          3

#define SENSOR_I2C_BUS                  0
#define TOUCH_I2C_BUS                   1

#define SD_D3_PIN                       21    // CS
#define SD_CMD_PIN                      17    // MOSI
#define SD_CLK_PIN                      14    // SCLK
#define SD_D0_PIN                       16    // MISO

// Display dimensions (native portrait for ST7789)
#define LCD_WIDTH                       240
#define LCD_HEIGHT                      320

// Display rotation
#define LCD_ROTATION                    3  // Counter-clockwise for landscape
#define BUFFER_LINES                    10 // LVGL buffer lines
#define LV_USE_DMA                      false // Disable DMA for stability

// Touch calibration
#define TOUCH_RAW_X_MAX                 320
#define TOUCH_RAW_Y_MAX                 240

// LEDC for backlight
#define LEDC_CHANNEL                    0
#define LEDC_FREQ                       5000
#define LEDC_RES                        10

// Ostali parametri iz demoja (brez sprememb)
#define LCD_PIXEL_CLOCK_HZ              (20 * 1000 * 1000)
#define LCD_CMD_BITS                    8
#define LCD_PARAM_BITS                  8
#define LCD_BK_LIGHT_ON_LEVEL           1
#define LCD_BK_LIGHT_OFF_LEVEL          0

// LVGL buffer
#define LVGL_DRAW_BUF_HEIGHT            60
#define LVGL_DRAW_BUF_DOUBLE            1

// LVGL memory (handled by LVGL config)

// I2C
#define I2C_PORT                        I2C_NUM_0

// Audio (če je uporabljen v demoju)
#define EXAMPLE_I2S_BCK_PIN             6
#define EXAMPLE_I2S_WS_PIN              7
#define EXAMPLE_I2S_DATA_PIN            8

// WiFi settings
extern const char* ssidList[];
extern const char* passwordList[];
extern const int numNetworks;
extern IPAddress localIP;
extern IPAddress gateway;
extern IPAddress subnet;
extern IPAddress dns;
extern const char* apSSID;
extern const char* apPassword;

// Sensor addresses
#define SHT41_ADDRESS 0x44
#define SCD40_ADDRESS 0x62

// Constants
#define SENSOR_READ_INTERVAL 180000  // 3 minutes
#define HTTP_HEARTBEAT 600000       // 10 minutes
#define HTTP_SEND_INTERVAL 600000   // 10 minutes
#define WEATHER_UPDATE_INTERVAL 900000  // 15 minutes
#define ARCHIVE_TIME 300            // 00:05
#define HUM_THRESHOLD 60
#define CO2_HIGH 1000
#define EEPROM_MARKER 0xAB
#define I2C_TIMEOUT_MS 100
#define SENSOR_RETRY_COUNT 3
#define TEMP_CHANGE_THRESHOLD 0.3f
#define HUM_CHANGE_THRESHOLD 1.0f
#define CO2_CHANGE_THRESHOLD 50.0f

// WiFi and NTP constants
#define WIFI_CHECK_INTERVAL 600000   // 10 minutes
#define WIFI_RETRY_COUNT 3
#define WIFI_FIXED_DELAY 2000        // 2 seconds
#define NTP_UPDATE_INTERVAL 1800000  // 30 minutes

// NTP servers
extern const char* ntpServers[];
#define NTP_SERVER_COUNT 3

// Timezone string for Central European Time with DST
#define TZ_STRING "CET-1CEST,M3.5.0,M10.5.0/3"

// WiFi icons (16x16 px, placeholders - replace with actual PNG converted data)
// Defined in disp.cpp

// UI colors (from OpisEkrana.md)
#define BG_COLOR        0x303030UL  // Siva za ozadje
#define BTN_BLUE        0x0066CCUL  // Modra za začetno barvo gumba
#define BTN_GREEN       0x00CC66UL  // Zelena za barvo ob pritisku

#define BTN_RADIUS      10

#define BTN_EXT_COLOR       0x1C398E
#define BTN_TIME_WIFI_COLOR 0x364153
#define BTN_WC_COLOR        0x7B3306
#define BTN_UT_COLOR        0x35530E
#define BTN_KOP_COLOR       0x024A70
#define BTN_DS_COLOR        0x721378
#define BTN_OPEN_COLOR      0xC11007
#define BTN_CLOSED_COLOR    0x5EA529

#define EXT_COLOR 0x1C398E
#define TIME_WIFI_COLOR 0x364153
#define WC_COLOR 0x7B3306
#define UT_COLOR 0x35530E
#define KOP_COLOR 0x024A70
#define DS_COLOR 0x721378
#define WINDOW_OPEN_COLOR 0xC11007  // Red
#define WINDOW_CLOSED_COLOR 0x5EA529  // Green
#define LIGHT_YELLOW 0xD0872E  // For light on

// EXT gradient colors based on lux
#define EXT_DARK 0x0A0F1A
#define EXT_NIGHT 0x0F1A2E
#define EXT_TWILIGHT 0x1C2E5A
#define EXT_CLOUDY 0x2C5ABF
#define EXT_DAY 0x3A7CFF

// UI dimensions
#define CARD_RADIUS 12
#define CARD_MARGIN 8
#define ICON_SIZE 32

// Fonts (Montserrat sizes)
#define FONT_12 &lv_font_montserrat_12
#define FONT_14 &lv_font_montserrat_14
#define FONT_16 &lv_font_montserrat_16
#define FONT_20 &lv_font_montserrat_20
#define FONT_28 &lv_font_montserrat_28
#define FONT_22 &lv_font_montserrat_22
#define FONT_24 &lv_font_montserrat_24

// Graph points
#define GRAPH_POINTS 100

// Structs
struct Settings {
    uint8_t humThreshold;
    uint32_t fanDuration;
    int8_t tempMinThreshold;
    bool dndAllowAutomatic;
    uint8_t nndDays[7];
};

struct SensorData {
    float localTemp, localHumidity, localCO2, localLux;
    float extTemp, extHumidity, extPressure, extVOC, extLux;
    float lastTemp, lastHumidity, lastCO2;
    uint8_t errorFlags[5];
    uint8_t fans[6];
    uint8_t fanStates[4];
    uint8_t inputs[8];
    float bathroomTemp, bathroomHumidity, bathroomPressure;
    float utTemp, utHumidity;
    uint32_t offTimes[6];
    bool bathroomFan, utilityFan, dsFan;
    bool bathroomLight, utilityLight;
    bool skylightOpen, balconyDoorOpen;
    uint32_t bathroomFanStart, utilityFanStart, dsFanStart;
    float currentPower, energyConsumption;
    String weatherIcon;
    int weatherCode;
};

// Enums
enum TabType { TAB_ROOMS, TAB_GRAPHS };
enum GraphType { GRAPH_TEMP, GRAPH_HUM, GRAPH_CO2, GRAPH_FAN };
enum TouchAction { TOUCH_SHORT, TOUCH_LONG };
enum ErrorFlag {
    ERR_NONE = 0,
    ERR_SENSOR = 1,
    ERR_SD = 2,
    ERR_WIFI = 4,
    ERR_HTTP = 8
};

#define HISTORY_INTERVAL 300000UL
#define LOG_BUFFER_MAX 16384
#define ERR_FLOAT -999.0f
#define ERR_INT -999

#define LONG_PRESS_THRESHOLD 500
#define TOUCH_DEBOUNCE_MS 100

#endif // CONFIG_H
