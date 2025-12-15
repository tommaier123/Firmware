@echo off
REM Workaround script for building and uploading ESP32-C6 filesystem
REM This builds the filesystem using the generic 'fs' environment,
REM copies it to the C6 build directory, and uploads it to the device

echo ================================================================================
echo Building filesystem for ESP32-C6...
echo ================================================================================

REM Build filesystem with fs environment
pio run -e fs -t buildfs
if errorlevel 1 (
    echo ERROR: Failed to build filesystem
    exit /b 1
)

REM Copy to C6 build directory
if not exist ".pio\build\Seeed-Xiao-ESP32C6" mkdir ".pio\build\Seeed-Xiao-ESP32C6"
copy ".pio\build\fs\littlefs.bin" ".pio\build\Seeed-Xiao-ESP32C6\littlefs.bin"
if errorlevel 1 (
    echo ERROR: Failed to copy filesystem image
    exit /b 1
)

echo ================================================================================
echo Filesystem build complete: .pio\build\Seeed-Xiao-ESP32C6\littlefs.bin
echo ================================================================================

REM Upload filesystem to device
echo.
echo ================================================================================
echo Uploading filesystem to ESP32-C6...
echo ================================================================================

REM Find the correct esptool from PlatformIO packages
set ESPTOOL=C:\Users\Max\.platformio\packages\tool-esptoolpy\esptool.py
python "%ESPTOOL%" --chip esp32c6 write_flash 0x353000 ".pio\build\Seeed-Xiao-ESP32C6\littlefs.bin"
if errorlevel 1 (
    echo ERROR: Failed to upload filesystem
    exit /b 1
)

echo ================================================================================
echo Filesystem upload complete!
echo ================================================================================
