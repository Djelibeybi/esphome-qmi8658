# ESPHome QMI8658C IMU Component

An ESPHome external component for the QMI8658C 6-axis Inertial Measurement Unit (IMU) sensor.

## Features

- 3-axis accelerometer with configurable range (±2g/±4g/±8g/±16g)
- 3-axis gyroscope with configurable range (±16 to ±2048 °/s)
- Configurable output data rate (31.25 Hz to 8000 Hz)
- On-chip temperature sensor
- Pitch and roll angle calculation
- Motion detection binary sensor
- Device orientation text sensor
- Hardware Wake on Motion (WoM) with interrupt support
- Automation triggers for motion and orientation changes
- I2C interface support
- ESP-IDF framework compatible

## Sensor Specifications

| Specification       | Value                                 |
| ------------------- | ------------------------------------- |
| Manufacturer        | QST Corporation                       |
| Interface           | I2C (address: 0x6B)                   |
| Accelerometer Range | ±2g (default), ±4g, ±8g, ±16g         |
| Gyroscope Range     | ±2048 °/s (default), ±16 to ±1024 °/s |
| Output Data Rate    | 31.25 Hz to 8000 Hz (default: 500 Hz) |
| Package             | 2.5 x 3.0 x 0.86 mm LGA-14            |
| Temperature Range   | -40°C to +85°C                        |

## Installation

### Using as a Local Component

1. Copy the `components/qmi8658` folder to your ESPHome configuration directory.

2. Add the external component reference to your YAML:

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [qmi8658]
```

### Using from GitHub

```yaml
external_components:
  - source: github://Djelibeybi/esphome-qmi8658@main
    components: [qmi8658]
```

## Configuration

### I2C Bus

First, configure the I2C bus:

```yaml
i2c:
  sda: GPIO8
  scl: GPIO9
  scan: true
```

### Component Configuration

The QMI8658 component must be configured first, then sensor, binary_sensor, and text_sensor platforms reference it:

```yaml
# Create the main QMI8658 component
qmi8658:
  id: imu_sensor # Required, can be any valid ID (e.g., imu, my_sensor)
  address: 0x6B # Optional, 0x6B is default
  update_interval: 100ms # Optional, 60s is default

  # Range and ODR configuration (all optional)
  accel_range: 2G # Options: 2G, 4G, 8G, 16G
  gyro_range: 2048DPS # Options: 16DPS, 32DPS, 64DPS, 128DPS, 256DPS, 512DPS, 1024DPS, 2048DPS
  accel_odr: 500HZ # Options: 8000HZ, 4000HZ, 2000HZ, 1000HZ, 500HZ, 250HZ, 125HZ, 62.5HZ, 31.25HZ
  gyro_odr: 500HZ # Options: 8000HZ, 4000HZ, 2000HZ, 1000HZ, 500HZ, 250HZ, 125HZ, 62.5HZ, 31.25HZ

  # Mounting orientation correction (optional)
  rotation: 0            # Options: 0, 90, 180, 270 (degrees clockwise viewed from above)
  invert_z: false        # Swap face_up/face_down (default: false)
```

### Automation Triggers

The component supports automation triggers that fire without requiring separate sensor platforms. This is useful when you only need to react to motion or orientation changes without exposing sensors to Home Assistant.

```yaml
qmi8658:
  id: imu_sensor

  # Fires once when motion is detected (leading edge only)
  on_motion:
    threshold: 0.5 # Optional, m/s² deviation from gravity (default: 0.5)
    then:
      - logger.log: "Motion detected!"

  # Fires when device orientation changes
  on_orientation_change:
    then:
      - logger.log:
          format: "Orientation changed to: %s"
          args: ['orientation.c_str()']
