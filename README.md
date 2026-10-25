# Retro Cyberdeck

A modular retro terminal project built for Arduino Uno R3 using existing hardware.

## Current Scope

- Arduino Uno R3
- 16x2 I2C LCD
- New 4x4 keypad (mapping to be verified)
- DS1302 RTC
- DHT11 temperature/humidity sensor
- IR receiver
- Calculator

No external LEDs are required. ESP32, OLED, Wi-Fi and Bluetooth are outside the current scope.

## Development Tools

- Windows PowerShell
- Antigravity CLI (`agy`)
- Arduino CLI
- Arduino AVR Boards core

## Getting Started

Read these documents in order:

1. `AGENTS.md`
2. `docs/ARCHITECTURE.md`
3. `docs/WORKFLOW.md`

Start with Milestone 1: compile the minimal Uno sketch, detect the board, and upload only after confirming the actual port and obtaining user approval.

## Budget

Current milestones must use existing hardware and must not require purchases.
