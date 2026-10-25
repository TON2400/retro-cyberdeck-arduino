#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <new>

// ============================================================================
// 1. Authoritative Hardware Configuration (Uno R3)
// ============================================================================

static const uint8_t KEYPAD_ROWS = 4;
static const uint8_t KEYPAD_COLS = 4;

// Planned mapping: Rows 1-4 -> D9-D6, Cols 1-4 -> D5-D2
static byte ROW_PINS[KEYPAD_ROWS] = {9, 8, 7, 6};
static byte COL_PINS[KEYPAD_COLS] = {5, 4, 3, 2};

// Candidate 4x4 layout (verified/logged via Serial diagnostic on each key press)
static char KEYMAP[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

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
  uint8_t selectedMenuItem; // 1 = Clock, 2 = Temp/Hum, 3 = IR Inspector
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

// ============================================================================
// 3. Display Handling (16x2 I2C LCD with timeout safety & dirty-check rendering)
// ============================================================================

static uint8_t lcdStorage[sizeof(LiquidCrystal_I2C)] __attribute__((aligned(alignof(LiquidCrystal_I2C))));
static LiquidCrystal_I2C* lcd = nullptr;

static char renderedLine0[LCD_COLS + 1] = "";
static char renderedLine1[LCD_COLS + 1] = "";

static uint8_t scanI2CBus() {
  Serial.println(F("=== I2C Bus Scan (SDA=A4, SCL=A5) ==="));
  uint8_t foundAddr = 0;
  uint8_t count = 0;

  // Check if SDA (A4) or SCL (A5) is stuck low before scanning
  pinMode(A4, INPUT_PULLUP);
  pinMode(A5, INPUT_PULLUP);
  delayMicroseconds(50);
  if (digitalRead(A4) == LOW || digitalRead(A5) == LOW) {
    Serial.print(F("[I2C] Bus line held LOW (SDA="));
    Serial.print(digitalRead(A4));
    Serial.print(F(", SCL="));
    Serial.print(digitalRead(A5));
    Serial.println(F("). Skipping I2C scan."));
    return 0;
  }

  Wire.begin();
  Wire.setWireTimeout(3000, true);

  for (uint8_t addr = 0x08; addr <= 0x77; ++addr) {
    Wire.clearWireTimeoutFlag();
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (Wire.getWireTimeoutFlag()) {
      Serial.println(F("[I2C] Timeout on bus; aborting scan."));
      return 0;
    }
    if (err == 0) {
      Serial.print(F("[I2C] Device found at 0x"));
      if (addr < 0x10) {
        Serial.print('0');
      }
      Serial.println(addr, HEX);
      if (addr == 0x27 || addr == 0x3F || foundAddr == 0) {
        foundAddr = addr;
      }
      ++count;
    }
  }

  if (count == 0) {
    Serial.println(F("[I2C] No I2C devices detected on A4/A5."));
  }
  return foundAddr;
}

static void initDisplay() {
  uint8_t detected = scanI2CBus();
  if (detected != 0) {
    sysState.lcdAddress = detected;
    sysState.lcdDetected = true;
    lcd = new (lcdStorage) LiquidCrystal_I2C(sysState.lcdAddress, LCD_COLS, LCD_ROWS);
    lcd->init();
    lcd->backlight();
    Serial.print(F("[LCD] Initialized 16x2 I2C LCD at 0x"));
    Serial.println(sysState.lcdAddress, HEX);
  } else {
    sysState.lcdAddress = DEFAULT_LCD_ADDR;
    sysState.lcdDetected = false;
    lcd = nullptr;
    Serial.println(F("[LCD] Running in Serial mirror mode until I2C LCD is detected (send 's' to rescan)."));
  }

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

  bool changed = false;

  if (strncmp(renderedLine0, padded0, LCD_COLS) != 0) {
    memcpy(renderedLine0, padded0, LCD_COLS + 1);
    changed = true;
    if (sysState.lcdDetected && lcd != nullptr) {
      lcd->setCursor(0, 0);
      lcd->print(renderedLine0);
    }
  }

  if (strncmp(renderedLine1, padded1, LCD_COLS) != 0) {
    memcpy(renderedLine1, padded1, LCD_COLS + 1);
    changed = true;
    if (sysState.lcdDetected && lcd != nullptr) {
      lcd->setCursor(0, 1);
      lcd->print(renderedLine1);
    }
  }

  if (changed) {
    Serial.print(F("[LCD] L0: \""));
    Serial.print(renderedLine0);
    Serial.print(F("\" | L1: \""));
    Serial.print(renderedLine1);
    Serial.println(F("\""));
  }
}

// ============================================================================
// 4. InputManager (Keypad scanning + diagnostic logging + global shortcut)
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

// Returns true if the key completed the global calculator shortcut (A, B, C, D, #)
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

// ============================================================================
// 5. HomeApp and AppManager
// ============================================================================

static const char* getMenuTitle(uint8_t item) {
  switch (item) {
    case 1: return ">1:Clock       #";
    case 2: return ">2:Temp/Humid  #";
    case 3: return ">3:IR Inspect  #";
    default: return ">1:Clock       #";
  }
}

static void handleHomeInput(char key) {
  if (key >= '1' && key <= '3') {
    uint8_t newItem = static_cast<uint8_t>(key - '0');
    if (sysState.selectedMenuItem != newItem) {
      sysState.selectedMenuItem = newItem;
      sysState.displayDirty = true;
    }
    Serial.print(F("[HOME] Selected item "));
    Serial.println(sysState.selectedMenuItem);
  } else if (key == '#') {
    if (sysState.selectedMenuItem == 1) {
      sysState.activeApp = APP_CLOCK;
    } else if (sysState.selectedMenuItem == 2) {
      sysState.activeApp = APP_SENSOR;
    } else if (sysState.selectedMenuItem == 3) {
      sysState.activeApp = APP_IR_INSPECTOR;
    }
    sysState.displayDirty = true;
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
  if (sysState.activeApp == APP_HOME) {
    handleHomeInput(key);
  } else {
    if (key == '*') {
      sysState.activeApp = APP_HOME;
      sysState.displayDirty = true;
      Serial.println(F("[APP] Returned to Home Menu"));
    }
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

  // Check global command sequences (A, B, C, D, #) before forwarding to active app
  if (checkGlobalShortcut(key)) {
    if (sysState.activeApp == APP_HOME) {
      sysState.activeApp = APP_CALCULATOR;
      sysState.displayDirty = true;
      Serial.println(F("[SHORTCUT] Calculator opened via ABCD#"));
    } else if (sysState.activeApp == APP_CALCULATOR) {
      sysState.activeApp = APP_HOME;
      sysState.displayDirty = true;
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
      renderDisplayLines("1:CLOCK APP", "*:Back to Home");
      break;
    case APP_SENSOR:
      renderDisplayLines("2:TEMP & HUMID", "*:Back to Home");
      break;
    case APP_IR_INSPECTOR:
      renderDisplayLines("3:IR INSPECTOR", "*:Back to Home");
      break;
    case APP_CALCULATOR:
      renderDisplayLines("CALCULATOR", "ABCD# or *:Exit");
      break;
  }
}

// ============================================================================
// 6. Arduino Setup & Cooperative Main Loop
// ============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println(F("=== Retro Cyberdeck: Milestone 2 Core Terminal ==="));
  Serial.println(F("Keypad Pins: R1-R4 = D9-D6, C1-C4 = D5-D2"));

  initDisplay();
  renderActiveApp();
}

void loop() {
  // 1. Read available physical keypad input
  char key = keypad.getKey();
  if (key != NO_KEY) {
    processKeyInput(key, false);
  }

  // Serial diagnostic interface ('s' to rescan I2C, or keypad characters for testing)
  if (Serial.available() > 0) {
    char cmd = static_cast<char>(Serial.read());
    if (cmd == 's' || cmd == 'S') {
      initDisplay();
    } else if ((cmd >= '0' && cmd <= '9') || (cmd >= 'A' && cmd <= 'D') || cmd == '*' || cmd == '#') {
      processKeyInput(cmd, true);
    }
  }

  // Render the LCD only when displayed content changes
  renderActiveApp();
}
