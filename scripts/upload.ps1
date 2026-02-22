# Upload firmware to ESP32-S3 board
# Prompts for board selection, then builds and flashes

$pio = "pio"
# Try common PlatformIO locations if not in PATH
if (-not (Get-Command $pio -ErrorAction SilentlyContinue)) {
    $winPath = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"
    if (Test-Path $winPath) { $pio = $winPath }
}
$projectDir = Join-Path $PSScriptRoot "..\firmware"

Write-Host ""
Write-Host "ClaudeGauge - Firmware Upload" -ForegroundColor Cyan
Write-Host "---------------------------------------"
Write-Host ""
Write-Host "Select target board:" -ForegroundColor Cyan
Write-Host "  [1] LILYGO T-Display-S3  (170x320, parallel, 2 buttons)"
Write-Host "  [2] Waveshare ESP32-S3-LCD-1.47  (172x320, SPI, 1 button)"
Write-Host ""

$choice = Read-Host "Enter choice (1 or 2)"

switch ($choice) {
    "1" {
        $env = "tdisplays3"
        $boardName = "LILYGO T-Display-S3"
    }
    "2" {
        $env = "waveshare147"
        $boardName = "Waveshare ESP32-S3-LCD-1.47"
    }
    default {
        Write-Host "Invalid choice. Exiting." -ForegroundColor Red
        exit 1
    }
}

Write-Host ""
Write-Host "Building and uploading for: $boardName" -ForegroundColor Yellow
Write-Host ""

& $pio run -d $projectDir -e $env --target upload

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host " Upload successful!" -ForegroundColor Green
    Write-Host " Board: $boardName" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Red
    Write-Host " Upload failed! (exit code $LASTEXITCODE)" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Red
    Write-Host ""
    Write-Host "Troubleshooting:" -ForegroundColor Yellow
    Write-Host "  - Is the board plugged in via USB?"
    Write-Host "  - Try holding BOOT while plugging in"
    Write-Host "  - Check Device Manager for COM port"
}
