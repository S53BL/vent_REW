// display.cpp

#include "display.h"
#include "config.h"
#include "web.h"

TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
SPIClass touchSpi(VSPI);
Square squares[numSquares];
uint8_t temperatureGraph[4][GRAPH_POINTS] = {0};
uint8_t humidityGraph[4][GRAPH_POINTS] = {0};
uint8_t fanGraph[4][GRAPH_POINTS] = {0};
uint8_t co2LightGraph[2][GRAPH_POINTS] = {0};

String lastTimeStr = "";
String lastDateStr = "";
bool lastWiFiStatus = false;
String currentSSID = "";
String lastSSIDDisplay = "";
bool lastWebStatus = false;
static unsigned long touchStartTime = 0;
static int activeSquare = -1;
static bool longPressHandled = false;
unsigned long lastUpdateEXT = 0;
unsigned long lastUpdateLocal = 0;
unsigned long lastUpdateFans = 0;
unsigned long lastUpdateRooms = 0;
bool needsRedraw[numSquares] = {false};
DisplayState currentDisplayState = DISPLAY_STATE_SQUARES;
GraphType currentGraphType = GraphType::TEMP;

SquareManager squareManager;
GraphRenderer graphRenderer;

static bool prevTimeSet = false;

extern unsigned long lastValidTime, lastTimeUpdate;

void initDisplay() {
  Serial.println("[Display] Inicializiram TFT zaslon...");
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  analogWrite(21, (BACKLIGHT_PERCENT * 255) / 100);
  Serial.println("[Display] TFT zaslon inicializiran.");

  Serial.println("[Display] Inicializiram touchscreen...");
  touchSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  if (ts.begin(touchSpi)) {
    ts.setRotation(1);
    Serial.println("[Display] Touchscreen inicializiran.");
  } else {
    Serial.println("[Display] Napaka: Touchscreen inicializacija neuspešna");
  }

  squareManager.initializeSquares();

  DisplayContext context = {sensorData, squares, currentSSID, webServerRunning, (timeStatus() == timeSet)};
  for (int i = 0; i < numSquares; i++) {
    squareManager.drawSquare(static_cast<SquareIndex>(i), context);
  }
  for (int i = 0; i < numSquares; i++) {
    needsRedraw[i] = true;
  }
}

void drawTimeAndWiFi() {
  static String prevTimeStr = "";
  static String prevDateStr = "";
  static bool prevWiFiStatus = false;
  static bool initialLog = true;
  static bool lastTimeSetState = false;

  bool isTimeSet = (timeStatus() == timeSet);
  if (isTimeSet != lastTimeSetState) {
    needsRedraw[TIME_WIFI] = true;
  }
  lastTimeSetState = isTimeSet;

  String timeStr, dateStr;
  bool currentWiFiStatus = (WiFi.status() == WL_CONNECTED);
  unsigned long currentTime = lastValidTime + (millis() - lastTimeUpdate) / 1000;
  timeStr = myTZ.dateTime(currentTime, "H:i");
  dateStr = myTZ.dateTime(currentTime, "d.m.Y");

  if (initialLog || timeStr != prevTimeStr || dateStr != prevDateStr || currentWiFiStatus != prevWiFiStatus) {
    Serial.println("[Display] Lokalni čas: " + myTZ.dateTime(currentTime, "Y-m-d H:i:s") + 
                   ", DST: " + String(myTZ.isDST() ? "CEST" : "CET"));
    initialLog = false;
  }

  prevTimeStr = timeStr;
  prevDateStr = dateStr;
  prevWiFiStatus = currentWiFiStatus;
  prevTimeSet = isTimeSet;

  DisplayContext context = {sensorData, squares, currentSSID, webServerRunning, isTimeSet};
  squareManager.drawSquare(TIME_WIFI, context);
}

