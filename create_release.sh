#!/bin/bash

# ESP32-S3 Insta Retrofit Release Script
# Creates a release package with firmware binaries and flashing tools

set -e

PROJECT_NAME="insta-retrofit"
VERSION=${1:-"v1.0.0"}
RELEASE_DIR="release-${VERSION}"

echo "🚀 Creating release ${VERSION} for ${PROJECT_NAME}"

# Check if build exists, if not build the project
if [ ! -f "build/${PROJECT_NAME}.bin" ]; then
    echo "📦 Building project..."
    if command -v idf.py &> /dev/null; then
        idf.py clean
        idf.py build
    else
        echo "⚠️  ESP-IDF not found in PATH. Using existing build if available..."
        if [ ! -d "build" ]; then
            echo "❌ No build directory found. Please install ESP-IDF and run: idf.py build"
            exit 1
        fi
    fi
else
    echo "✅ Using existing build..."
fi

# Create release directory
echo "📁 Creating release directory..."
rm -rf "${RELEASE_DIR}"
mkdir -p "${RELEASE_DIR}/firmware"
mkdir -p "${RELEASE_DIR}/tools"

# Copy firmware files
echo "📋 Copying firmware files..."
cp "build/${PROJECT_NAME}.bin" "${RELEASE_DIR}/firmware/"
cp "build/bootloader/bootloader.bin" "${RELEASE_DIR}/firmware/"
cp "build/partition_table/partition-table.bin" "${RELEASE_DIR}/firmware/"
cp "build/flasher_args.json" "${RELEASE_DIR}/firmware/"

# Create flashing script for Linux/macOS
cat > "${RELEASE_DIR}/flash.sh" << 'EOF'
#!/bin/bash

# ESP32-S3 Insta Retrofit Flashing Script
# Run this script to flash the firmware to your ESP32-S3 device

set -e

echo "🔥 Flashing ESP32-S3 Insta Retrofit firmware..."

# Check if esptool.py is available
if ! command -v esptool.py &> /dev/null; then
    echo "❌ esptool.py not found. Please install ESP-IDF or esptool:"
    echo "   pip install esptool"
    exit 1
fi

# Check if device is connected
if ! ls /dev/ttyUSB* 1>/dev/null 2>&1 && ! ls /dev/ttyACM* 1>/dev/null 2>&1; then
    echo "❌ No ESP32 device found. Please connect your ESP32-S3 and try again."
    echo "   Common device paths: /dev/ttyUSB0, /dev/ttyACM0"
    exit 1
fi

# Find the device (prefer ttyUSB over ttyACM)
DEVICE=""
if ls /dev/ttyUSB* 1>/dev/null 2>&1; then
    DEVICE=$(ls /dev/ttyUSB* | head -1)
elif ls /dev/ttyACM* 1>/dev/null 2>&1; then
    DEVICE=$(ls /dev/ttyACM* | head -1)
fi

echo "📡 Using device: ${DEVICE}"

# Flash the firmware
esptool.py --chip esp32s3 \
           --port "${DEVICE}" \
           --baud 921600 \
           --before default_reset \
           --after hard_reset \
           write_flash \
           --flash_mode dio \
           --flash_freq 80m \
           --flash_size 2MB \
           0x0 bootloader.bin \
           0x8000 partition-table.bin \
           0x10000 insta-retrofit.bin

echo "✅ Firmware flashed successfully!"
echo ""
echo "📖 Next steps:"
echo "1. Disconnect and reconnect your ESP32-S3"
echo "2. Open a serial monitor: screen ${DEVICE} 115200"
echo "3. Or use: idf.py monitor"
echo ""
echo "🎯 Happy printing!"
EOF

# Create Windows batch file
cat > "${RELEASE_DIR}/flash.bat" << 'EOF'
@echo off
REM ESP32-S3 Insta Retrofit Flashing Script (Windows)
REM Run this script to flash the firmware to your ESP32-S3 device

echo 🔥 Flashing ESP32-S3 Insta Retrofit firmware...

