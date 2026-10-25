# Hardware Integration Test Checklist (`test_hardware.md`)

## 1. 16x2 I2C LCD (`SDA=A4`, `SCL=A5`)
- Startup runs `scanI2CBus()` over `0x20..0x3F` with `Wire.setWireTimeout(3000, true)`.
- Initializes `LiquidCrystal_I2C` at the detected address (or `0x27` default) and mirrors all 16x2 updates to `Serial` at `115200` baud.

## 2. 4x4 Matrix Keypad (`R1-R4 = D9-D6`, `C1-C4 = D5-D2`)
- Every physical key press logs `[KEYPAD] Key='<char>' Row=R<1-4>(D<9-6>) Col=C<1-4>(D<5-2>)` to `Serial`.

## 3. DS1302 RTC (`CLK=A0`, `DAT=A1`, `RST=A2`)
- `ClockService` queries DS1302 via `ThreeWire(A1, A0, A2)`.
- Automatically falls back to a 1 Hz software clock (`SW-CLK`) if DS1302 is disconnected, and writes new times to DS1302 when edited via `#` in `ClockApp`.

## 4. DHT11 Temperature/Humidity Sensor (`DATA=D11`)
- Polled non-blockingly every `2000 ms` (or immediately on `#` in `SensorApp`).
- Displays Celsius, Fahrenheit, Relative Humidity (`%`), and Heat Index (`°C`).

## 5. IR Receiver (`OUT=D10`)
- Captured asynchronously via Pin Change Interrupt `PCINT0_vect` (`PCINT2` on `PB2`/`D10`).
- Decodes 32-bit NEC frames and computes 32-bit FNV-1a signatures for raw IR protocols.
