#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <DHT.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <new>

// ============================================================================
// 1. Authoritative Pin & Hardware Configuration (Uno R3)
// ============================================================================

static const uint8_t KEYPAD_ROWS = 4;
static const uint8_t KEYPAD_COLS = 4;

// Keypad mapping: Rows 1-4 -> D9-D6, Cols 1-4 -> D5-D2
static byte ROW_PINS[KEYPAD_ROWS] = {9, 8, 7, 6};
static byte COL_PINS[KEYPAD_COLS] = {5, 4, 3, 2};

static char KEYMAP[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Peripherals per ARCHITECTURE.md
static const uint8_t PIN_IR_RECV   = 10; // D10 (PB2 / PCINT2)
static const uint8_t PIN_DHT11     = 11; // D11
static const uint8_t PIN_RTC_CLK   = A0; // DS1302 CLK
static const uint8_t PIN_RTC_DAT   = A1; // DS1302 DAT/IO
static const uint8_t PIN_RTC_RST   = A2; // DS1302 RST/CE

static const uint8_t LCD_COLS = 16;
static const uint8_t LCD_ROWS = 2;
static const uint8_t DEFAULT_LCD_ADDR = 0x27;

// ============================================================================
// 2. SystemState
// ============================================================================

enum AppId : uint8_t {
  APP_HOME = 0,
  APP_CLOCK = 1,
  APP_SENSOR = 2,
  APP_IR_INSPECTOR = 3,
  APP_CALCULATOR = 4
};

struct SystemState {
  AppId activeApp;
  uint8_t selectedMenuItem; // 1=Clock, 2=Temp/Hum, 3=IR Inspector, 4=Calculator
  uint8_t lcdAddress;
  bool lcdDetected;
  bool displayDirty;
  char lastKey;
  uint8_t lastRow;
  uint8_t lastCol;
};

static SystemState sysState = {
  APP_HOME,
  1,
  DEFAULT_LCD_ADDR,
  false,
  true,
  '\0',
  0,
  0
};

// Forward declarations for mode transitions
static void setIRCaptureEnabled(bool enabled);
static void pollDHT11(bool forceNow);
static void calcReset();

static void switchApp(AppId nextApp) {
  if (sysState.activeApp == APP_IR_INSPECTOR && nextApp != APP_IR_INSPECTOR) {
    setIRCaptureEnabled(false);
  }
  sysState.activeApp = nextApp;
  if (nextApp == APP_IR_INSPECTOR) {
    setIRCaptureEnabled(true);
  } else if (nextApp == APP_SENSOR) {
    pollDHT11(true);
  }
  sysState.displayDirty = true;
}

// ============================================================================
// 3. Display Handling (16x2 I2C LCD with timeout safety & dirty-check rendering)
// ============================================================================

static uint8_t lcdStorage[sizeof(LiquidCrystal_I2C)] __attribute__((aligned(alignof(LiquidCrystal_I2C))));
static LiquidCrystal_I2C* lcd = nullptr;

static char renderedLine0[LCD_COLS + 1] = "";
static char renderedLine1[LCD_COLS + 1] = "";

static bool probeI2CAddress(uint8_t addr) {
  Wire.clearWireTimeoutFlag();
  Wire.beginTransmission(addr);
  uint8_t err = Wire.endTransmission();
  return (err == 0 && !Wire.getWireTimeoutFlag());
}

static uint8_t scanI2CBus() {
  Wire.begin();
  Wire.setWireTimeout(3000, true);

  if (probeI2CAddress(0x27)) {
    Serial.println(F("[I2C] LCD found at 0x27"));
    return 0x27;
  }
  if (probeI2CAddress(0x3F)) {
    Serial.println(F("[I2C] LCD found at 0x3F"));
    return 0x3F;
  }
  for (uint8_t addr = 0x20; addr <= 0x3E; ++addr) {
    if (probeI2CAddress(addr)) {
      Serial.print(F("[I2C] Device found at 0x"));
      Serial.println(addr, HEX);
      return addr;
    }
    if (Wire.getWireTimeoutFlag()) {
      break;
    }
  }
  return 0;
}

static void initDisplay() {
  uint8_t detected = scanI2CBus();
  if (detected != 0) {
    sysState.lcdAddress = detected;
    sysState.lcdDetected = true;
  } else {
    sysState.lcdAddress = DEFAULT_LCD_ADDR;
    sysState.lcdDetected = false;
  }

  lcd = new (lcdStorage) LiquidCrystal_I2C(sysState.lcdAddress, LCD_COLS, LCD_ROWS);
  Wire.setWireTimeout(3000, true);
  lcd->init();
  lcd->backlight();

  renderedLine0[0] = '\0';
  renderedLine1[0] = '\0';
  sysState.displayDirty = true;
}

static void formatPaddedLine(char* dest, const char* src) {
  uint8_t i = 0;
  while (i < LCD_COLS && src[i] != '\0') {
    dest[i] = src[i];
    ++i;
  }
  while (i < LCD_COLS) {
    dest[i] = ' ';
    ++i;
  }
  dest[LCD_COLS] = '\0';
}

static void renderDisplayLines(const char* line0, const char* line1) {
  char padded0[LCD_COLS + 1];
  char padded1[LCD_COLS + 1];
  formatPaddedLine(padded0, line0);
  formatPaddedLine(padded1, line1);

  bool changed0 = (strncmp(renderedLine0, padded0, LCD_COLS) != 0);
  bool changed1 = (strncmp(renderedLine1, padded1, LCD_COLS) != 0);

  if (!changed0 && !changed1) {
    return;
  }

  // If LCD was not detected at boot, do a single fast probe to see if it is now connected
  if (!sysState.lcdDetected && lcd != nullptr) {
    if (probeI2CAddress(sysState.lcdAddress)) {
      sysState.lcdDetected = true;
      lcd->init();
      lcd->backlight();
    }
  }

  if (changed0) {
    memcpy(renderedLine0, padded0, LCD_COLS + 1);
    if (sysState.lcdDetected && lcd != nullptr) {
      lcd->setCursor(0, 0);
      lcd->print(renderedLine0);
    }
  }

  if (changed1) {
    memcpy(renderedLine1, padded1, LCD_COLS + 1);
    if (sysState.lcdDetected && lcd != nullptr) {
      lcd->setCursor(0, 1);
      lcd->print(renderedLine1);
    }
  }

  Serial.print(F("[LCD] L0: \""));
  Serial.print(renderedLine0);
  Serial.print(F("\" | L1: \""));
  Serial.print(renderedLine1);
  Serial.println(F("\""));
}

// ============================================================================
// 4. ClockService & ClockApp (DS1302 on CLK=A0, DAT=A1, RST=A2 + SW Clock)
// ============================================================================

static ThreeWire rtcWire(PIN_RTC_DAT, PIN_RTC_CLK, PIN_RTC_RST);
static RtcDS1302<ThreeWire> rtc(rtcWire);

struct ClockState {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  bool hwValid;
  unsigned long lastTickMs;
  bool editMode;
  char editBuf[7]; // HHMMSS
  uint8_t editLen;
};

static ClockState clockState = {
  2026, 10, 25, 18, 0, 0,
  false, 0, false, "", 0
};

static bool isRtcTimePlausible(const RtcDateTime& dt) {
  return dt.IsValid() &&
         dt.Year() >= 2020 && dt.Year() <= 2099 &&
         dt.Month() >= 1 && dt.Month() <= 12 &&
         dt.Day() >= 1 && dt.Day() <= 31 &&
         dt.Hour() <= 23 && dt.Minute() <= 59 && dt.Second() <= 59;
}

static void advanceSoftwareClockOneSecond() {
  ++clockState.second;
  if (clockState.second >= 60) {
    clockState.second = 0;
    ++clockState.minute;
    if (clockState.minute >= 60) {
      clockState.minute = 0;
      ++clockState.hour;
      if (clockState.hour >= 24) {
        clockState.hour = 0;
      }
    }
  }
}

static void initClockService() {
  rtc.Begin();
  RtcDateTime compiled(__DATE__, __TIME__);

  if (rtc.GetIsWriteProtected()) {
    rtc.SetIsWriteProtected(false);
  }
  if (!rtc.GetIsRunning()) {
    rtc.SetIsRunning(true);
  }

  RtcDateTime now = rtc.GetDateTime();
  if (isRtcTimePlausible(now)) {
    clockState.year = now.Year();
    clockState.month = now.Month();
    clockState.day = now.Day();
    clockState.hour = now.Hour();
    clockState.minute = now.Minute();
    clockState.second = now.Second();
    clockState.hwValid = true;
  } else {
    clockState.year = compiled.Year();
    clockState.month = compiled.Month();
    clockState.day = compiled.Day();
    clockState.hour = compiled.Hour();
    clockState.minute = compiled.Minute();
    clockState.second = compiled.Second();
    rtc.SetDateTime(compiled);
    RtcDateTime verify = rtc.GetDateTime();
    clockState.hwValid = isRtcTimePlausible(verify);
  }
  clockState.lastTickMs = millis();
}

static void updateClockService() {
  unsigned long nowMs = millis();
  if (nowMs - clockState.lastTickMs < 1000UL) {
    return;
  }

  uint8_t prevSec = clockState.second;
  while (nowMs - clockState.lastTickMs >= 1000UL) {
    clockState.lastTickMs += 1000UL;
    advanceSoftwareClockOneSecond();
  }

  // Sync with DS1302 once per minute or when in ClockApp
  if (sysState.activeApp == APP_CLOCK && clockState.second == 0) {
    RtcDateTime hwNow = rtc.GetDateTime();
    if (isRtcTimePlausible(hwNow)) {
      clockState.year = hwNow.Year();
      clockState.month = hwNow.Month();
      clockState.day = hwNow.Day();
      clockState.hour = hwNow.Hour();
      clockState.minute = hwNow.Minute();
      clockState.second = hwNow.Second();
      clockState.hwValid = true;
    }
  }

  if (sysState.activeApp == APP_CLOCK && !clockState.editMode && clockState.second != prevSec) {
    sysState.displayDirty = true;
  }
}

static void handleClockInput(char key) {
  if (!clockState.editMode) {
    if (key == '*') {
      switchApp(APP_HOME);
      Serial.println(F("[CLOCK] Returned to Home Menu"));
    } else if (key == '#') {
      clockState.editMode = true;
      clockState.editLen = 0;
      clockState.editBuf[0] = '\0';
      sysState.displayDirty = true;
      Serial.println(F("[CLOCK] Entered HHMMSS time set mode"));
    }
    return;
  }

  if (key == '*') {
    clockState.editMode = false;
    clockState.editLen = 0;
    sysState.displayDirty = true;
    Serial.println(F("[CLOCK] Cancelled time edit"));
  } else if (key >= '0' && key <= '9') {
    if (clockState.editLen < 6) {
      clockState.editBuf[clockState.editLen++] = key;
      clockState.editBuf[clockState.editLen] = '\0';
      sysState.displayDirty = true;
    }
  } else if (key == '#') {
    if (clockState.editLen == 6) {
      uint8_t hh = static_cast<uint8_t>((clockState.editBuf[0] - '0') * 10 + (clockState.editBuf[1] - '0'));
      uint8_t mm = static_cast<uint8_t>((clockState.editBuf[2] - '0') * 10 + (clockState.editBuf[3] - '0'));
      uint8_t ss = static_cast<uint8_t>((clockState.editBuf[4] - '0') * 10 + (clockState.editBuf[5] - '0'));
      if (hh <= 23 && mm <= 59 && ss <= 59) {
        clockState.hour = hh;
        clockState.minute = mm;
        clockState.second = ss;
        clockState.lastTickMs = millis();
        RtcDateTime newDt(clockState.year, clockState.month, clockState.day, hh, mm, ss);
        rtc.SetIsWriteProtected(false);
        rtc.SetIsRunning(true);
        rtc.SetDateTime(newDt);
        Serial.println(F("[CLOCK] Time updated"));
      }
      clockState.editMode = false;
      clockState.editLen = 0;
      sysState.displayDirty = true;
    }
  }
}

static void renderClockApp() {
  char line0[LCD_COLS + 1];
  char line1[LCD_COLS + 1];

  if (clockState.editMode) {
    char d[6];
    for (uint8_t i = 0; i < 6; ++i) {
      d[i] = (i < clockState.editLen) ? clockState.editBuf[i] : '_';
    }
    snprintf(line0, sizeof(line0), "SET: %c%c:%c%c:%c%c", d[0], d[1], d[2], d[3], d[4], d[5]);
    snprintf(line1, sizeof(line1), "#:Save  *:Cancel");
  } else {
    snprintf(line0, sizeof(line0), "TIME: %02u:%02u:%02u",
             clockState.hour, clockState.minute, clockState.second);
    snprintf(line1, sizeof(line1), "%04u-%02u-%02u #Set*",
             clockState.year, clockState.month, clockState.day);
  }
  renderDisplayLines(line0, line1);
}

// ============================================================================
// 5. SensorService & SensorApp (DHT11 on D11)
// ============================================================================

static DHT dht(PIN_DHT11, DHT11);

struct SensorState {
  float tempC;
  float tempF;
  float humidity;
  float heatIndexC;
  bool valid;
  bool showHeatIndex;
  unsigned long lastReadMs;
};

static SensorState sensorState = {
  0.0f, 0.0f, 0.0f, 0.0f, false, false, 0
};

static void pollDHT11(bool forceNow) {
  unsigned long nowMs = millis();
  if (!forceNow && (nowMs - sensorState.lastReadMs < 2500UL)) {
    return;
  }
  sensorState.lastReadMs = nowMs;

  // Flush UART TX buffer before DHT disables interrupts during 1-wire read
  Serial.flush();

  float h = dht.readHumidity(forceNow);
  float t = dht.readTemperature(false, forceNow);
  float f = dht.readTemperature(true, false);

  if (!isnan(h) && !isnan(t) && !isnan(f)) {
    sensorState.humidity = h;
    sensorState.tempC = t;
    sensorState.tempF = f;
    sensorState.heatIndexC = dht.computeHeatIndex(t, h, false);
    sensorState.valid = true;
  } else {
    sensorState.valid = false;
  }

  if (sysState.activeApp == APP_SENSOR) {
    sysState.displayDirty = true;
  }
}

static void initSensorService() {
  dht.begin();
  sensorState.lastReadMs = 0;
}

static void handleSensorInput(char key) {
  if (key == '*') {
    switchApp(APP_HOME);
    Serial.println(F("[SENSOR] Returned to Home Menu"));
  } else if (key == '#') {
    sensorState.showHeatIndex = !sensorState.showHeatIndex;
    pollDHT11(true);
    sysState.displayDirty = true;
    Serial.println(F("[SENSOR] Refreshed DHT11 reading"));
  }
}

static void renderSensorApp() {
  char line0[LCD_COLS + 1];
  char line1[LCD_COLS + 1];

  if (!sensorState.valid) {
    snprintf(line0, sizeof(line0), "DHT11 Pin D11");
    snprintf(line1, sizeof(line1), "No Data #:Try *B");
  } else {
    char cBuf[8];
    char fBuf[8];
    dtostrf(sensorState.tempC, 4, 1, cBuf);
    dtostrf(sensorState.tempF, 3, 0, fBuf);
    snprintf(line0, sizeof(line0), "T:%sC / %sF", cBuf, fBuf);

    if (sensorState.showHeatIndex) {
      char hiBuf[8];
      dtostrf(sensorState.heatIndexC, 4, 1, hiBuf);
      snprintf(line1, sizeof(line1), "HI:%sC  #/*Bk", hiBuf);
    } else {
      char hBuf[8];
      dtostrf(sensorState.humidity, 4, 1, hBuf);
      snprintf(line1, sizeof(line1), "RH:%s%%  #/*Bk", hBuf);
    }
  }
  renderDisplayLines(line0, line1);
}

// ============================================================================
// 6. IRService & IRInspectorApp (IR Receiver on D10 using PCINT2 / Port B)
// ============================================================================

static const uint8_t IR_MAX_EDGES = 68;
static volatile uint16_t irDurations[IR_MAX_EDGES];
static volatile uint8_t irEdgeCount = 0;
static volatile unsigned long irLastEdgeUs = 0;

struct IRState {
  uint32_t lastCode;
  uint8_t lastBits;
  uint8_t lastEdges;
  bool isNec;
  bool hasSignal;
  uint16_t rxCount;
};

static IRState irState = {0, 0, 0, false, false, 0};

ISR(PCINT0_vect) {
  unsigned long nowUs = micros();
  unsigned long diff = nowUs - irLastEdgeUs;
  irLastEdgeUs = nowUs;

  if (diff > 15000UL) {
    irEdgeCount = 0;
    return;
  }

  uint8_t idx = irEdgeCount;
  if (idx < IR_MAX_EDGES) {
    irDurations[idx] = (diff > 65535UL) ? 65535U : static_cast<uint16_t>(diff);
    irEdgeCount = idx + 1;
  }
}

static void setIRCaptureEnabled(bool enabled) {
  noInterrupts();
  irEdgeCount = 0;
  irLastEdgeUs = micros();
  if (enabled) {
    PCICR |= _BV(PCIE0);
    PCMSK0 = _BV(PCINT2);
  } else {
    PCMSK0 &= ~_BV(PCINT2);
  }
  interrupts();
}

static void initIRService() {
  pinMode(PIN_IR_RECV, INPUT_PULLUP);
  setIRCaptureEnabled(false);
}

static void pollIRService() {
  if (sysState.activeApp != APP_IR_INSPECTOR) {
    return;
  }

  uint8_t edges = 0;
  unsigned long lastUs = 0;

  noInterrupts();
  edges = irEdgeCount;
  lastUs = irLastEdgeUs;
  interrupts();

  if (edges < 4 || (micros() - lastUs) < 12000UL) {
    return;
  }

  PCMSK0 &= ~_BV(PCINT2);
  edges = irEdgeCount;

  bool necDecoded = false;
  uint32_t code = 0;
  uint8_t bits = 0;

  if (edges >= 66 &&
      irDurations[0] >= 7000 && irDurations[0] <= 11000 &&
      irDurations[1] >= 3000 && irDurations[1] <= 6000) {
    for (uint8_t b = 0; b < 32; ++b) {
      uint16_t mark = irDurations[2 + b * 2];
      uint16_t space = irDurations[3 + b * 2];
      if (mark < 250 || mark > 1000) {
        break;
      }
      code <<= 1;
      if (space >= 1100) {
        code |= 1UL;
      }
      ++bits;
    }
    if (bits == 32) {
      necDecoded = true;
    }
  }

  if (!necDecoded && edges <= 6 &&
      irDurations[0] >= 7000 && irDurations[0] <= 11000 &&
      irDurations[1] >= 1500 && irDurations[1] <= 3000 &&
      irState.hasSignal) {
    irEdgeCount = 0;
    PCMSK0 |= _BV(PCINT2);
    ++irState.rxCount;
    sysState.displayDirty = true;
    return;
  }

  if (!necDecoded) {
    code = 2166136261UL;
    bits = edges / 2;
    for (uint8_t i = 0; i < edges; ++i) {
      uint8_t quant = static_cast<uint8_t>((irDurations[i] + 128U) >> 8);
      code ^= quant;
      code *= 16777619UL;
    }
  }

  irEdgeCount = 0;
  PCMSK0 |= _BV(PCINT2);

  irState.lastCode = code;
  irState.lastBits = bits;
  irState.lastEdges = edges;
  irState.isNec = necDecoded;
  irState.hasSignal = true;
  ++irState.rxCount;

  Serial.print(F("[IR] Code=0x"));
  Serial.print(irState.lastCode, HEX);
  Serial.print(irState.isNec ? F(" Proto=NEC Bits=") : F(" Proto=RAW Bits="));
  Serial.println(irState.lastBits);

  sysState.displayDirty = true;
}

static void handleIRInspectorInput(char key) {
  if (key == '*') {
    switchApp(APP_HOME);
    Serial.println(F("[IR] Returned to Home Menu"));
  } else if (key == '#') {
    irState.hasSignal = false;
    irState.lastCode = 0;
    irState.lastBits = 0;
    irState.lastEdges = 0;
    sysState.displayDirty = true;
    Serial.println(F("[IR] Cleared captured signal"));
  }
}

static void renderIRInspectorApp() {
  char line0[LCD_COLS + 1];
  char line1[LCD_COLS + 1];

  if (!irState.hasSignal) {
    snprintf(line0, sizeof(line0), "IR D10: Waiting");
    snprintf(line1, sizeof(line1), "Aim Remote  *:Bk");
  } else {
    snprintf(line0, sizeof(line0), "IR:0x%08lX", static_cast<unsigned long>(irState.lastCode));
    snprintf(line1, sizeof(line1), "%s %ub #:Clr*Bk",
             irState.isNec ? "NEC" : "RAW",
             irState.lastBits);
  }
  renderDisplayLines(line0, line1);
}

// ============================================================================
// 7. CalculatorApp (Two operands, one operator, decimals, division-by-0 safety)
// ============================================================================

static const uint8_t CALC_MAX_EXPR = 16;

struct CalculatorState {
  char expr[CALC_MAX_EXPR + 1];
  uint8_t len;
  char statusLine[LCD_COLS + 1];
  bool evaluated;
};

static CalculatorState calcState;

static void calcReset() {
  calcState.expr[0] = '\0';
  calcState.len = 0;
  strncpy(calcState.statusLine, "A+ B- C* D/ #=", LCD_COLS);
  calcState.statusLine[LCD_COLS] = '\0';
  calcState.evaluated = false;
  sysState.displayDirty = true;
}

static bool isOperatorChar(char c) {
  return c == '+' || c == '-' || c == '*' || c == '/';
}

static char mapKeyToOperator(char key) {
  switch (key) {
    case 'A': return '+';
    case 'B': return '-';
    case 'C': return '*';
    case 'D': return '/';
    default:  return '\0';
  }
}

static bool parseSingleOperand(const char* start, uint8_t length, double* outVal) {
  if (length == 0 || length > CALC_MAX_EXPR) {
    return false;
  }

  char buf[CALC_MAX_EXPR + 1];
  uint8_t dotCount = 0;
  uint8_t digitCount = 0;

  for (uint8_t i = 0; i < length; ++i) {
    char c = start[i];
    if (c >= '0' && c <= '9') {
      ++digitCount;
    } else if (c == '.') {
      ++dotCount;
      if (dotCount > 1) {
        return false;
      }
    } else {
      return false;
    }
    buf[i] = c;
  }
  buf[length] = '\0';

  if (digitCount == 0) {
    return false;
  }

  *outVal = atof(buf);
  return true;
}

static void formatCalcResult(double val, char* outBuf, size_t outSize) {
  if (isnan(val) || isinf(val) || fabs(val) > 999999999.0) {
    strncpy(outBuf, "ERR: OVERFLOW", outSize - 1);
    outBuf[outSize - 1] = '\0';
    return;
  }

  char numBuf[20];
  dtostrf(val, 1, 4, numBuf);

  char* dot = strchr(numBuf, '.');
  if (dot != nullptr) {
    char* end = numBuf + strlen(numBuf) - 1;
    while (end > dot && *end == '0') {
      *end = '\0';
      --end;
    }
    if (end == dot) {
      *dot = '\0';
    }
  }

  if (strcmp(numBuf, "-0") == 0) {
    strcpy(numBuf, "0");
  }

  outBuf[0] = '=';
  strncpy(outBuf + 1, numBuf, outSize - 2);
  outBuf[outSize - 1] = '\0';
}

static void calcEvaluate() {
  int8_t opPos = -1;
  char opChar = '\0';

  for (uint8_t i = 0; i < calcState.len; ++i) {
    char c = calcState.expr[i];
    if (isOperatorChar(c)) {
      if (opPos != -1) {
        strncpy(calcState.statusLine, "ERR: INVALID", LCD_COLS);
        calcState.statusLine[LCD_COLS] = '\0';
        calcState.evaluated = true;
        sysState.displayDirty = true;
        Serial.println(F("[CALC] Error: multiple operators"));
        return;
      }
      opPos = static_cast<int8_t>(i);
      opChar = c;
    }
  }

  if (opPos <= 0 || opPos >= static_cast<int8_t>(calcState.len) - 1) {
    strncpy(calcState.statusLine, "ERR: INVALID", LCD_COLS);
    calcState.statusLine[LCD_COLS] = '\0';
    calcState.evaluated = true;
    sysState.displayDirty = true;
    Serial.println(F("[CALC] Error: malformed expression"));
    return;
  }

  double lhs = 0.0;
  double rhs = 0.0;
  uint8_t lhsLen = static_cast<uint8_t>(opPos);
  uint8_t rhsLen = calcState.len - lhsLen - 1;

  if (!parseSingleOperand(calcState.expr, lhsLen, &lhs) ||
      !parseSingleOperand(calcState.expr + opPos + 1, rhsLen, &rhs)) {
    strncpy(calcState.statusLine, "ERR: INVALID", LCD_COLS);
    calcState.statusLine[LCD_COLS] = '\0';
    calcState.evaluated = true;
    sysState.displayDirty = true;
    Serial.println(F("[CALC] Error: invalid operand"));
    return;
  }

  if (opChar == '/' && fabs(rhs) < 1e-9) {
    strncpy(calcState.statusLine, "ERR: DIV BY 0", LCD_COLS);
    calcState.statusLine[LCD_COLS] = '\0';
    calcState.evaluated = true;
    sysState.displayDirty = true;
    Serial.println(F("[CALC] Error: division by zero"));
    return;
  }

  double result = 0.0;
  switch (opChar) {
    case '+': result = lhs + rhs; break;
    case '-': result = lhs - rhs; break;
    case '*': result = lhs * rhs; break;
    case '/': result = lhs / rhs; break;
    default:  break;
  }

  formatCalcResult(result, calcState.statusLine, sizeof(calcState.statusLine));
  calcState.evaluated = true;
  sysState.displayDirty = true;

  Serial.print(F("[CALC] Evaluated: "));
  Serial.print(calcState.expr);
  Serial.print(' ');
  Serial.println(calcState.statusLine);
}

static void handleCalculatorInput(char key) {
  if (key == '*') {
    if (calcState.len == 0 && !calcState.evaluated) {
      // Pressing '*' when already cleared returns to Home Menu
      switchApp(APP_HOME);
      Serial.println(F("[CALC] Returned to Home Menu"));
    } else {
      calcReset();
      Serial.println(F("[CALC] Input cleared"));
    }
    return;
  }

  if (key == '#') {
    calcEvaluate();
    return;
  }

  char op = mapKeyToOperator(key);
  char toAppend = (op != '\0') ? op : key;

  if ((toAppend >= '0' && toAppend <= '9') || toAppend == '.' || isOperatorChar(toAppend)) {
    if (calcState.evaluated) {
      calcState.expr[0] = '\0';
      calcState.len = 0;
      calcState.evaluated = false;
      strncpy(calcState.statusLine, "A+ B- C* D/ #=", LCD_COLS);
      calcState.statusLine[LCD_COLS] = '\0';
    }

    if (calcState.len < CALC_MAX_EXPR) {
      calcState.expr[calcState.len++] = toAppend;
      calcState.expr[calcState.len] = '\0';
      sysState.displayDirty = true;
    } else {
      strncpy(calcState.statusLine, "ERR: MAX LEN", LCD_COLS);
      calcState.statusLine[LCD_COLS] = '\0';
      sysState.displayDirty = true;
      Serial.println(F("[CALC] Warning: input buffer full"));
    }
  }
}

static void renderCalculatorApp() {
  const char* line0 = (calcState.len > 0) ? calcState.expr : "CALC: 0";
  renderDisplayLines(line0, calcState.statusLine);
}

// ============================================================================
// 8. InputManager & AppManager
// ============================================================================

static Keypad keypad(makeKeymap(KEYMAP), ROW_PINS, COL_PINS, KEYPAD_ROWS, KEYPAD_COLS);

static const char SHORTCUT_SEQ[5] = {'A', 'B', 'C', 'D', '#'};
static uint8_t shortcutIndex = 0;

static void findKeyCoords(char key, uint8_t* outRow, uint8_t* outCol) {
  for (uint8_t r = 0; r < KEYPAD_ROWS; ++r) {
    for (uint8_t c = 0; c < KEYPAD_COLS; ++c) {
      if (KEYMAP[r][c] == key) {
        *outRow = r + 1;
        *outCol = c + 1;
        return;
      }
    }
  }
  *outRow = 0;
  *outCol = 0;
}

static void logKeyPressDiagnostic(char key, uint8_t row, uint8_t col, bool fromSerial) {
  Serial.print(fromSerial ? F("[SERIAL-KEY] Key='") : F("[KEYPAD] Key='"));
  Serial.print(key);
  Serial.print(F("' Row=R"));
  Serial.print(row);
  Serial.print(F("(D"));
  Serial.print(row > 0 ? ROW_PINS[row - 1] : 0);
  Serial.print(F(") Col=C"));
  Serial.print(col);
  Serial.print(F("(D"));
  Serial.print(col > 0 ? COL_PINS[col - 1] : 0);
  Serial.println(F(")"));
}

static bool checkGlobalShortcut(char key) {
  if (key == SHORTCUT_SEQ[shortcutIndex]) {
    ++shortcutIndex;
    if (shortcutIndex == sizeof(SHORTCUT_SEQ)) {
      shortcutIndex = 0;
      return true;
    }
  } else if (key == SHORTCUT_SEQ[0]) {
    shortcutIndex = 1;
  } else {
    shortcutIndex = 0;
  }
  return false;
}

static const char* getMenuTitle(uint8_t item) {
  switch (item) {
    case 1: return ">1:Clock       #";
    case 2: return ">2:Temp/Humid  #";
    case 3: return ">3:IR Inspect  #";
    case 4: return ">4:Calculator  #";
    default: return ">1:Clock       #";
  }
}

static void handleHomeInput(char key) {
  if (key >= '1' && key <= '4') {
    uint8_t newItem = static_cast<uint8_t>(key - '0');
    if (sysState.selectedMenuItem != newItem) {
      sysState.selectedMenuItem = newItem;
      sysState.displayDirty = true;
    }
    Serial.print(F("[HOME] Selected item "));
    Serial.println(sysState.selectedMenuItem);
  } else if (key == '#') {
    if (sysState.selectedMenuItem == 1) {
      clockState.editMode = false;
      switchApp(APP_CLOCK);
    } else if (sysState.selectedMenuItem == 2) {
      switchApp(APP_SENSOR);
    } else if (sysState.selectedMenuItem == 3) {
      switchApp(APP_IR_INSPECTOR);
    } else if (sysState.selectedMenuItem == 4) {
      calcReset();
      switchApp(APP_CALCULATOR);
    }
    Serial.print(F("[HOME] Execute item "));
    Serial.println(sysState.selectedMenuItem);
  } else if (key == '*') {
    if (sysState.selectedMenuItem != 1) {
      sysState.selectedMenuItem = 1;
      sysState.displayDirty = true;
    }
    Serial.println(F("[HOME] Selection reset to 1"));
  }
}

static void dispatchAppInput(char key) {
  switch (sysState.activeApp) {
    case APP_HOME:
      handleHomeInput(key);
      break;
    case APP_CLOCK:
      handleClockInput(key);
      break;
    case APP_SENSOR:
      handleSensorInput(key);
      break;
    case APP_IR_INSPECTOR:
      handleIRInspectorInput(key);
      break;
    case APP_CALCULATOR:
      handleCalculatorInput(key);
      break;
  }
}

static void processKeyInput(char key, bool fromSerial) {
  uint8_t row = 0;
  uint8_t col = 0;
  findKeyCoords(key, &row, &col);
  sysState.lastKey = key;
  sysState.lastRow = row;
  sysState.lastCol = col;

  logKeyPressDiagnostic(key, row, col, fromSerial);

  // Global shortcut A, B, C, D, # enters or exits Calculator
  if (checkGlobalShortcut(key)) {
    if (sysState.activeApp == APP_HOME) {
      calcReset();
      switchApp(APP_CALCULATOR);
      Serial.println(F("[SHORTCUT] Calculator opened via ABCD#"));
    } else if (sysState.activeApp == APP_CALCULATOR) {
      calcReset();
      switchApp(APP_HOME);
      Serial.println(F("[SHORTCUT] Calculator exited via ABCD#"));
    }
  } else {
    dispatchAppInput(key);
  }
}

static void renderActiveApp() {
  if (!sysState.displayDirty) {
    return;
  }
  sysState.displayDirty = false;

  switch (sysState.activeApp) {
    case APP_HOME:
      renderDisplayLines("RETRO CYBERDECK", getMenuTitle(sysState.selectedMenuItem));
      break;
    case APP_CLOCK:
      renderClockApp();
      break;
    case APP_SENSOR:
      renderSensorApp();
      break;
    case APP_IR_INSPECTOR:
      renderIRInspectorApp();
      break;
    case APP_CALCULATOR:
      renderCalculatorApp();
      break;
  }
}

// ============================================================================
// 9. Setup & Cooperative Main Loop
// ============================================================================

void setup() {
  delay(250);
  Serial.begin(115200);
  Serial.println(F("=== Retro Cyberdeck Firmware (Uno R3) ==="));
  Serial.println(F("Pins: KP R1-R4=D9-D6, C1-C4=D5-D2 | IR=D10 | DHT11=D11 | RTC=A0/A1/A2 | LCD=A4/A5"));

  calcReset();
  initDisplay();
  initClockService();
  initSensorService();
  initIRService();
  renderActiveApp();
}

void loop() {
  // 1. Read available keypad input
  char key = keypad.getKey();
  if (key != NO_KEY) {
    processKeyInput(key, false);
  }

  // Serial input bridge for diagnostics and testing
  if (Serial.available() > 0) {
    char cmd = static_cast<char>(Serial.read());
    if (cmd == 's' || cmd == 'S') {
      initDisplay();
    } else if ((cmd >= '0' && cmd <= '9') || (cmd >= 'A' && cmd <= 'D') ||
               cmd == '*' || cmd == '#' || cmd == '.') {
      processKeyInput(cmd, true);
    }
  }

  // 4. Poll IR input when IR Inspector is active
  pollIRService();

  // 5. Refresh DHT11 readings when SensorApp is active
  if (sysState.activeApp == APP_SENSOR) {
    pollDHT11(false);
  }

  // 6. Update clock display every second
  updateClockService();

  // 7. Render the LCD only when displayed content changes
  renderActiveApp();
}