REM Check if esptool.py is available
esptool.py --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ esptool.py not found. Please install ESP-IDF or esptool:
    echo    pip install esptool
    goto :error
)

REM Find available COM ports (this is a simple check)
set DEVICE=
for /f "tokens=*" %%i in ('powershell "Get-WMIObject Win32_SerialPort | Select-Object DeviceID" 2^>nul') do (
    set DEVICE=%%i
    goto :found_device
)

if "%DEVICE%"=="" (
    echo ❌ No serial device found. Please connect your ESP32-S3 and try again.
    goto :error
)

:found_device
echo 📡 Using device: %DEVICE%

REM Flash the firmware
esptool.py --chip esp32s3 ^
           --port "%DEVICE%" ^
           --baud 921600 ^
           --before default_reset ^
           --after hard_reset ^
           write_flash ^
           --flash_mode dio ^
           --flash_freq 80m ^
           --flash_size 2MB ^
           0x0 bootloader.bin ^
           0x8000 partition-table.bin ^
           0x10000 insta-retrofit.bin

echo ✅ Firmware flashed successfully!
echo.
echo 📖 Next steps:
echo 1. Disconnect and reconnect your ESP32-S3
echo 2. Open a serial monitor or use your preferred terminal program
echo.
echo 🎯 Happy printing!
goto :end

:error
echo ❌ Flashing failed. Please check the error messages above.
exit /b 1

:end
EOF

# Create flashing script for PlatformIO
cat > "${RELEASE_DIR}/platformio-flash.py" << 'EOF'
#!/usr/bin/env python3
"""
ESP32-S3 Insta Retrofit PlatformIO Flashing Script
Place this script in your PlatformIO project root and run with:
pio run -t upload --upload-port /dev/ttyUSB0
"""

import os
import sys
import subprocess
from pathlib import Path

def flash_firmware():
    """Flash the firmware using esptool.py"""

    # Check if we're in the right directory
    firmware_dir = Path(__file__).parent / "firmware"
    if not firmware_dir.exists():
        print("❌ firmware directory not found. Please run this script from the release directory.")
        sys.exit(1)

    # Check for required files
    required_files = ["insta-retrofit.bin", "bootloader.bin", "partition-table.bin"]
    for file in required_files:
        if not (firmware_dir / file).exists():
            print(f"❌ {file} not found in firmware directory")
            sys.exit(1)

    # Get upload port from environment or ask user
    upload_port = os.environ.get('UPLOAD_PORT')
    if not upload_port:
        # Try to find a port
        try:
            import serial.tools.list_ports
            ports = [p.device for p in serial.tools.list_ports.comports() if 'USB' in p.device or 'ACM' in p.device]
            if ports:
                upload_port = ports[0]
                print(f"📡 Auto-detected port: {upload_port}")
            else:
                print("❌ No serial ports found. Please set UPLOAD_PORT environment variable.")
                sys.exit(1)
        except ImportError:
            print("❌ Please install pyserial: pip install pyserial")
            sys.exit(1)

    print("🔥 Flashing ESP32-S3 Insta Retrofit firmware...")

    # Flash command
    cmd = [
        "esptool.py",
        "--chip", "esp32s3",
        "--port", upload_port,
        "--baud", "921600",
        "--before", "default_reset",
        "--after", "hard_reset",
        "write_flash",
        "--flash_mode", "dio",
        "--flash_freq", "80m",
        "--flash_size", "2MB",
        "0x0", str(firmware_dir / "bootloader.bin"),
        "0x8000", str(firmware_dir / "partition-table.bin"),
        "0x10000", str(firmware_dir / "insta-retrofit.bin")
    ]

    try:
        result = subprocess.run(cmd, check=True, capture_output=True, text=True)
        print("✅ Firmware flashed successfully!")
        print("\n📖 Next steps:")
        print("1. Disconnect and reconnect your ESP32-S3")
        print("2. Monitor serial output if needed")
        print("\n🎯 Happy printing!")
    except subprocess.CalledProcessError as e:
        print(f"❌ Flashing failed: {e}")
        print(f"stdout: {e.stdout}")
        print(f"stderr: {e.stderr}")
        sys.exit(1)

