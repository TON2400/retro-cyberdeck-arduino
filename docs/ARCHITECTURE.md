# Retro Cyberdeck — Architecture Specification

**Version:** 1.2  
**Status:** Initial specification  
**Target board:** Arduino Uno R3 only  
**Language:** Arduino C++  
**Development environment:** Windows + PowerShell + Arduino CLI + Antigravity CLI  
**Budget:** Existing hardware only; no new purchases required

## 1. Project Objective

Build a modular, keyboard-operated retro terminal using the existing Arduino Uno R3 and available modules.

Planned applications:

1. Home menu
2. Clock using DS1302 RTC
3. Temperature and humidity using DHT11
4. IR Inspector using an IR receiver
5. Calculator

The project must be developed incrementally. Hardware integration must not begin until the core toolchain and minimal firmware are verified.

ESP32, OLED, Wi-Fi and Bluetooth are outside the current project scope.

## 2. Hardware Inventory

Planned existing hardware:

- Arduino Uno R3
- New 4x4 matrix keypad
- 16x2 LCD with I2C backpack
- DS1302 RTC module
- DHT11 sensor module
- IR receiver module
- USB data cable

No external LEDs are required.

Do not require the user to purchase hardware for current milestones. If a required module is unavailable or incompatible, report the blocker and propose a solution using existing hardware before suggesting any purchase.

## 3. Planned Pin Mapping — Uno R3

This is the proposed mapping, not proof of actual wiring. Verify each connection during hardware integration.

| Component | Signal | Uno pin |
|---|---|---|
| Keypad | R1 | D2 |
| Keypad | R2 | D3 |
| Keypad | R3 | D4 |
| Keypad | R4 | D5 |
| Keypad | C1 | D6 |
| Keypad | C2 | D7 |
| Keypad | C3 | D8 |
| Keypad | C4 | D9 |
| IR receiver | OUT | D10 |
| DHT11 | DATA | D11 |
| DS1302 | CLK | A0 |
| DS1302 | DAT | A1 |
| DS1302 | RST/CE | A2 |
| LCD I2C | SDA | A4 |
| LCD I2C | SCL | A5 |
| Serial debugging | RX/TX | D0/D1 |

D12, D13 and A3 are reserved.

A0–A2 can be used as digital GPIO on the Uno R3. The selected DS1302 library must support explicit pin assignment.

Verify the LCD I2C address during integration. Common addresses include `0x27` and `0x3F`; do not assume either address without checking.

Confirm module voltage requirements and common-ground requirements before connecting hardware.

## 4. Keypad Requirements

The keypad has been replaced. Its exact physical key layout and row/column wiring are not yet verified.

Before integrating the keypad:

1. Confirm the connector and wiring.
2. Connect it to the planned pins after checking the module.
3. Run a diagnostic sketch.
4. Press each physical key individually.
5. Record the character reported by each key.
6. Update this document and the code to match the observed mapping.

Do not reuse the previous keypad's mapping without testing.

Keep the final mapping in one authoritative configuration so code and documentation cannot silently disagree.

## 5. Software Architecture

Use a small, modular, cooperative architecture that fits the Uno R3's limited resources.

### Core modules

- `AppManager`: manages the active application and navigation.
- `InputManager`: reads keypad input and recognizes global command sequences.
- `SystemState`: stores the current mode and shared state.
- Display handling: provides the functions needed to write text to the 16x2 I2C LCD.

### Applications

- `HomeApp`: displays the home menu.
- `ClockApp`: displays time and date from DS1302.
- `SensorApp`: displays DHT11 temperature and humidity.
- `IRInspectorApp`: displays information about received IR signals.
- `CalculatorApp`: performs basic arithmetic.

### Services

- `ClockService`: reads the RTC.
- `SensorService`: reads and caches sensor measurements.
- `IRService`: receives and exposes IR data.

Keep the design simple. Introduce interfaces or additional files only when they solve a real implementation problem. Do not build generic frameworks for hypothetical hardware.

## 6. Navigation and Input

### Home menu

Planned menu:

- `1`: Clock
- `2`: Temperature and humidity
- `3`: IR Inspector

The calculator is opened through a dedicated shortcut.