```

#### Automation Trigger Configuration Variables

##### on_motion

Triggers when motion is detected. Fires on the leading edge (when motion starts), not continuously while in motion.

- **threshold** (_Optional_, float): Motion detection threshold as deviation from gravity in m/s². Range: 0.1 to 10.0. Default is `0.5`. Lower values are more sensitive.
- **then** (_Required_): Automation actions to execute when motion is detected.

> **Warning:** The trigger fires every time motion is detected, which can be very frequent during continuous or repetitive movement. Without debouncing, this can overwhelm your automations. Consider using a [script](https://esphome.io/components/script) with `mode: single` or adding a cooldown delay to prevent rapid repeated triggers.

##### on_orientation_change

Triggers when the device orientation changes. The `orientation` variable is available in the automation containing the new orientation string.

- **then** (_Required_): Automation actions to execute when orientation changes.

The `orientation` variable can have the following values:

- `face_up`: Device is flat, screen facing up
- `face_down`: Device is flat, screen facing down
- `portrait`: Device is upright
- `portrait_inverted`: Device is upside down
- `landscape_left`: Device is tilted left
- `landscape_right`: Device is tilted right
- `unknown`: Device is at an intermediate angle

### Sensor Platform Configuration

```yaml
sensor:
  - platform: qmi8658
    qmi8658_id: imu_sensor # Reference to the component

    # Accelerometer outputs (m/s²)
    accel_x:
      name: "Accelerometer X"
    accel_y:
      name: "Accelerometer Y"
    accel_z:
      name: "Accelerometer Z"

    # Gyroscope outputs (°/s)
    gyro_x:
      name: "Gyroscope X"
    gyro_y:
      name: "Gyroscope Y"
    gyro_z:
      name: "Gyroscope Z"

    # Temperature (°C)
    temperature:
      name: "IMU Temperature"

    # Tilt angles (degrees)
    pitch:
      name: "Pitch"
    roll:
      name: "Roll"
```

### Component Configuration Variables

#### General Options

- **id** (_Required_, string): Component ID used to reference this component from sensor platforms. You can use any valid ID (e.g., `imu`, `motion_sensor`, `my_qmi8658`).
- **address** (_Optional_, int): The I2C address of the sensor. Default is `0x6B`.
- **update_interval** (_Optional_, time): The interval to check the sensor. Default is `60s`.

#### Mounting Orientation

- **rotation** (_Optional_, int): Clockwise rotation in degrees when viewed from above, to compensate for sensor mounting orientation. Options: `0`, `90`, `180`, `270`. Default is `0`.
- **invert_z** (_Optional_, boolean): Invert the Z axis to swap face-up and face-down detection. Default is `false`.

#### Range and ODR Configuration

- **accel_range** (_Optional_): Accelerometer full-scale range. Default is `2G`.

  - `2G`: ±2g (highest sensitivity)
  - `4G`: ±4g
  - `8G`: ±8g
  - `16G`: ±16g (lowest sensitivity, highest range)

- **gyro_range** (_Optional_): Gyroscope full-scale range. Default is `2048DPS`.

  - `16DPS`: ±16 °/s (highest sensitivity)
  - `32DPS`: ±32 °/s
  - `64DPS`: ±64 °/s
  - `128DPS`: ±128 °/s
  - `256DPS`: ±256 °/s
  - `512DPS`: ±512 °/s
  - `1024DPS`: ±1024 °/s
  - `2048DPS`: ±2048 °/s (lowest sensitivity, highest range)

- **accel_odr** (_Optional_): Accelerometer output data rate. Default is `500HZ`.
- **gyro_odr** (_Optional_): Gyroscope output data rate. Default is `500HZ`.
  - Options: `8000HZ`, `4000HZ`, `2000HZ`, `1000HZ`, `500HZ`, `250HZ`, `125HZ`, `62.5HZ`, `31.25HZ`

### Sensor Platform Configuration Variables

- **qmi8658_id** (_Required_): Reference to the component ID.

#### Sensor Outputs

- **accel_x** (_Optional_): Acceleration on the X-axis in m/s².
- **accel_y** (_Optional_): Acceleration on the Y-axis in m/s².
- **accel_z** (_Optional_): Acceleration on the Z-axis in m/s².
- **gyro_x** (_Optional_): Angular velocity on the X-axis in °/s.
- **gyro_y** (_Optional_): Angular velocity on the Y-axis in °/s.
- **gyro_z** (_Optional_): Angular velocity on the Z-axis in °/s.
- **temperature** (_Optional_): Internal temperature sensor in °C.
- **pitch** (_Optional_): Pitch angle (rotation around Y-axis) in degrees.
- **roll** (_Optional_): Roll angle (rotation around X-axis) in degrees.

All sensor outputs accept the standard [Sensor](https://esphome.io/components/sensor/#config-sensor) configuration options.

### Binary Sensor Platform Configuration

The binary sensor platform provides motion detection capabilities.

```yaml
binary_sensor:
  - platform: qmi8658
    qmi8658_id: imu_sensor # Reference to the component

    # Software-based motion detection
    motion:
      name: "Motion Detected"
      threshold: 0.5 # Optional, deviation from gravity in m/s² (default: 0.5)

    # Hardware Wake on Motion (requires interrupt pin)
    wom:
      name: "Wake on Motion"
      threshold: 100 # Optional, threshold in mg (1-255, default: 100)
      pin: GPIO4 # Required, interrupt pin from QMI8658C INT1 or INT2