void drawSensorData() {
  static unsigned long lastFullRedraw = 0;
  bool updateEXT = (httpStatus == 1 && millis() - httpStatusTime < 5000) || millis() - lastUpdateEXT >= 180000 || lastUpdateEXT == 0;
  bool updateLocal = (millis() - lastUpdateLocal >= SENSOR_READ_INTERVAL * 1000) || 
                     sensorData.localTemp != sensorData.prevLocalTemp || 
                     sensorData.localHumidity != sensorData.prevLocalHumidity || 
                     sensorData.localCO2 != sensorData.prevLocalCO2 || 
                     sensorData.localLight != sensorData.prevLocalLight || 
                     lastUpdateLocal == 0;
  bool updateRooms = (millis() - lastUpdateRooms >= 60000) || lastUpdateRooms == 0;
  bool updateFans = false;

  unsigned long currentTime = lastValidTime + (millis() - lastTimeUpdate) / 1000;

  // Preračun remaining za vse kvadratke z ventilatorji
  int roomMap[] = {0, 1, 0, 2, 3}; // Map: EXT=0 (ni), UT=1, KOP=0, WC=2, DS=3
  for (int i = 1; i <= 4; i++) {
    if (squares[i].fanActive) {
      unsigned long newRemaining = (sensorData.offTimes[roomMap[i]] > currentTime) ? sensorData.offTimes[roomMap[i]] - currentTime : 0;
      newRemaining = std::min(newRemaining, 999UL);
      if (newRemaining != squares[i].fanRemainingTime) {
        squares[i].fanRemainingTime = newRemaining;
        needsRedraw[i] = true;
        if (newRemaining == 0) {
          squares[i].fanActive = false;
          squares[i].border = false;
          Serial.println("[Ventilator] " + String(squares[i].label) + " ugasnjen");
        }
      }
      updateFans = true;
    }
  }

  if (millis() - lastFullRedraw >= 10000) {
    for (int i = 0; i < numSquares; i++) {
      needsRedraw[i] = true;
    }
    lastFullRedraw = millis();
  }

  DisplayContext context = {sensorData, squares, currentSSID, webServerRunning, (timeStatus() == timeSet)};

  if (updateEXT) needsRedraw[EXT] = true;
  if (updateLocal) needsRedraw[DS] = true;
  if (updateRooms) {
    needsRedraw[UT] = true;
    needsRedraw[KOP] = true;
    needsRedraw[WC] = true;
  }

  // Redraw samo če needsRedraw
  if (needsRedraw[EXT]) {
    tft.fillRoundRect(squares[EXT].x, squares[EXT].y, squares[EXT].width, squares[EXT].height, SQUARE_RADIUS, TFT_BLACK);
    squareManager.drawSquare(EXT, context);
    lastUpdateEXT = millis();
    sensorData.prevExtTemp = sensorData.extTemp;
    sensorData.prevExtHumidity = sensorData.extHumidity;
    sensorData.prevExtPressure = sensorData.extPressure;
    sensorData.prevExtVOC = sensorData.extVOC;
    needsRedraw[EXT] = false;
    if (sensorData.extTemp != sensorData.prevExtTemp || sensorData.extHumidity != sensorData.prevExtHumidity || 
        sensorData.extPressure != sensorData.prevExtPressure || sensorData.extVOC != sensorData.prevExtVOC) {
      Serial.println("[Display] Redrawing square EXT due to change");
    }
  }

  if (needsRedraw[DS]) {
    tft.fillRoundRect(squares[DS].x, squares[DS].y, squares[DS].width, squares[DS].height, SQUARE_RADIUS, TFT_BLACK);
    squareManager.drawSquare(DS, context);
    lastUpdateLocal = millis();
    sensorData.prevLocalTemp = sensorData.localTemp;
    sensorData.prevLocalHumidity = sensorData.localHumidity;
    sensorData.prevLocalCO2 = sensorData.localCO2;
    sensorData.prevLocalLight = sensorData.localLight;
    needsRedraw[DS] = false;
    if (sensorData.localTemp != sensorData.prevLocalTemp || sensorData.localHumidity != sensorData.prevLocalHumidity || 
        sensorData.localCO2 != sensorData.prevLocalCO2 || sensorData.localLight != sensorData.prevLocalLight) {
      Serial.println("[Display] Redrawing square DS due to change");
    }
  }

  // Za ventilatorje (UT, KOP, WC, DS) - redraw če needsRedraw ali updateFans
  if (updateFans || updateRooms) {
    for (int i = 1; i <= 4; i++) {
      if (needsRedraw[i]) {
        tft.fillRoundRect(squares[i].x, squares[i].y, squares[i].width, squares[i].height, SQUARE_RADIUS, TFT_BLACK);
        squares[i].prevFanActive = squares[i].fanActive;
        squares[i].prevFanRemainingTime = squares[i].fanRemainingTime;
        squareManager.drawSquare(static_cast<SquareIndex>(i), context);
        needsRedraw[i] = false;
        if (squares[i].fanRemainingTime != squares[i].prevFanRemainingTime || squares[i].fanActive != squares[i].prevFanActive) {
          Serial.println("[Display] Redrawing square " + String(squares[i].label) + " due to fan change");
        }
      }
    }
    if (updateFans) lastUpdateFans = millis();
    if (updateRooms) lastUpdateRooms = millis();
  }

  if (needsRedraw[TIME_WIFI]) {
    tft.fillRoundRect(squares[TIME_WIFI].x, squares[TIME_WIFI].y, squares[TIME_WIFI].width, squares[TIME_WIFI].height, SQUARE_RADIUS, TFT_BLACK);
    squareManager.drawSquare(TIME_WIFI, context);
    sensorData.prevCurrentPower = sensorData.currentPower;
    sensorData.prevEnergyConsumption = sensorData.energyConsumption;
    needsRedraw[TIME_WIFI] = false;
  }
}