if __name__ == "__main__":
    flash_firmware()
EOF

# Make scripts executable
chmod +x "${RELEASE_DIR}/flash.sh"
chmod +x "${RELEASE_DIR}/platformio-flash.py"

# Copy documentation
echo "📚 Copying documentation..."
cp "README.md" "${RELEASE_DIR}/"
cp "QUICK_REFERENCE.md" "${RELEASE_DIR}/"

# Create version info
cat > "${RELEASE_DIR}/version.txt" << EOF
ESP32-S3 Insta Retrofit Firmware
Version: ${VERSION}
Build Date: $(date -u +"%Y-%m-%d %H:%M:%S UTC")
Git Commit: $(git rev-parse --short HEAD)
ESP-IDF Version: $(grep "ESP-IDF" build/project_description.json | cut -d'"' -f4)
Target: ESP32-S3
Flash Size: 2MB
Flash Mode: DIO
Flash Frequency: 80MHz
EOF

# Create release notes template
cat > "${RELEASE_DIR}/RELEASE_NOTES.md" << EOF
# Release ${VERSION}

## Overview
Automated temperature and timing control system for the Insta 221 heat press.

## What's New in ${VERSION}
- [List major changes and new features here]

## Hardware Requirements
- ESP32-S3-WROOM-1-N8 microcontroller
- MAX31855 K-type thermocouple
- SH1106 1.3" OLED I2C display
- 20-step rotary encoder with push button
- Confirm and back buttons
- Solid State Relay (SSR) for heating control
- Reed switch for press detection
- Green and blue status LEDs

## Installation
1. Download the firmware files from the \`firmware/\` directory
2. Run the appropriate flashing script:
   - Linux/macOS: \`./flash.sh\`
   - Windows: \`flash.bat\`
   - PlatformIO: \`python platformio-flash.py\`

## Default Configuration
- Target Temperature: 140°C
- Stage 1 Duration: 15 seconds
- Stage 2 Duration: 5 seconds
- PID parameters: Auto-tuned for typical heat press

## Safety Features
- Emergency shutdown system
- Temperature limits (max 220°C)
- Sensor failure detection
- Memory monitoring
- Press safety interlocks

## Troubleshooting
- Ensure ESP-IDF v5.x is installed for development
- Check serial connections and device permissions
- Verify hardware connections match pin definitions
- Monitor serial output for error messages

## Support
For issues and questions, please check the main repository documentation.

---
Built with ESP-IDF $(grep "ESP-IDF" build/project_description.json | cut -d'"' -f4)
EOF

# Create archive
echo "📦 Creating release archive..."
tar -czf "${RELEASE_DIR}.tar.gz" "${RELEASE_DIR}"

# Create checksums
echo "🔐 Creating checksums..."
cd "${RELEASE_DIR}"
sha256sum firmware/* > checksums.sha256
cd ..

echo "✅ Release ${VERSION} created successfully!"
echo ""
echo "📁 Release contents:"
echo "  ${RELEASE_DIR}/"
echo "  ├── firmware/           # Binary files"
echo "  ├── flash.sh            # Linux/macOS flashing script"
echo "  ├── flash.bat           # Windows flashing script"
echo "  ├── platformio-flash.py # PlatformIO flashing script"
echo "  ├── README.md           # Documentation"
echo "  ├── RELEASE_NOTES.md    # Release notes"
echo "  └── version.txt         # Version information"
echo ""
echo "📦 Archive: ${RELEASE_DIR}.tar.gz"
echo ""
echo "🚀 To flash the firmware:"
echo "  cd ${RELEASE_DIR}"
echo "  ./flash.sh"
echo ""
echo "📋 Next steps:"
echo "1. Test the flashing scripts"
echo "2. Update RELEASE_NOTES.md with actual changes"
echo "3. Create a git tag: git tag ${VERSION}"
echo "4. Push to repository: git push origin ${VERSION}"
echo "5. Create GitHub release and upload the .tar.gz file"