```

#### Binary Sensor Configuration Variables

- **qmi8658_id** (_Required_): Reference to the component ID.

##### Motion Detection (Software)

- **motion** (_Optional_): Software-based motion detection.
  - **threshold** (_Optional_, float): Motion detection threshold as deviation from gravity in m/s². Default is `0.5`. Higher values are less sensitive.
  - All standard [Binary Sensor](https://esphome.io/components/binary_sensor/#config-binary-sensor) options are supported, including filters for debouncing:

```yaml
binary_sensor:
  - platform: qmi8658
    qmi8658_id: imu_sensor
    motion:
      name: "Motion Detected"
      threshold: 0.5
      filters:
        - delayed_off: 1s  # Stay "on" for at least 1 second after motion stops
```

##### Wake on Motion (Hardware)

- **wom** (_Optional_): Hardware-based Wake on Motion using the QMI8658C's built-in motion detection.
  - **threshold** (_Optional_, int): WoM threshold in milligrams (1-255). Default is `100`.
  - **pin** (_Required_): GPIO pin connected to QMI8658C INT1 or INT2 output.

> **Note:** When WoM is enabled, the gyroscope is disabled per datasheet requirements. This means gyroscope sensor readings will not update. Additionally, orientation detection and the software-based motion sensor/trigger will not function reliably because the accelerometer operates in a special low-power mode optimized for wake detection rather than continuous data streaming.
>
> WoM is best suited for low-power/deep-sleep scenarios where the IMU's interrupt is used to wake the MCU from sleep. For normal operation where you need orientation detection or software motion sensing, use the `on_motion` trigger or `motion` binary sensor instead.

### Text Sensor Platform Configuration

The text sensor platform provides device orientation detection.

```yaml
text_sensor:
  - platform: qmi8658
    qmi8658_id: imu_sensor # Reference to the component

    orientation:
      name: "Device Orientation"
```

#### Text Sensor Configuration Variables

- **qmi8658_id** (_Required_): Reference to the component ID.

##### Orientation Detection

- **orientation** (_Optional_): Reports device orientation based on gravity vector.
  - Possible values:
    - `face_up`: Device is flat, screen facing up
    - `face_down`: Device is flat, screen facing down
    - `portrait`: Device is upright
    - `portrait_inverted`: Device is upside down
    - `landscape_left`: Device is tilted left
    - `landscape_right`: Device is tilted right
    - `unknown`: Device is at an intermediate angle

## Complete Example

```yaml
# Full example with all features

# Create the main component
qmi8658:
  id: imu_sensor
  address: 0x6B
  update_interval: 100ms
  accel_range: 4G
  gyro_range: 512DPS
  accel_odr: 500HZ
  gyro_odr: 500HZ
  rotation: 90          # Compensate for 90° rotated mounting
  invert_z: true        # USB port faces down, so invert Z

  # Automation triggers (optional, work without sensor platforms)
  on_motion:
    threshold: 0.5
    then:
      - logger.log: "Motion detected!"

  on_orientation_change:
    then:
      - logger.log:
          format: "Orientation: %s"
          args: ['orientation.c_str()']