bool isTouchInRect(int touchX, int touchY, int x, int y, int w, int h) {
  return (touchX >= x && touchX <= x + w && touchY >= y && touchY <= y + h);
}

void handleTouch() {
  static unsigned long lastTouch = 0;
  static int lastTouchX = -1;
  static int lastTouchY = -1;
  static TouchState touchState = TouchState::IDLE;

  if (millis() - lastTouch < 200) return;

  bool isTouched = ts.tirqTouched() && ts.touched();

  DisplayContext context = {sensorData, squares, currentSSID, webServerRunning, (timeStatus() == timeSet)};

  switch (touchState) {
    case TouchState::IDLE:
      if (isTouched) {
        TS_Point p = ts.getPoint();
        int touchX = map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, 320);
        int touchY = map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, 240);

        if (abs(touchX - lastTouchX) <= 5 && abs(touchY - lastTouchY) <= 5 && lastTouchX != -1) {
          return;
        }

        lastTouchX = touchX;
        lastTouchY = touchY;

        if (currentDisplayState == DISPLAY_STATE_SQUARES) {
          for (int i = 0; i < numSquares; i++) {
            if (isTouchInRect(touchX, touchY, squares[i].x, squares[i].y, squares[i].width, squares[i].height)) {
              activeSquare = i;
              touchStartTime = millis();
              longPressHandled = false;
              Serial.println("[Touch] Pritisk na gumb: " + String(squares[i].label));
              touchState = TouchState::TOUCH_BEGIN;
              break;
            }
          }
        } else if (currentDisplayState == DISPLAY_STATE_GRAPH) {
          activeSquare = 99;
          touchStartTime = millis();
          longPressHandled = false;
          touchState = TouchState::TOUCH_BEGIN;
        }
      }
      break;

    case TouchState::TOUCH_BEGIN:
      if (isTouched) {
        unsigned long touchDuration = millis() - touchStartTime;
        if (touchDuration >= 2000) {
          touchState = TouchState::TOUCH_HOLD;
        }
      } else {
        touchState = TouchState::TOUCH_END;
      }
      break;

    case TouchState::TOUCH_HOLD:
      if (!longPressHandled) {
        if (currentDisplayState == DISPLAY_STATE_SQUARES) {
          int i = activeSquare;
          if (i == 0) {
            sendSensorData();
            Serial.println("[Touch] Dolgi dotik na EXT: Poslan SENSOR_DATA");
            longPressHandled = true;
          } else if (i >= 1 && i <= 4) {
            if (squares[i].disabled) {
              squares[i].disabled = false;
              squares[i].color = squares[i].originalColor;
              sendManualControl(squares[i].roomId, 0);
              Serial.println("[Touch] Dolgi dotik: " + String(squares[i].label) + " omogočen");
              Serial.println("[Touch] Disabled stanje: " + String(squares[i].label) + " enabled");
            } else {
              squares[i].disabled = true;
              squares[i].border = false;
              squares[i].color = CUSTOM_DARKGREY;
              sendManualControl(squares[i].roomId, 1);
              Serial.println("[Touch] Dolgi dotik: " + String(squares[i].label) + " onemogočen");
              Serial.println("[Touch] Disabled stanje: " + String(squares[i].label) + " disabled");
            }
            tft.fillRoundRect(squares[i].x, squares[i].y, squares[i].width, squares[i].height, SQUARE_RADIUS, TFT_BLACK);
            squareManager.drawSquare(static_cast<SquareIndex>(i), context);
            longPressHandled = true;
          }
        } else if (currentDisplayState == DISPLAY_STATE_GRAPH) {
          currentDisplayState = DISPLAY_STATE_SQUARES;
          tft.fillScreen(TFT_BLACK);
          for (int i = 0; i < numSquares; i++) {
            tft.fillRoundRect(squares[i].x, squares[i].y, squares[i].width, squares[i].height, SQUARE_RADIUS, TFT_BLACK);
            squareManager.drawSquare(static_cast<SquareIndex>(i), context);
          }
          longPressHandled = true;
          Serial.println("[Graph] Dolgi dotik: Vrnitev v squares mode");
        }
      }
      if (!isTouched) {
        touchState = TouchState::TOUCH_END;
      }
      break;

    case TouchState::TOUCH_END:
      if (!longPressHandled) {
        if (currentDisplayState == DISPLAY_STATE_SQUARES) {
          int i = activeSquare;
          if (i == 0) {
            currentDisplayState = DISPLAY_STATE_GRAPH;
            currentGraphType = GraphType::TEMP;
            tft.fillScreen(TFT_BLACK);
            graphRenderer.drawGraph(currentGraphType, context);
            sendGraphRequest(currentGraphType);
            Serial.println("[Touch] Kratek dotik na EXT: Preklop v graph mode (TEMP)");
          } else if (i >= 1 && i <= 4) {
            sendManualControl(squares[i].roomId, 0);
            Serial.println("[Touch] Kratek dotik: " + String(squares[i].label) + " vklopljen");
            tft.fillRoundRect(squares[i].x, squares[i].y, squares[i].width, squares[i].height, SQUARE_RADIUS, TFT_BLACK);
            squareManager.drawSquare(static_cast<SquareIndex>(i), context);
          } else if (i == TIME_WIFI) {
            // Odstranjeno border toggle za konsolidacijo
            tft.fillRoundRect(squares[i].x, squares[i].y, squares[i].width, squares[i].height, SQUARE_RADIUS, TFT_BLACK);
            squareManager.drawSquare(static_cast<SquareIndex>(i), context);
          }
        } else if (currentDisplayState == DISPLAY_STATE_GRAPH) {
          int nextType = (static_cast<int>(currentGraphType) % 4) + 1;
          currentGraphType = static_cast<GraphType>(nextType);
          tft.fillScreen(TFT_BLACK);
          graphRenderer.drawGraph(currentGraphType, context);
          sendGraphRequest(currentGraphType);
          Serial.println("[Graph] Kratek dotik: Preklop na graf tip " + String(static_cast<int>(currentGraphType)));
        }
      }
      activeSquare = -1;
      longPressHandled = false;
      lastTouch = millis();
      touchState = TouchState::IDLE;
      break;
  }
}

