# Retro Cyberdeck — Development Workflow

## Environment

- OS: Windows
- Shell: PowerShell
- Agent: Antigravity CLI (`agy`)
- Build tool: Arduino CLI
- Board: Arduino Uno R3
- FQBN: `arduino:avr:uno`

The current project uses existing hardware only. ESP32 and OLED work are out of scope.

## 1. Inspect the Environment

Run in PowerShell:

```powershell
$PSVersionTable.PSVersion
git --version
agy --version
arduino-cli version
arduino-cli config dump
arduino-cli core list
arduino-cli board list
```

If a command is missing, report it before installing anything.

## 2. Verify Arduino AVR Core

Update the board index:

```powershell
arduino-cli core update-index
```

Check installed cores:

```powershell
arduino-cli core list
```

If `arduino:avr` is missing:

```powershell
arduino-cli core install arduino:avr
```

Verify again:

```powershell
arduino-cli core list
arduino-cli board list
```

## 3. Create the Minimal Sketch

Initial build target:

`firmware/firmware.ino`

Minimal sketch:

```cpp
void setup() {
  Serial.begin(115200);
}

void loop() {
}
```

Inspect existing files before creating or overwriting anything.

Compile:

```powershell
arduino-cli compile --fqbn arduino:avr:uno firmware
```

Do not add peripherals or application logic to this sketch.

## 4. Detect the Board

Connect the Uno R3 using a USB data cable, then run:

```powershell
arduino-cli board list
```

Record the actual port, such as `COM3`. `COM3` is only an example; use the port detected on the user's machine.

If no board appears:

1. Check that the USB cable supports data.
2. Try another USB port.
3. Check Windows Device Manager.
4. Identify the USB-to-serial chip.
5. Install the appropriate driver only if needed.
6. Run board detection again.

Do not guess the port or install a random driver.

## 5. Upload After Explicit Approval

Before upload, confirm the board, FQBN and actual serial port with the user.

Example only:

```powershell
arduino-cli upload --fqbn arduino:avr:uno --port COM3 firmware
```

Replace `COM3` with the detected port. Do not execute the example unchanged unless it matches the actual environment.

## 6. Verify

Check the command exit status and report the result.

If needed, open Serial Monitor:

```powershell
arduino-cli monitor --port COM3 --config baudrate=115200
```

Use the actual port. The minimal sketch does not need to print recurring messages.

An upload success message alone does not prove every hardware feature works.

## 7. Git Checkpoint

After a verified milestone:

```powershell
git status
git add AGENTS.md docs firmware
git commit -m "Complete milestone 1: minimal Uno firmware"
```

Review `git status` first. Stage only intended project files.

If Git identity is not configured, report the issue and ask before changing global settings.

## 8. Continue to Milestone 2

Only after Milestone 1 is verified:

- Test the new keypad mapping.
- Integrate the existing LCD.
- Implement the home menu.
- Test navigation and execute behavior.

Integrate each peripheral independently. Do not require new hardware or paid tools.

## End of Workflow
