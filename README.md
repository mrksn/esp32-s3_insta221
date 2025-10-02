# Insta Retrofit - Heat Press Automation System

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.x-blue)](https://docs.espressif.com/projects/esp-idf/en/v5.1.4/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

An automated temperature and timing control system for the Insta 221 heat press, designed for t-shirt printing businesses. Built with ESP32-S3 and ESP-IDF framework, featuring real-time PID temperature control, automated pressing cycles, and comprehensive safety mechanisms.

## 🎯 Features

- **Automated Pressing Cycles**: Two-stage timing system (configurable durations)
- **PID Temperature Control**: Precise temperature maintenance with configurable PID parameters
- **Safety Systems**: Emergency shutdown, temperature limits, sensor failure detection
- **User Interface**: OLED display with rotary encoder navigation and button controls
- **Persistent Storage**: Settings and statistics saved across power cycles
- **Progress Tracking**: Real-time monitoring of print runs and cycle completion
- **Safety Interlocks**: Press safety validation and emergency stop functionality

## 🛠️ Hardware Requirements

- **Microcontroller**: ESP32-S3-WROOM-1-N8
- **Temperature Sensor**: MAX31855 K-type thermocouple
- **Display**: SH1106 1.3" OLED I2C display
- **Input Controls**:
  - 20-step rotary encoder with push button
  - Confirm button
  - Back button
- **Outputs**:
  - Solid State Relay (SSR) for heating element control
  - Green LED (temperature ready indicator)
  - Blue LED (pause mode indicator)
- **Sensors**:
  - Reed switch for press closure detection

## 📋 Prerequisites

- **ESP-IDF v5.x** installed and configured
- **Development Environment**: Linux/macOS/Windows with ESP-IDF toolchain
- **Hardware**: ESP32-S3 development board and all required components

## 🚀 Quick Start

### 1. Environment Setup

```bash
# Clone the repository
git clone <repository-url>
cd gpt5mini

# Set up ESP-IDF environment (if not already done)
# Follow ESP-IDF installation guide: https://docs.espressif.com/projects/esp-idf/en/v5.1.4/get-started/

# Configure for ESP32-S3 target
idf.py set-target esp32s3
```

### 2. Build and Flash

```bash
# Build the project
idf.py build

# Flash to device
idf.py flash

# Monitor serial output
idf.py monitor
```

### 3. Initial Configuration

1. Power on the device
2. Use rotary encoder to navigate the menu system
3. Configure settings:
   - **Target Temperature**: Set desired pressing temperature (default: 140°C)
   - **PID Parameters**: Tune Kp, Ki, Kd for your heating system
   - **Stage Timers**: Set stage 1 (default: 15s) and stage 2 (default: 5s) durations
   - **Print Run**: Configure number of shirts and single/double-sided printing
4. Save settings and start your first print run

## 📖 Usage Guide

### Menu Navigation

The system features a hierarchical menu system:

1. **Job Settings**: Configure print run parameters (shirt count, single/double-sided)
2. **Start Pressing**: Begin automated pressing cycles
3. **Timings**: Adjust stage 1 and stage 2 durations
4. **Temperature**: Set target temperature and PID parameters
5. **Statistics**: View current run progress and timing data

**Controls**:

- **Rotary Encoder**: Navigate menus and adjust values
- **Confirm Button**: Select menu items and confirm actions
- **Back Button**: Return to previous menu level

### Operating Procedure

1. **Setup**: Configure target temperature and print run parameters
2. **Heating**: Wait for green LED to indicate target temperature reached
3. **Pressing**: Close the heat press manually to start each cycle
4. **Automation**: System automatically times stage 1 and stage 2
5. **Progress**: Monitor completion through display and statistics

### Safety Features

- **Temperature Limits**: Automatic heating shutdown above 220°C
- **Sensor Monitoring**: System halts on temperature sensor failure
- **Emergency Stop**: Immediate shutdown capability
- **Cycle Validation**: Safety checks before starting pressing cycles
- **Heap Monitoring**: Memory usage validation

## 🔧 Configuration

### Default Settings

| Parameter          | Default Value | Range    | Description           |
| ------------------ | ------------- | -------- | --------------------- |
| Target Temperature | 140°C         | 20-200°C | Pressing temperature  |
| Stage 1 Duration   | 15 seconds    | 1-300s   | First pressing stage  |
| Stage 2 Duration   | 5 seconds     | 1-300s   | Second pressing stage |
| PID Kp             | System-tuned  | 0.1-10.0 | Proportional gain     |
| PID Ki             | System-tuned  | 0.0-5.0  | Integral gain         |
| PID Kd             | System-tuned  | 0.0-5.0  | Derivative gain       |

### Safety Limits

- **Maximum Temperature**: 220°C (heating disabled)
- **Sensor Timeout**: 30 seconds (system halt)
- **Maximum Cycle Time**: 300 seconds
- **Minimum Heap**: 8KB free memory
- **Sensor Retry**: 3 attempts with 500ms delay

## 🧪 Testing

### Unit Tests

```bash
# Run unit tests
idf.py test
```

Test coverage includes:

- Safety validation functions
- Data validation and error handling
- Performance benchmarks (<1s response times)
- Memory usage monitoring

### Integration Testing

1. **Hardware Setup**: Connect all components according to pin definitions
2. **Temperature Testing**: Verify PID control maintains target temperature
3. **Cycle Testing**: Validate stage timing and press detection
4. **Safety Testing**: Confirm emergency shutdown and limit enforcement

## 📊 Monitoring and Statistics

The system tracks:

- **Time Elapsed**: Total run time
- **Shirts Completed**: Progress through print run
- **Average Time per Shirt**: Performance metrics
- **Temperature History**: Sensor readings and control performance
- **Error Events**: Safety incidents and system issues

## 🛡️ Safety and Reliability

### Safety Mechanisms

- **Emergency Shutdown System**: Immediate system lockdown on critical errors
- **Temperature Hysteresis**: ±5°C tolerance band for stable control
- **Press Safety Interlocks**: Validation before cycle start
- **Sensor Redundancy**: Retry logic for reliable readings
- **Memory Protection**: Heap monitoring prevents memory exhaustion

### Error Recovery

- **Graceful Degradation**: System attempts recovery from non-critical errors
- **State Persistence**: Progress saved during power interruptions
- **Error Logging**: Comprehensive logging for troubleshooting
- **Validation Checks**: Input validation prevents invalid configurations

## 🏗️ Architecture

### Software Components

- **Main Application**: FreeRTOS task coordination and system initialization
- **Temperature Control**: PID controller with sensor integration
- **User Interface**: Menu system with display and input handling
- **Storage System**: NVS-based persistent configuration
- **Safety Monitor**: Real-time system health checking

### Task Structure

- `ui_task`: User interface and menu navigation
- `temp_control_task`: Temperature regulation and PID control
- `pressing_task`: Cycle timing and press detection
- `watchdog_task`: System health monitoring and safety checks

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Implement changes with comprehensive tests
4. Ensure all tests pass
5. Submit a pull request

## 📝 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🆘 Troubleshooting

### Common Issues

**System won't start**:

- Check ESP-IDF installation and environment variables
- Verify hardware connections and power supply
- Check serial output for initialization errors

**Temperature control unstable**:

- Adjust PID parameters for your specific heating system
- Ensure proper thermocouple connection and placement
- Check for electrical noise or interference

**Display not responding**:

- Verify I2C connections and pull-up resistors
- Check OLED power supply and addressing
- Test with different I2C bus speed settings

**Press detection not working**:

- Verify reed switch wiring and magnet alignment
- Check GPIO pin configuration
- Test switch continuity with multimeter

### Debug Mode

Enable debug logging by modifying `ESP_LOG_LEVEL` in `sdkconfig`:

```bash
idf.py menuconfig
# Component config → Log output → Default log verbosity → Debug
```

## 📚 Additional Resources

- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/v5.1.4/)
- [ESP32-S3 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [MAX31855 Datasheet](https://datasheets.maximintegrated.com/en/ds/MAX31855.pdf)
- [SH1106 OLED Documentation](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf)

## 🔄 Version History

- **v1.0.0**: Initial release with core automation features
- Safety mechanisms and error handling
- Performance optimizations and validation
- Comprehensive testing suite

---

**Built with ❤️ for the t-shirt printing community**
