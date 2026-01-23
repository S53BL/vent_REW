// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Definicije pinov
#define XPT2046_CS   33
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define PIN_RS485_TX 16
#define PIN_RS485_RX 17
#define PIN_RS485_DN 4
#define PIN_I2C_SDA 22
#define PIN_I2C_SCL 27
#define PIN_LDR      34

// Touch mapping constants
#define TOUCH_X_MIN 250
#define TOUCH_X_MAX 3700
#define TOUCH_Y_MIN 300
#define TOUCH_Y_MAX 3800

// I2C naslovi
#define SHT41_ADDRESS 0x44
#define SCD40_ADDRESS 0x62

// Osvetlitev zaslona v odstotkih
#define BACKLIGHT_PERCENT 50

// Barve v RGB565 formatu iz TFT_eSPI.h
#define CUSTOM_BROWN     0x9A60
#define CUSTOM_DARKGREEN 0x03E0
#define CUSTOM_DARKBLUE  0x000F
#define CUSTOM_BURGUNDY  0x7800
#define CUSTOM_PURPLE    0x780F
#define CUSTOM_DARKGREY  0x8410
#define TFT_WIFION       0x07E0
#define TFT_WIFIOFF      0xF800
#define TFT_WHITE        0xFFFF

// Koordinate in dimenzije
#define SQUARE_WIDTH  100
#define SQUARE_HEIGHT 70
#define SQUARE_RADIUS 10
#define SQUARE_GAP    5
#define SQUARE_Y      100
#define UPPER_HEIGHT  72
#define MIDDLE_HEIGHT 70
#define LOWER_HEIGHT  65
#define LOWER_Y       175
#define EXT_WIDTH     120
#define EXT_HEIGHT    95
#define EXT_X         5
#define EXT_Y         0
#define TIME_WIFI_WIDTH  185
#define TIME_WIFI_HEIGHT 95
#define TIME_WIFI_X      130
#define TIME_WIFI_Y      0

// Konstante za tekst in pisave
#define TEXT_OFFSET_X 10
#define TEXT_FONT_SIZE 2

// Konstante za grafe
#define GRAPH_POINTS 240
#define GRAPH_TEMP_MIN 0.0f
#define GRAPH_TEMP_MAX 35.0f
#define GRAPH_TEMP_STEP 5.0f
#define GRAPH_HUM_MIN 20.0f
#define GRAPH_HUM_MAX 100.0f
#define GRAPH_HUM_STEP 20.0f
#define GRAPH_CO2_MIN 400.0f
#define GRAPH_CO2_MAX 1200.0f
#define GRAPH_CO2_STEP 200.0f
#define GRAPH_LIGHT_MIN 0.0f
#define GRAPH_LIGHT_MAX 1000.0f
#define GRAPH_LIGHT_STEP 200.0f
#define GRAPH_FAN_MIN 0.0f
#define GRAPH_FAN_MAX 100.0f
#define GRAPH_FAN_STEP 20.0f

// Konstante
#define RS485_BAUD_RATE 19200
#define SENSOR_READ_INTERVAL 30
#define RS485_RESPONSE_TIMEOUT 5000
#define DND_START_HOUR 22
#define DND_START_MIN 0
#define DND_END_HOUR 6
#define DND_END_MIN 0
#define NND_START_HOUR 6
#define NND_START_MIN 0
#define NND_END_HOUR 16
#define NND_END_MIN 0
const bool NND_DAYS[7] = {true, true, true, true, true, false, false};

// Strukture
struct Settings {
  float humThreshold;
  int fanDuration;
  int fanOffDuration;
  int fanOffDurationKop;
  float tempLowThreshold;
  float tempMinThreshold;
  bool dndAllowAutomatic;
  bool dndAllowSemiautomatic;
  bool dndAllowManual;
  int cycleDurationDS;
  float cycleActivePercentDS;
  float humThresholdDS;
  float humThresholdHighDS;
  float humExtremeHighDS;
  int co2ThresholdLowDS;
  int co2ThresholdHighDS;
  float incrementPercentLowDS;
  float incrementPercentHighDS;
  float incrementPercentTempDS;
  float tempIdealDS;
  float tempExtremeHighDS;
  float tempExtremeLowDS;
} __attribute__((packed));  // Dodano za odstranitev padding-a in preprečitev offset mismatch