The LCD is 16x2, so use a simple selection indicator or paging as needed.

### Execute command

- Press a number to select a menu item.
- Press `#` to execute the selected item.
- Press `*` to return or clear, depending on the active application.

### Calculator shortcut

The sequence `A`, `B`, `C`, `D`, `#` enters the calculator from the home menu. The same sequence exits the calculator while it is active.

Recognize the global shortcut before forwarding keys to the active application. Use a bounded buffer or a small explicit sequence recognizer. Define how incomplete sequences are handled so normal input is not unexpectedly lost.

## 7. Calculator

The calculator supports:

- Addition
- Subtraction
- Multiplication
- Division
- Decimal numbers
- Input clearing
- Invalid-input handling
- Result display

Key mapping:

- `A`: addition
- `B`: subtraction
- `C`: multiplication
- `D`: division
- `#`: evaluate
- `*`: clear input

The first version supports two operands and one operator, for example `12+34`. A full expression parser is out of scope.

Use bounded input storage. Handle division by zero and malformed expressions without crashing or corrupting memory.

## 8. Main Loop

Use a cooperative loop and avoid long blocking delays.

Conceptual order:

1. Read available keypad input.
2. Check global command sequences.
3. Dispatch ordinary input to the active application.
4. Poll IR input when the IR service is implemented.
5. Refresh DHT11 readings at a suitable interval.
6. Update the clock display when needed.
7. Render the LCD only when the displayed content changes.

Keep keypad input responsive while periodic services run.

## 9. Uno R3 Memory and Reliability

Arduino Uno R3 resources:

- 32 KB flash
- 2 KB SRAM
- 1 KB EEPROM

Rules:

- Avoid unnecessary dynamic memory allocation.
- Prefer fixed-size buffers.
- Avoid large temporary objects.
- Avoid unnecessary global variables.
- Use `F()` for constant strings where practical.
- Avoid repeatedly allocating Arduino `String` objects.
- Review compiler memory reports after integration.
- Do not claim hardware functionality has been verified without physical testing.

## 10. Proposed Directory Structure

```text
retro-cyberdeck/
├── AGENTS.md
├── README.md
├── docs/
│   ├── ARCHITECTURE.md
│   ├── WORKFLOW.md
│   ├── PINMAP.md
│   └── USER_GUIDE.md
├── firmware/
│   └── firmware.ino
└── test/
    ├── test_calculator.md
    ├── test_navigation.md
    └── test_hardware.md
```

This is a target structure, not a requirement to create every file immediately.

For Milestone 1, keep `firmware/firmware.ino` as the build target. The sketch folder and primary `.ino` filename must match. Add source files only when the implementation needs them.

## 11. Milestones

### Milestone 1 — Toolchain and Minimal Firmware

- Inspect Windows, PowerShell, Git, Antigravity CLI and Arduino CLI.
- Verify the Arduino AVR board core.
- Detect the Uno and serial port.
- Create a minimal sketch.
- Compile successfully.
- Upload only after explicit user approval.
- Report what was actually verified.

No keypad, LCD, RTC, DHT11 or IR integration in this milestone.

### Milestone 2 — Core Terminal

- Verify the new keypad mapping.
- Integrate keypad input.
- Integrate the existing LCD.
- Implement the home menu.
- Test selection and execute behavior.

### Milestone 3 — Calculator

- Implement calculator input and arithmetic.
- Handle invalid expressions and division by zero.
- Test entry and exit shortcuts.

### Milestone 4 — Hardware Applications

- Integrate DS1302.
- Integrate DHT11.
- Integrate the IR receiver.
- Test each module independently.

### Milestone 5 — Integration and Reliability

- Integrate all applications.
- Test switching between applications.
- Verify shortcut priority.
- Check memory use.
- Verify input responsiveness.
- Update documentation to match actual behavior.

## 12. Definition of Done

A milestone is complete only when:

1. Required implementation exists.
2. Compilation passes for the intended board.
3. Relevant tests pass.
4. Hardware-dependent behavior is physically verified when hardware is available.
5. Documentation matches implementation.
6. Remaining limitations are reported honestly.

Compilation does not prove hardware functionality. Successful upload does not prove every feature works.

## End of Architecture
