# Retro Cyberdeck — Uno R3 Pin Mapping

## Authoritative Pin Assignments

| Component | Signal | Uno Pin | Notes |
|---|---|---|---|
| Keypad (4x4) | Row 1 (`R1`) | `D9` | Matrix Row 1 |
| Keypad (4x4) | Row 2 (`R2`) | `D8` | Matrix Row 2 |
| Keypad (4x4) | Row 3 (`R3`) | `D7` | Matrix Row 3 |
| Keypad (4x4) | Row 4 (`R4`) | `D6` | Matrix Row 4 |
| Keypad (4x4) | Col 1 (`C1`) | `D5` | Matrix Column 1 |
| Keypad (4x4) | Col 2 (`C2`) | `D4` | Matrix Column 2 |
| Keypad (4x4) | Col 3 (`C3`) | `D3` | Matrix Column 3 |
| Keypad (4x4) | Col 4 (`C4`) | `D2` | Matrix Column 4 |
| IR Receiver | `OUT` | `D10` | `PB2` / `PCINT2` interrupt capture |
| DHT11 Sensor | `DATA` | `D11` | Digital temperature & humidity |
| DS1302 RTC | `CLK` (`SCLK`) | `A0` | Digital GPIO on `A0` |
| DS1302 RTC | `DAT` (`I/O`) | `A1` | Digital GPIO on `A1` |
| DS1302 RTC | `RST` (`CE`) | `A2` | Digital GPIO on `A2` |
| 16x2 I2C LCD | `SDA` | `A4` | Hardware TWI SDA (auto-detected `0x27`/`0x3F`) |
| 16x2 I2C LCD | `SCL` | `A5` | Hardware TWI SCL |
| USB Serial | `RX` / `TX` | `D0` / `D1` | `115200` baud diagnostic & input mirror |

Reserved pins: `D12`, `D13`, `A3`.

## 4x4 Keypad Matrix Layout

| Row \ Col | `C1` (`D5`) | `C2` (`D4`) | `C3` (`D3`) | `C4` (`D2`) |
|---|---|---|---|---|
| **`R1` (`D9`)** | `1` | `2` | `3` | `A` |
| **`R2` (`D8`)** | `4` | `5` | `6` | `B` |
| **`R3` (`D7`)** | `7` | `8` | `9` | `C` |
| **`R4` (`D6`)** | `*` | `0` | `#` | `D` |