struct Square {
  int x, y;
  int width, height;
  uint16_t color;
  uint16_t originalColor;
  const char* label;
  bool border;
  bool disabled;
  bool fanActive;
  unsigned long fanRemainingTime;
  bool lightOn;
  uint8_t roomId;  // ID prostora za manual control (0=KOP, 1=UT, 2=WC, 3=DS)
  bool prevFanActive;
  unsigned long prevFanRemainingTime;
  bool prevLightOn; // Dodano za sledenje spremembam lightOn
};

struct SensorData {
  float localTemp; // Temperatura DS, inicialno 0.0
  float localHumidity; // Vlaga DS, inicialno 0.0
  float localCO2; // CO2 DS, inicialno 0.0
  float localLight; // Svetlost DS, inicialno 0.0
  float extTemp; // Zunanja temperatura, inicialno 0.0
  float extHumidity; // Zunanja vlaga, inicialno 0.0
  float extPressure; // Zunanji tlak, inicialno 0.0
  float extVOC; // Zunanji VOC, inicialno 0.0
  float kopTemp; // Temperatura Kopalnica, inicialno 0.0
  float kopHumidity; // Vlaga Kopalnica, inicialno 0.0
  float kopPressure; // Tlak Kopalnica, inicialno 0.0
  float utTemp; // Temperatura Utility, inicialno 0.0
  float utHumidity; // Vlaga Utility, inicialno 0.0
  float prevLocalTemp; // Predhodna lokalna temperatura, inicialno 0.0
  float prevLocalHumidity; // Predhodna lokalna vlaga, inicialno 0.0
  float prevLocalCO2; // Predhodni lokalni CO2, inicialno 0.0
  float prevLocalLight; // Predhodna lokalna svetlost, inicialno 0.0
  float prevExtTemp; // Predhodna zunanja temperatura, inicialno 0.0
  float prevExtHumidity; // Predhodna zunanja vlaga, inicialno 0.0
  float prevExtPressure; // Predhodni zunanji tlak, inicialno 0.0
  float prevExtVOC; // Predhodni zunanji VOC, inicialno 0.0
  float prevKopTemp; // Predhodna temperatura Kopalnica, inicialno 0.0
  float prevKopHumidity; // Predhodna vlaga Kopalnica, inicialno 0.0
  float prevKopPressure; // Predhodni tlak Kopalnica, inicialno 0.0
  float prevUtTemp; // Predhodna temperatura Utility, inicialno 0.0
  float prevUtHumidity; // Predhodna vlaga Utility, inicialno 0.0
  float currentPower; // Trenutna moč, inicialno 0.0
  float energyConsumption; // Poraba energije, inicialno 0.0
  float prevCurrentPower; // Predhodna trenutna moč, inicialno 0.0
  float prevEnergyConsumption; // Predhodna poraba energije, inicialno 0.0
  uint8_t errorFlags[5]; // Napake, inicialno 0
  uint8_t fans[6]; // Stanje ventilatorjev, inicialno 0
  uint8_t inputs[8]; // Vhodi, inicialno 0
  uint32_t offTimes[6]; // Časi izklopa, inicialno 0
};

// Enumeracije
enum DisplayState {
  DISPLAY_STATE_SQUARES,
  DISPLAY_STATE_GRAPH
};

enum class GraphType {
  TEMP = 0x01,
  HUM = 0x02,
  CO2LIGHT = 0x03,
  FAN = 0x04
};

enum MessageType {
  SENSOR_DATA = 1,
  GRAPH_REQUEST = 2,
  MANUAL_CONTROL = 4,
  CONFIRM_ACTION = 6,
  STATE_REQUEST = 7,
  STATUS_UPDATE = 8,
  HISTORY_REQUEST = 9,
  GRAPH_DATA = 3,
  HISTORY_RESPONSE = 11,
  PARAM_UPDATE = 14,
  PARAM_CONFIRM = 10
};

enum SquareIndex {
  EXT,
  UT,
  KOP,
  WC,
  DS,
  TIME_WIFI
};

constexpr int numSquares = 6; // Dodano za konstantno velikost array-a

#endif // CONFIG_H

// config.h