void processStatusUpdate() {
  // Enostavna rešitev: Rišemo vse znova ob vsakem STATUS_UPDATE
  for (int i = 0; i < numSquares; i++) {
    needsRedraw[i] = true;
  }
  drawSensorData();
}

void updateFanTimers() {
  unsigned long currentTime = lastValidTime + (millis() - lastTimeUpdate) / 1000;
  int roomMap[] = {0, 1, 0, 2, 3};
  for (int i = 1; i <= 4; i++) {
    if (squares[i].fanActive && squares[i].fanRemainingTime > 0) {
      squares[i].fanRemainingTime--;
      if (squares[i].fanRemainingTime == 0) {
        squares[i].fanActive = false;
        squares[i].border = false;
        Serial.println("[Ventilator] " + String(squares[i].label) + " ugasnjen po odštevanju");
      }
      needsRedraw[i] = true;  // Osveži kvadratek za vidno odštevanje
    }
  }
}

void drawTextIfChanged(int x, int y, const String& text, uint16_t fgColor, uint16_t bgColor) {
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setTextColor(fgColor, bgColor);
  tft.drawString(text, x, y);
}

void drawCenteredText(const String& text, int x, int y, int fontSize, uint16_t fgColor, uint16_t bgColor) {
  tft.setTextFont(2);
  tft.setTextSize(fontSize);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(fgColor, bgColor);
  tft.drawString(text, x, y);
  tft.setTextDatum(TL_DATUM);
}

void SquareManager::initializeSquares() {
  squares[EXT] = {EXT_X, EXT_Y, EXT_WIDTH, EXT_HEIGHT, CUSTOM_DARKBLUE, CUSTOM_DARKBLUE, "EXT", false, false, false, 0, false, 255, false, 0, false};
  squares[UT] = {SQUARE_GAP, SQUARE_Y, SQUARE_WIDTH, SQUARE_HEIGHT, CUSTOM_BROWN, CUSTOM_BROWN, "UT", false, false, false, 0, false, 1, false, 0, false};
  squares[KOP] = {SQUARE_GAP * 2 + SQUARE_WIDTH, SQUARE_Y, SQUARE_WIDTH, SQUARE_HEIGHT, CUSTOM_DARKGREEN, CUSTOM_DARKGREEN, "KOP", false, false, false, 0, false, 0, false, 0, false};
  squares[WC] = {SQUARE_GAP * 3 + SQUARE_WIDTH * 2, SQUARE_Y, SQUARE_WIDTH, SQUARE_HEIGHT, CUSTOM_PURPLE, CUSTOM_PURPLE, "WC", false, false, false, 0, false, 2, false, 0, false};
  squares[DS] = {SQUARE_GAP, LOWER_Y, 320 - 2 * SQUARE_GAP, LOWER_HEIGHT, CUSTOM_BURGUNDY, CUSTOM_BURGUNDY, "DS", false, false, false, 0, false, 3, false, 0, false};
  squares[TIME_WIFI] = {TIME_WIFI_X, TIME_WIFI_Y, TIME_WIFI_WIDTH, TIME_WIFI_HEIGHT, CUSTOM_BURGUNDY, CUSTOM_BURGUNDY, "TIME_WIFI", false, false, false, 0, false, 255, false, 0, false};
}