# Sensor platform
sensor:
  - platform: qmi8658
    qmi8658_id: imu_sensor
    accel_x:
      name: "Accelerometer X"
    accel_y:
      name: "Accelerometer Y"
    accel_z:
      name: "Accelerometer Z"
    gyro_x:
      name: "Gyroscope X"
    gyro_y:
      name: "Gyroscope Y"
    gyro_z:
      name: "Gyroscope Z"
    temperature:
      name: "IMU Temperature"
    pitch:
      name: "Pitch"
    roll:
      name: "Roll"

# Binary sensor platform
binary_sensor:
  - platform: qmi8658
    qmi8658_id: imu_sensor
    motion:
      name: "Motion Detected"
      threshold: 0.5

# Text sensor platform
text_sensor:
  - platform: qmi8658
    qmi8658_id: imu_sensor
    orientation:
      name: "Device Orientation"
```

## Example Output

When the device is flat and stationary (face up):

- Accelerometer X and Y: ~0 m/s²
- Accelerometer Z: ~9.8 m/s² (gravity)
- Gyroscope X, Y, Z: ~0 °/s
- Pitch: ~0°
- Roll: ~0°
- Orientation: "face_up"
- Motion: OFF

## Wiring

### Basic Wiring (I2C only)

| QMI8658C Pin | ESP32-S3 Pin                   |
| ------------ | ------------------------------ |
| VCC          | 3.3V                           |
| GND          | GND                            |
| SDA          | GPIO8 (or your configured SDA) |
| SCL          | GPIO9 (or your configured SCL) |

### With Wake on Motion Interrupt

| QMI8658C Pin | ESP32-S3 Pin                  |
| ------------ | ----------------------------- |
| VCC          | 3.3V                          |
| GND          | GND                           |
| SDA          | GPIO8                         |
| SCL          | GPIO9                         |
| INT1         | GPIO4 (or any available GPIO) |

## Troubleshooting

### Sensor not detected

1. Check I2C wiring connections
2. Verify the I2C address (default is 0x6B)
3. Enable I2C bus scanning in your configuration:
   ```yaml
   i2c:
     scan: true
   ```
4. Check the logs for I2C errors

### Invalid readings

1. Ensure the sensor is properly powered (3.3V)
2. Verify the I2C pull-up resistors are present (many breakout boards include these)
3. Try reducing the I2C frequency if experiencing communication errors

### Motion detection too sensitive/insensitive

1. Adjust the `threshold` value for the motion binary sensor
2. Lower values are more sensitive (detect smaller movements)
3. Higher values require more significant motion to trigger

### Orientation not updating

1. Ensure the device experiences sufficient tilt (~45°) to trigger orientation change
2. The sensor uses a threshold of approximately 7 m/s² (~45° tilt) to determine orientation

## Development Notes

This component was developed iteratively with the assistance of [Claude](https://claude.ai) (Anthropic's AI). The initial prompt was:

> "Please create an ESPHome external component for the QMI8658 IMU found on the Waveshare ESP32-S3 Touch LCD 1.83"

From there, features were added based on real-world usage and ideas that emerged during testing:

- Configurable accelerometer/gyroscope ranges and output data rates
- Pitch and roll angle calculation
- Motion detection (software-based) and device orientation sensing
- Wake-on-Motion hardware interrupt support for low-power applications
- Automation triggers for sensor-free automations
- Low-pass filtering for smoother readings

The code, documentation, and examples were refined through conversation.

## License

Universal Permissive License v1.0 (UPL-1.0)

## References

- [QMI8658C Datasheet](https://files.waveshare.com/upload/3/36/QMI8658C_datasheet_rev_0.8.pdf)
- [ESPHome Documentation](https://esphome.io/)
- [ESPHome External Components](https://esphome.io/components/external_components/)
- [ESPHome Sensor Component](https://esphome.io/components/sensor/)
- [ESPHome Binary Sensor Component](https://esphome.io/components/binary_sensor/)
- [ESPHome Text Sensor Component](https://esphome.io/components/text_sensor/)
- [ESPHome Automations](https://esphome.io/automations/)
