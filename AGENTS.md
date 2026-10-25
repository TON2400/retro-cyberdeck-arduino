# Antigravity Agent Instructions

## Role

You are the implementation agent for Retro Cyberdeck, an Arduino Uno R3 project developed on Windows using PowerShell and Arduino CLI.

Inspect the environment and project before changing anything. Implement one milestone at a time and verify every change.

## Read First

Before modifying files, read:

- `AGENTS.md`
- `docs/ARCHITECTURE.md`
- `docs/WORKFLOW.md`

Preserve existing user work. Do not recreate or overwrite files without inspecting them first.

## Current Project Scope

- Target board: Arduino Uno R3 only
- Display: existing 16x2 LCD with I2C backpack
- Input: new 4x4 keypad; exact mapping must be tested
- Planned applications: Home Menu, Clock, Temperature/Humidity, IR Inspector, Calculator
- Development environment: Windows, PowerShell, Arduino CLI, Antigravity CLI
- Budget: use existing hardware only; do not require purchases

ESP32 and OLED are out of scope. Do not create ESP32 support, OLED drivers, or migration milestones.

## Milestone 1 Restrictions

Milestone 1 is limited to toolchain setup and minimal firmware.

Do not implement:

- Keypad
- LCD
- Calculator
- RTC
- DHT11
- IR receiver
- External LEDs
- Wi-Fi
- Bluetooth
- ESP32 support

Do not infer the new keypad mapping from the previous keypad.

## Hardware and Upload Safety

Before uploading firmware:

1. Run board detection.
2. Confirm the actual serial port.
3. Confirm the FQBN is `arduino:avr:uno`.
4. Show the exact upload command to the user.
5. Wait for explicit user approval.

Never guess the serial port. Never claim upload or physical hardware testing succeeded unless there is evidence.

## Coding Rules

- Use Arduino C++.
- Keep code small and readable.
- Prefer fixed-size buffers on Uno.
- Avoid unnecessary heap allocations and `String` usage.
- Avoid unnecessary blocking delays.
- Preserve existing working behavior.
- Do not add external LEDs.
- Do not require new hardware or paid tools.
- Keep hardware-specific code simple; do not introduce abstractions without a concrete need.

## Required Workflow

For each task:

1. Inspect current files and available tools.
2. State the immediate goal and planned changes.
3. Implement only the requested milestone.
4. Compile with Arduino CLI and the correct FQBN.
5. Fix compile errors and retry when possible.
6. Report exact commands and results.
7. Separate verified facts from assumptions.
8. Summarize changed files and next steps.

Do not report tests that were not executed. Compilation does not prove that hardware works.

## Final Report Format

- **Status:** PASS / FAIL / BLOCKED
- **Changed files:** files modified
- **Commands executed:** relevant commands
- **Verification:** what passed and what was not tested
- **Remaining issues:** unresolved problems
- **Next step:** one concrete action

## End of Instructions