void SquareManager::drawSquare(SquareIndex index, DisplayContext& context) {
  tft.setTextFont(2);
  tft.setTextSize(1);
  switch (index) {
    case EXT:
      drawExternalSquare(index, context);
      break;
    case UT:
    case KOP:
    case WC:
    case DS:
      drawRoomSquare(index, context);
      break;
    case TIME_WIFI:
      drawTimeWiFiSquare(index, context);
      break;
  }
}

void SquareManager::updateNeedsRedraw(SquareIndex index, DisplayContext& context) {
  needsRedraw[index] = shouldRedraw(index, context);
}

bool SquareManager::shouldRedraw(SquareIndex index, DisplayContext& context) {
  switch (index) {
    case EXT:
      return (context.sensorData.extTemp != context.sensorData.prevExtTemp ||
              context.sensorData.extHumidity != context.sensorData.prevExtHumidity ||
              context.sensorData.extPressure != context.sensorData.prevExtPressure ||
              context.sensorData.extVOC != context.sensorData.prevExtVOC ||
              squares[index].border != squares[index].disabled);
    case UT:
      return (squares[index].fanActive != squares[index].prevFanActive ||
              squares[index].fanRemainingTime != squares[index].prevFanRemainingTime ||
              squares[index].lightOn != squares[index].prevLightOn ||
              context.sensorData.utTemp != context.sensorData.prevUtTemp ||
              context.sensorData.utHumidity != context.sensorData.prevUtHumidity);
    case KOP:
      return (squares[index].fanActive != squares[index].prevFanActive ||
              squares[index].fanRemainingTime != squares[index].prevFanRemainingTime ||
              squares[index].lightOn != squares[index].prevLightOn ||
              context.sensorData.kopTemp != context.sensorData.prevKopTemp ||
              context.sensorData.kopHumidity != context.sensorData.prevKopHumidity);
    case WC:
      return (squares[index].fanActive != squares[index].prevFanActive ||
              squares[index].fanRemainingTime != squares[index].prevFanRemainingTime ||
              squares[index].lightOn != squares[index].prevLightOn ||
              context.sensorData.kopPressure != context.sensorData.prevKopPressure);
    case DS:
      return (context.sensorData.localTemp != context.sensorData.prevLocalTemp ||
              context.sensorData.localHumidity != context.sensorData.prevLocalHumidity ||
              context.sensorData.localCO2 != context.sensorData.prevLocalCO2 ||
              context.sensorData.localLight != context.sensorData.prevLocalLight ||
              squares[index].fanActive != squares[index].prevFanActive ||
              squares[index].fanRemainingTime != squares[index].prevFanRemainingTime);
    case TIME_WIFI:
      return (context.timeSet != prevTimeSet ||
              context.sensorData.currentPower != context.sensorData.prevCurrentPower ||
              context.sensorData.energyConsumption != context.sensorData.prevEnergyConsumption ||
              context.webServerRunning != lastWebStatus ||
              context.ssid != lastSSIDDisplay ||
              WiFi.status() != lastWiFiStatus);
    default:
      return false;
  }
}

void SquareManager::drawExternalSquare(SquareIndex index, DisplayContext& context) {
  uint16_t currentColor = squares[index].disabled ? CUSTOM_DARKGREY : squares[index].color;
  tft.fillRoundRect(squares[index].x, squares[index].y, squares[index].width, squares[index].height, SQUARE_RADIUS, currentColor);

  uint16_t textColor = TFT_WHITE;
  tft.setTextColor(textColor, currentColor);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setTextDatum(TL_DATUM);

  drawTextIfChanged(squares[index].x + 10, squares[index].y + 15, "T: " + String(context.sensorData.extTemp, 1) + " °C", textColor, currentColor);
  drawTextIfChanged(squares[index].x + 10, squares[index].y + 35, "H: " + String(context.sensorData.extHumidity, 1) + " %", textColor, currentColor);
  drawTextIfChanged(squares[index].x + 10, squares[index].y + 55, "P: " + String(context.sensorData.extPressure, 1) + " hPa", textColor, currentColor);
  drawTextIfChanged(squares[index].x + 10, squares[index].y + 75, "VOC: " + String((int)context.sensorData.extVOC), textColor, currentColor);

  if (squares[index].border && !squares[index].disabled) {
    tft.drawRoundRect(squares[index].x + 3, squares[index].y + 3, squares[index].width - 6, squares[index].height - 6, SQUARE_RADIUS - 3, TFT_WHITE);
  }
}

