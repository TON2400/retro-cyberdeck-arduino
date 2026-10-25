# Calculator Test Specification (`test_calculator.md`)

## Test Cases

| Case | Input Sequence | Expected Line 0 | Expected Line 1 |
|---|---|---|---|
| Open Calculator | `A`, `B`, `C`, `D`, `#` (from Home) | `CALC: 0` | `A+ B- C* D/ #=` |
| Addition | `1`, `2`, `A`, `3`, `4`, `#` | `12+34` | `=46` |
| Subtraction | `9`, `0`, `B`, `2`, `5`, `#` | `90-25` | `=65` |
| Multiplication | `1`, `5`, `C`, `6`, `#` | `15*6` | `=90` |
| Decimal Division | `7`, `D`, `2`, `#` | `7/2` | `=3.5` |
| Decimal Operands | `1`, `.`, `5`, `A`, `2`, `.`, `2`, `5`, `#` | `1.5+2.25` | `=3.75` |
| Division by Zero | `8`, `D`, `0`, `#` | `8/0` | `ERR: DIV BY 0` |
| Malformed (Missing RHS) | `1`, `2`, `A`, `#` | `12+` | `ERR: INVALID` |
| Malformed (Double Op) | `1`, `2`, `A`, `B`, `3`, `#` | `12+-3` | `ERR: INVALID` |
| Clear Input | `*` | `CALC: 0` | `A+ B- C* D/ #=` |
| Exit Calculator | `A`, `B`, `C`, `D`, `#` | `RETRO CYBERDECK` | `>1:Clock       #` |
