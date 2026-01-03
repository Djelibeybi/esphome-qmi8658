# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESPHome external component for the QMI8658C 6-axis IMU sensor (accelerometer + gyroscope) by QST Corporation. Provides sensor, binary_sensor (motion detection, hardware Wake-on-Motion), and text_sensor (orientation) platforms.

## Development Commands

```bash
# Validate and compile configuration
esphome compile example.yaml

# Upload to device
esphome upload example.yaml

# View device logs
esphome logs example.yaml

# Compile with verbose output for debugging
esphome compile example.yaml --verbose
```

**Note:** No unit test framework - testing is done by compiling and uploading to actual hardware.

## Architecture

The component uses ESPHome's multi-platform external component pattern:

```
components/qmi8658/
├── __init__.py      # Main component: config schema, range/ODR settings, code generation
├── sensor.py        # Sensor platform: accel, gyro, temperature, pitch, roll
├── binary_sensor.py # Binary sensor platform: motion detection, WoM interrupt
├── text_sensor.py   # Text sensor platform: orientation detection
├── qmi8658.h        # C++ header: register definitions, class declaration
└── qmi8658.cpp      # C++ implementation: I2C communication, sensor logic
```

**Component registration pattern:** The main `__init__.py` defines `CONFIG_SCHEMA` and `to_code()` for the base component. Each platform file (sensor.py, binary_sensor.py, text_sensor.py) extends this with `PLATFORM_SCHEMA` (keyed by `qmi8658_id`) and its own `to_code()`.

**C++ class hierarchy:** `QMI8658Component` inherits from `PollingComponent` (periodic updates) and `i2c::I2CDevice` (I2C communication).

## Code Style

### C++
- Register constants: `QMI8658_REG_*` (uppercase with prefix)
- Enum classes: `AccelScale`, `GyroScale`, `AccelODR`, `GyroODR`, `LPFMode`
- Member variables: trailing underscore (e.g., `accel_x_sensor_`)
- Logging: `ESP_LOGI`, `ESP_LOGD`, `ESP_LOGW`, `ESP_LOGE` macros
- Error handling: `mark_failed()` for setup failures, `status_set_warning()`/`status_clear_warning()` for runtime issues

### Python
- Config keys: `CONF_*` constants
- Range/ODR mappings: `ACCEL_RANGE`, `GYRO_RANGE`, `ACCEL_ODR`, `GYRO_ODR` dicts
- Async code generation: `async def to_code(config)`

## Git Preferences

Always use `--no-gpg-sign` when committing.
