# Navigation & Shortcut Test Specification (`test_navigation.md`)

## Test Cases

| Step | Action / Keys | Expected Active App | Expected LCD Line 0 | Expected LCD Line 1 |
|---|---|---|---|---|
| 1 | Boot | `APP_HOME` | `RETRO CYBERDECK` | `>1:Clock       #` |
| 2 | Press `2` | `APP_HOME` | `RETRO CYBERDECK` | `>2:Temp/Humid  #` |
| 3 | Press `3` | `APP_HOME` | `RETRO CYBERDECK` | `>3:IR Inspect  #` |
| 4 | Press `*` | `APP_HOME` | `RETRO CYBERDECK` | `>1:Clock       #` |
| 5 | Press `1`, `#` | `APP_CLOCK` | `HH:MM:SS DS1302/SW-CLK` | `YYYY-MM-DD #Set*` |
| 6 | Press `*` | `APP_HOME` | `RETRO CYBERDECK` | `>1:Clock       #` |
| 7 | Press `2`, `#` | `APP_SENSOR` | `T:..C / ..F` or `DHT11 D11:WAIT` | `RH:..%` or `#:Read    *:Back` |
| 8 | Press `*` | `APP_HOME` | `RETRO CYBERDECK` | `>2:Temp/Humid  #` |
| 9 | Press `3`, `#` | `APP_IR_INSPECTOR` | `IR D10: Waiting` | `Aim Remote  *:Bk` |
| 10 | Press `*` | `APP_HOME` | `RETRO CYBERDECK` | `>3:IR Inspect  #` |
| 11 | Press `A,B,C,D,#` | `APP_CALCULATOR` | `CALC: 0` | `A+ B- C* D/ #=` |
| 12 | Press `A,B,C,D,#` | `APP_HOME` | `RETRO CYBERDECK` | `>3:IR Inspect  #` |