void SquareManager::drawRoomSquare(SquareIndex index, DisplayContext& context) {
  bool lightOn = false;
  int roomMap[] = {0, 1, 0, 2, 3}; // EXT ni, UT=1, KOP=0, WC=2, DS=3
  if (index == KOP) lightOn = context.sensorData.inputs[4] || context.sensorData.inputs[5];
  else if (index == UT) lightOn = context.sensorData.inputs[6];
  else if (index == WC) lightOn = context.sensorData.inputs[7];
  
  if (lightOn != squares[index].prevLightOn) {
    Serial.println("[Display] LightOn changed for square " + String(squares[index].label) + ": " + String(lightOn));
    squares[index].prevLightOn = lightOn;
    needsRedraw[index] = true;
  }

  uint16_t currentColor = squares[index].disabled ? CUSTOM_DARKGREY : squares[index].color;
  tft.fillRoundRect(squares[index].x, squares[index].y, squares[index].width, squares[index].height, SQUARE_RADIUS, currentColor);

  if (index >= UT && index <= WC && lightOn) {
    tft.fillCircle(squares[index].x + squares[index].width / 2, squares[index].y + squares[index].height / 2, 32, TFT_YELLOW);
  }

  uint16_t textColor = (index >= UT && index <= WC && lightOn) ? TFT_BLACK : TFT_WHITE;
  tft.setTextColor(textColor, lightOn ? TFT_YELLOW : currentColor);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setTextDatum(TL_DATUM);

  if (squares[index].fanActive) {
    drawFanTime(index, context);
  } else if (!lightOn) {
    switch (index) {
      case UT:
        drawTextIfChanged(squares[index].x + SQUARE_WIDTH / 2 - TEXT_OFFSET_X, squares[index].y + TEXT_OFFSET_X, squares[index].label, textColor, currentColor);
        drawTextIfChanged(squares[index].x + SQUARE_WIDTH / 2 - 35, squares[index].y + 30, "T: " + String(context.sensorData.utTemp, 1) + " °C", textColor, currentColor);  // Pomik -15px
        drawTextIfChanged(squares[index].x + SQUARE_WIDTH / 2 - 35, squares[index].y + 50, "H: " + String(context.sensorData.utHumidity, 1) + " %", textColor, currentColor);  // Pomik -15px
        break;
      case KOP:
        drawTextIfChanged(squares[index].x + SQUARE_WIDTH / 2 - TEXT_OFFSET_X, squares[index].y + TEXT_OFFSET_X, squares[index].label, textColor, currentColor);
        drawTextIfChanged(squares[index].x + SQUARE_WIDTH / 2 - 35, squares[index].y + 30, "T: " + String(context.sensorData.kopTemp, 1) + " °C", textColor, currentColor);  // Pomik -15px
        drawTextIfChanged(squares[index].x + SQUARE_WIDTH / 2 - 35, squares[index].y + 50, "H: " + String(context.sensorData.kopHumidity, 1) + " %", textColor, currentColor);  // Pomik -15px
        break;
      case WC:
        drawTextIfChanged(squares[index].x + SQUARE_WIDTH / 2 - TEXT_OFFSET_X, squares[index].y + TEXT_OFFSET_X, squares[index].label, textColor, currentColor);
        drawTextIfChanged(squares[index].x + SQUARE_WIDTH / 2 - 35, squares[index].y + 50, "P: " + String(context.sensorData.kopPressure, 1) + " hPa", textColor, currentColor);  // Pomik -15px
        break;
      case DS:
        drawTextIfChanged(squares[index].x + 10, squares[index].y + 10, squares[index].label, textColor, currentColor);
        drawTextIfChanged(squares[index].x + 110, squares[index].y + 10, "T: " + String(context.sensorData.localTemp, 1) + " °C", textColor, currentColor);
        drawTextIfChanged(squares[index].x + 10, squares[index].y + 35, "L: " + String(context.sensorData.localLight, 0) + " %", textColor, currentColor);
        drawTextIfChanged(squares[index].x + 110, squares[index].y + 35, "H: " + String(context.sensorData.localHumidity, 1) + " %", textColor, currentColor);
        drawTextIfChanged(squares[index].x + 200, squares[index].y + 35, "CO2: " + String((int)context.sensorData.localCO2) + " ppm", textColor, currentColor);  // Samo CO2 na x+200
        
        // Dodatek za okna v DS: STREHA (inputs[2]) na x+200, y+10; VRATA (inputs[3]) na x+260, y+10, če je zaprto (!inputs)
        if (!context.sensorData.inputs[2]) {
          drawTextIfChanged(squares[index].x + 200, squares[index].y + 10, "STREHA ", textColor, currentColor);  // Dodan presledek
        }
        if (!context.sensorData.inputs[3]) {
          drawTextIfChanged(squares[index].x + 260, squares[index].y + 10, "VRATA", textColor, currentColor);
        }
        break;
    }
  } else {
    drawTextIfChanged(squares[index].x + SQUARE_WIDTH / 2 - TEXT_OFFSET_X, squares[index].y + TEXT_OFFSET_X, squares[index].label, textColor, TFT_YELLOW);
  }

  if (squares[index].border && !squares[index].disabled && !lightOn) {
    tft.drawRoundRect(squares[index].x + 3, squares[index].y + 3, squares[index].width - 6, squares[index].height - 6, SQUARE_RADIUS - 3, TFT_WHITE);
  }
}

