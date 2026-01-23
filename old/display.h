// display.h
#ifndef DISPLAY_H
#define DISPLAY_H

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include "ezTime.h"
#include "485.h"
#include "config.h"

extern TFT_eSPI tft;
extern XPT2046_Touchscreen ts;
extern SPIClass touchSpi;
extern Square squares[numSquares];
extern uint8_t temperatureGraph[4][GRAPH_POINTS];
extern uint8_t humidityGraph[4][GRAPH_POINTS];
extern uint8_t fanGraph[4][GRAPH_POINTS];
extern uint8_t co2LightGraph[2][GRAPH_POINTS];
extern String lastTimeStr;
extern String lastDateStr;
extern bool lastWiFiStatus;
extern String currentSSID;
extern String lastSSIDDisplay;
extern bool lastWebStatus;
extern DisplayState currentDisplayState;
extern GraphType currentGraphType;
extern unsigned long lastUpdateEXT;
extern unsigned long lastUpdateLocal;
extern unsigned long lastUpdateFans;
extern unsigned long lastUpdateRooms;
extern Timezone myTZ;
extern bool needsRedraw[numSquares];
extern SensorData sensorData;
extern bool webServerRunning;
extern int httpStatus;
extern unsigned long httpStatusTime;

enum class TouchState {
  IDLE,
  TOUCH_BEGIN,
  TOUCH_HOLD,
  TOUCH_END
};

struct DisplayContext {
  SensorData& sensorData;
  Square* squares;
  String& ssid;
  bool webServerRunning;
  bool timeSet;
};

class SquareManager {
public:
  void initializeSquares();
  void drawSquare(SquareIndex index, DisplayContext& context);
  void updateNeedsRedraw(SquareIndex index, DisplayContext& context);
  bool shouldRedraw(SquareIndex index, DisplayContext& context);
private:
  void drawExternalSquare(SquareIndex index, DisplayContext& context);
  void drawRoomSquare(SquareIndex index, DisplayContext& context);
  void drawTimeWiFiSquare(SquareIndex index, DisplayContext& context);
  void drawFanTime(SquareIndex index, DisplayContext& context);
};

class GraphRenderer {
public:
  void drawGraph(GraphType type, DisplayContext& context);
  void drawYAxis(float minVal, float maxVal, float step, bool left);
  void drawGraphCurve(uint8_t* dataPtr, uint16_t color, int offsetX, int offsetY, float scaleX, float scaleY);
  void drawGraphLegend(GraphType type);
  void drawGraphTitle(GraphType type);
};

extern SquareManager squareManager;
extern GraphRenderer graphRenderer;

void initDisplay();
void drawTimeAndWiFi();
void drawSensorData();
void handleTouch();
void drawTextIfChanged(int x, int y, const String& text, uint16_t fgColor, uint16_t bgColor);
void drawCenteredText(const String& text, int x, int y, int fontSize, uint16_t fgColor, uint16_t bgColor);
void processStatusUpdate();  // Nova funkcija za takojšnje osveževanje po STATUS_UPDATE
void updateFanTimers();  // Nova funkcija za sekundno odštevanje fanRemainingTime

#endif // DISPLAY_H
// display.h