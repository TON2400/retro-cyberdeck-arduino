# Calculator Test Specification (`test_calculator.md`)

## Test Cases

| Case | Keypad Input Sequence | Expression | Expected Line 1 |
|---|---|---|---|
| Open Calculator | `A`, `B`, `C`, `D`, `#` (or `4`, `#`) | `CALC: 0` | `A+B-C*D/ *. #=` |
| Multi-Operator Precedence | `3`, `C`, `2`, `B`, `1`, `#` | `3*2-1` | `=5` |
| Precedence (`+` before `*`) | `2`, `A`, `3`, `C`, `4`, `#` | `2+3*4` | `=14` |
| Leading Negative Real | `B`, `2`, `*`, `5`, `C`, `4`, `#` | `-2.5*4` | `=-10` |
| Negative Second Operand | `3`, `C`, `B`, `2`, `#` | `3*-2` | `=-6` |
| Multi-Operator Real | `B`, `3`, `*`, `5`, `C`, `2`, `A`, `1`, `*`, `5`, `#` | `-3.5*2+1.5` | `=-5.5` |
| Division by Zero | `8`, `D`, `0`, `#` | `8/0` | `ERR: DIV BY 0` |
| Malformed (Missing RHS) | `1`, `2`, `A`, `#` | `12+` | `ERR: INVALID` |
| Malformed (Invalid Op) | `1`, `2`, `A`, `C`, `3`, `#` | `12+*3` | `ERR: INVALID` |
| Clear Input | `*` (after `#`, or `**` while typing) | `CALC: 0` | `A+B-C*D/ *. #=` |
| Exit Calculator | `A`, `B`, `C`, `D`, `#` (or `*` when empty) | `RETRO CYBERDECK` | `>1:Clock       #` |