void SquareManager::drawFanTime(SquareIndex index, DisplayContext& context) {
  uint16_t currentColor = squares[index].disabled ? CUSTOM_DARKGREY : squares[index].color;
  uint16_t textColor = TFT_WHITE;
  tft.setTextColor(textColor, currentColor);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setTextDatum(TL_DATUM);
  drawTextIfChanged(squares[index].x + SQUARE_WIDTH / 2 - TEXT_OFFSET_X, squares[index].y + TEXT_OFFSET_X, squares[index].label, textColor, currentColor);

  // Uspešen popravek: Dvojna velikost, brez "s", sredinsko pod label-om
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  String remainingStr = String(squares[index].fanRemainingTime);
  tft.drawString(remainingStr, squares[index].x + squares[index].width / 2, squares[index].y + 40);
  tft.setTextDatum(TL_DATUM);  // Ponastavi
  tft.setTextSize(1);  // Ponastavi
}

void SquareManager::drawTimeWiFiSquare(SquareIndex index, DisplayContext& context) {
  uint16_t currentColor = squares[index].disabled ? CUSTOM_DARKGREY : squares[index].color;
  tft.fillRoundRect(squares[index].x, squares[index].y, squares[index].width, squares[index].height, SQUARE_RADIUS, currentColor);

  uint16_t textColor = TFT_WHITE;
  tft.setTextColor(textColor, currentColor);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setTextDatum(TL_DATUM);

  unsigned long currentTime = lastValidTime + (millis() - lastTimeUpdate) / 1000;
  String timeStr = context.timeSet ? "Ura: " + myTZ.dateTime(currentTime, "H:i") : "Čas: ERR";
  String dateStr = context.timeSet ? myTZ.dateTime(currentTime, "d.m.Y") : "--";
  bool currentWiFiStatus = (WiFi.status() == WL_CONNECTED);
  String ssidDisplay = currentWiFiStatus && context.ssid.length() >= 2 ? context.ssid.substring(0, 2) : "--";
  uint16_t wifiColor = currentWiFiStatus ? TFT_WIFION : TFT_WIFIOFF;
  uint16_t webColor = context.webServerRunning ? TFT_WIFION : TFT_WIFIOFF;

  tft.setTextColor(textColor, currentColor);
  drawTextIfChanged(squares[index].x + 10, squares[index].y + 10, timeStr, textColor, currentColor);
  drawTextIfChanged(squares[index].x + 10, squares[index].y + 25, dateStr, textColor, currentColor);
  drawTextIfChanged(squares[index].x + 10, squares[index].y + 40, "P = " + String((int)context.sensorData.currentPower) + " w", textColor, currentColor);
  drawTextIfChanged(squares[index].x + 10, squares[index].y + 55, "E = " + String((int)context.sensorData.energyConsumption) + " w/h", textColor, currentColor);
  
  tft.setTextSize(2);
  tft.setTextColor(wifiColor, currentColor);
  tft.setTextDatum(MC_DATUM);
  drawCenteredText("W", squares[index].x + 140, squares[index].y + 20, 2, wifiColor, currentColor);  // Centered za večji izgled
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(textColor, currentColor);
  drawTextIfChanged(squares[index].x + 125, squares[index].y + 30, ssidDisplay, textColor, currentColor);
  tft.setTextColor(webColor, currentColor);
  drawTextIfChanged(squares[index].x + 125, squares[index].y + 55, "Web", webColor, currentColor);

  // Nova vrstica za napake: BME SHT LFS SD GEN
  String errorStr = "";
  uint16_t bmeColor = (context.sensorData.errorFlags[0] == 0) ? TFT_WIFION : TFT_WIFIOFF;
  tft.setTextColor(bmeColor, currentColor);
  drawTextIfChanged(squares[index].x + 10, squares[index].y + 70, "BME", bmeColor, currentColor);
  
  uint16_t shtColor = (context.sensorData.errorFlags[1] == 0) ? TFT_WIFION : TFT_WIFIOFF;
  tft.setTextColor(shtColor, currentColor);
  drawTextIfChanged(squares[index].x + 10 + 30, squares[index].y + 70, "SHT", shtColor, currentColor);
  
  uint16_t lfsColor = (context.sensorData.errorFlags[2] == 0) ? TFT_WIFION : TFT_WIFIOFF;
  tft.setTextColor(lfsColor, currentColor);
  drawTextIfChanged(squares[index].x + 10 + 60, squares[index].y + 70, "LFS", lfsColor, currentColor);
  
  uint16_t sdColor = (context.sensorData.errorFlags[3] == 0) ? TFT_WIFION : TFT_WIFIOFF;
  tft.setTextColor(sdColor, currentColor);
  drawTextIfChanged(squares[index].x + 10 + 90, squares[index].y + 70, "SD", sdColor, currentColor);
  
  uint16_t genColor = (context.sensorData.errorFlags[4] == 0) ? TFT_WIFION : TFT_WIFIOFF;
  tft.setTextColor(genColor, currentColor);
  drawTextIfChanged(squares[index].x + 10 + 115, squares[index].y + 70, "GEN", genColor, currentColor);

  // Odstranjeno border za konsolidacijo
}

