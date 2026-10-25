# Retro Cyberdeck — User Guide

## 1. Home Menu Navigation

- Press `1`, `2`, `3`, or `4` to select an application:
  - `1`: **Clock** (DS1302 RTC + automatic computer upload time sync)
  - `2`: **Temperature & Humidity** (DHT11 sensor)
  - `3`: **IR Inspector** (IR receiver protocol & HEX code analyzer)
  - `4`: **Calculator** (Multi-operator real-number calculator)
- Press `#` to launch the selected application.
- Press `*` on the Home Menu to reset selection to `1`.

## 2. Global Calculator Shortcut (`A B C D #`)

- From the **Home Menu**, press `A`, `B`, `C`, `D`, `#` in sequence (or `4` then `#`) to open the **Calculator**.
- While inside the **Calculator**, press `A`, `B`, `C`, `D`, `#` in sequence (or press `*` when the input is already cleared) to exit back to the **Home Menu**.

## 3. Applications

### App 1: Clock (`1` then `#`)
- Automatically syncs to your computer's date & time on every firmware upload via `upload.ps1` and EEPROM build-stamp detection.
- Displays live `TIME: HH:MM:SS` on Line 0 and `YYYY-MM-DD #Set*` on Line 1.
- **Set Time (`#`):**
  1. Press `#` to enter `SET: __:__:__` mode.
  2. Enter 6 digits (`HHMMSS`, 24-hour format, e.g., `183000` for `18:30:00`).
  3. Press `#` to save to the DS1302 RTC and software clock, or `*` to cancel.
- Press `*` in normal view to return to the Home Menu.

### App 2: Temperature & Humidity (`2` then `#`)
- Reads the DHT11 sensor on `D11` every 2.5 seconds.
- Line 0 shows temperature in Celsius and Fahrenheit (`T:28.4C / 83F`).
- Line 1 shows relative humidity (`RH:64.0%`) or Heat Index (`HI:30.1C`).
- Press `#` to force an immediate sensor read and toggle between Relative Humidity (`RH`) and Heat Index (`HI`).
- Press `*` to return to the Home Menu.

### App 3: IR Inspector (`3` then `#`)
- Captures infrared remote signals on `D10` (`PCINT2`).
- Line 0 displays the 32-bit hexadecimal code (`IR:0x00FFA25D`).
- Line 1 displays the protocol (`NEC` or `RAW`) and bit count.
- Press `#` to clear the last captured signal and wait for a new remote press.
- Press `*` to return to the Home Menu.

### App 4: Calculator (`A B C D #` or `4` then `#`)
- Supports **multi-operator expressions** with standard operator precedence (`*`, `/` evaluated before `+`, `-`, e.g., `3*2-1` -> `=5`, `2+3*4` -> `=14`) and **negative real numbers** (e.g., `-2.5*4` -> `=-10`, `3*-2` -> `=-6`).
- **Keypad Controls:**
  - `0`–`9`: Digits
  - `A`: Addition (`+`)
  - `B`: Subtraction (`-`) / Unary minus (`-` at start of expression or after an operator, e.g., `B` `5` for `-5` or `3` `C` `B` `2` for `3*-2`)
  - `C`: Multiplication (`*`)
  - `D`: Division (`/`)
  - `*`:
    - When typing digits of a number (without a decimal point yet): press `*` once to insert a decimal point (`.`), e.g., `2` `*` `5` -> `2.5`.
    - Press `*` again (`**`) while typing, or press `*` once after `#`, to **clear** the input (`CALC: 0`).
    - When already cleared at `CALC: 0`, press `*` to exit to the Home Menu.
  - `#`: Evaluate expression
- After evaluating a result (e.g. `=5`), pressing an operator (`A`, `B`, `C`, `D`) chains the calculation from that result (`5+...`), while pressing a digit starts a new expression.
