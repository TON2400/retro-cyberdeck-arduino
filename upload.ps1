param(
  [string]$Port = "COM5",
  [string]$Fqbn = "arduino:avr:uno"
)

# 1. Touch firmware.ino so __DATE__ and __TIME__ always reflect current computer time
(Get-Item "$PSScriptRoot\firmware\firmware.ino").LastWriteTime = Get-Date

Write-Host "[1/3] Compiling firmware with current computer timestamp ($(Get-Date -Format 'yyyy-MM-dd HH:mm:ss'))..."
arduino-cli compile --fqbn $Fqbn "$PSScriptRoot\firmware"
if ($LASTEXITCODE -ne 0) {
  Write-Error "Compilation failed."
  exit $LASTEXITCODE
}

Write-Host "[2/3] Uploading firmware to $Port ($Fqbn)..."
arduino-cli upload --fqbn $Fqbn --port $Port -v "$PSScriptRoot\firmware"
if ($LASTEXITCODE -ne 0) {
  Write-Error "Upload failed."
  exit $LASTEXITCODE
}

Write-Host "[3/3] Synchronizing exact computer clock to Arduino on $Port..."
Start-Sleep -Milliseconds 1800
try {
  $sp = New-Object System.IO.Ports.SerialPort $Port, 115200, None, 8, one
  $sp.Open()
  Start-Sleep -Milliseconds 200
  $stamp = "T" + (Get-Date).ToString("yyyyMMddHHmmss")
  $sp.Write($stamp)
  Start-Sleep -Milliseconds 200
  $sp.Close()
  Write-Host "Clock synchronized to $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss') ($stamp)."
} catch {
  Write-Warning "Uploaded with compile-time clock sync; live serial post-sync skipped: $_"
}
