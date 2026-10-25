# Retro Cyberdeck — User Guide

## 1. Home Menu Navigation

- Press `1`, `2`, or `3` to select an application:
  - `1`: **Clock** (DS1302 RTC + software clock fallback)
  - `2`: **Temperature & Humidity** (DHT11 sensor)
  - `3`: **IR Inspector** (IR receiver protocol & HEX code analyzer)
- Press `#` to launch the selected application.
- Press `*` on the Home Menu to reset selection to `1`.

## 2. Global Calculator Shortcut (`A B C D #`)

- From the **Home Menu**, press `A`, `B`, `C`, `D`, `#` in sequence to open the **Calculator**.
- While inside the **Calculator**, press `A`, `B`, `C`, `D`, `#` in sequence to exit back to the **Home Menu**.

## 3. Applications

### App 1: Clock (`1` then `#`)
- Displays live `HH:MM:SS` (`DS1302` hardware RTC when detected on `A0/A1/A2`, or `SW-CLK` fallback) on Line 0 and `YYYY-MM-DD` on Line 1.
- **Set Time (`#`):**
  1. Press `#` to enter `SET: __:__:__` mode.
  2. Enter 6 digits (`HHMMSS`, 24-hour format, e.g., `183000` for `18:30:00`).
  3. Press `#` to save to the DS1302 RTC and software clock, or `*` to cancel.
- Press `*` in normal view to return to the Home Menu.

### App 2: Temperature & Humidity (`2` then `#`)
- Reads the DHT11 sensor on `D11` every 2 seconds.
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

### App 4: Calculator (`A B C D #`)
- Enter two operands and one operator:
  - `0`–`9`: Digits
  - `A`: Addition (`+`)
  - `B`: Subtraction (`-`)
  - `C`: Multiplication (`*`)
  - `D`: Division (`/`)
  - `#`: Evaluate expression
  - `*`: Clear input / reset error
- Handles decimal results (up to 4 decimal places with trailing zeros stripped), division by zero (`ERR: DIV BY 0`), malformed expressions (`ERR: INVALID`), and buffer limits (`ERR: MAX LEN`).
- Press `A`, `B`, `C`, `D`, `#` to exit to the Home Menu.