void GraphRenderer::drawGraph(GraphType type, DisplayContext& context) {
  drawGraphTitle(type);  // Premaknjeno nad legendo
  drawGraphLegend(type);
  float minVal, maxVal, step;
  uint8_t* dataPtr;
  int numCurves;

  if (type == GraphType::CO2LIGHT) {
    minVal = GRAPH_CO2_MIN;
    maxVal = GRAPH_CO2_MAX;
    step = GRAPH_CO2_STEP;
    dataPtr = co2LightGraph[0];
    numCurves = 2;
    drawYAxis(GRAPH_LIGHT_MIN, GRAPH_LIGHT_MAX, GRAPH_LIGHT_STEP, false);
  } else {
    switch (type) {
      case GraphType::TEMP:
        minVal = GRAPH_TEMP_MIN;
        maxVal = GRAPH_TEMP_MAX;
        step = GRAPH_TEMP_STEP;
        dataPtr = temperatureGraph[0];
        break;
      case GraphType::HUM:
        minVal = GRAPH_HUM_MIN;
        maxVal = GRAPH_HUM_MAX;
        step = GRAPH_HUM_STEP;
        dataPtr = humidityGraph[0];
        break;
      case GraphType::FAN:
        minVal = GRAPH_FAN_MIN;
        maxVal = GRAPH_FAN_MAX;
        step = GRAPH_FAN_STEP;
        dataPtr = fanGraph[0];
        break;
    }
    numCurves = 4;
  }

  drawYAxis(minVal, maxVal, step, true);

  float scaleX = 280.0 / GRAPH_POINTS;
  float scaleY = 177.0 / (maxVal - minVal);
  uint16_t colors[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW};

  for (int curve = 0; curve < numCurves; curve++) {
    drawGraphCurve(&dataPtr[curve * GRAPH_POINTS], colors[curve], 40, 218, scaleX, scaleY);
  }
}

void GraphRenderer::drawYAxis(float minVal, float maxVal, float step, bool left) {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextSize(1);
  int x = left ? 30 : 290;
  for (float val = minVal; val <= maxVal; val += step) {
    int y = 218 - ((val - minVal) / (maxVal - minVal)) * 177;
    tft.drawLine(x - 5, y, x + 5, y, TFT_WHITE);
    String label = String(val, 0);
    tft.drawString(label, left ? x - 25 : x + 10, y - 5);
  }
}

void GraphRenderer::drawGraphCurve(uint8_t* dataPtr, uint16_t color, int offsetX, int offsetY, float scaleX, float scaleY) {
  for (int i = 0; i < GRAPH_POINTS - 1; i++) {
    int x1 = offsetX + i * scaleX;
    int x2 = offsetX + (i + 1) * scaleX;
    int y1 = offsetY - dataPtr[i];
    int y2 = offsetY - dataPtr[i + 1];
    tft.drawLine(x1, y1, x2, y2, color);
  }
}

void GraphRenderer::drawGraphLegend(GraphType type) {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextSize(1);
  String labels[4];
  uint16_t colors[4];
  int numLabels;

  if (type == GraphType::CO2LIGHT) {
    labels[0] = "CO2";
    labels[1] = "Light";
    colors[0] = TFT_RED;
    colors[1] = TFT_YELLOW;
    numLabels = 2;
  } else {
    labels[0] = "EXT";
    labels[1] = "DS";
    labels[2] = "KOP";
    labels[3] = "UT";
    colors[0] = TFT_RED;
    colors[1] = TFT_GREEN;
    colors[2] = TFT_BLUE;
    colors[3] = TFT_YELLOW;
    numLabels = 4;
  }

  for (int i = 0; i < numLabels; i++) {
    tft.fillRect(50 + i * 60, 25, 10, 10, colors[i]);  // Premaknjeno navzdol za naslov
    tft.drawString(labels[i], 65 + i * 60, 25);
  }
}

void GraphRenderer::drawGraphTitle(GraphType type) {
  String title;
  switch (type) {
    case GraphType::TEMP: title = "Temperatura"; break;
    case GraphType::HUM: title = "Vlaga"; break;
    case GraphType::CO2LIGHT: title = "CO2/Svetlost"; break;
    case GraphType::FAN: title = "Ventilatorji"; break;
  }
  drawCenteredText(title, 160, 10, 2, TFT_WHITE, TFT_BLACK);  // Premaknjeno nad legendo
}
// display.cpp