#pragma once

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/i2c/i2c.h"
#include <cmath>
#include <string>

namespace esphome {
namespace qmi8658 {

/// Component version - increment this when making changes for testing
static const char *const QMI8658_VERSION = "1.0.7";

/// QMI8658C Register Addresses
static const uint8_t QMI8658_REG_WHO_AM_I = 0x00;
static const uint8_t QMI8658_REG_REVISION = 0x01;
static const uint8_t QMI8658_REG_CTRL1 = 0x02;
static const uint8_t QMI8658_REG_CTRL2 = 0x03;
static const uint8_t QMI8658_REG_CTRL3 = 0x04;
static const uint8_t QMI8658_REG_CTRL5 = 0x06;
static const uint8_t QMI8658_REG_CTRL7 = 0x08;
static const uint8_t QMI8658_REG_CTRL9 = 0x0A;
static const uint8_t QMI8658_REG_RESET = 0x60;

/// Data output registers
static const uint8_t QMI8658_REG_TEMP_L = 0x33;
static const uint8_t QMI8658_REG_AX_L = 0x35;

/// Wake on Motion registers
static const uint8_t QMI8658_REG_CAL1_L = 0x0B;   ///< WoM threshold low byte
static const uint8_t QMI8658_REG_CAL1_H = 0x0C;   ///< WoM blanking/interrupt config
static const uint8_t QMI8658_REG_STATUSINT = 0x2D;  ///< Sensor data available, lock mechanism
static const uint8_t QMI8658_REG_STATUS0 = 0x2E;   ///< Output data over-run and availability
static const uint8_t QMI8658_REG_STATUS1 = 0x2F;   ///< Misc status: CmdDone (bit 0), WoM (bit 2)

/// CTRL9 commands
static const uint8_t QMI8658_CTRL9_CMD_ACK = 0x00;  ///< Acknowledge CTRL9 command completion
static const uint8_t QMI8658_CTRL9_CMD_WOM = 0x08;  ///< Configure Wake on Motion (CTRL_CMD_WRITE_WOM_SETTING)

/// Device identification
static const uint8_t QMI8658_CHIP_ID = 0x05;

/// CTRL7 enable bits
static const uint8_t QMI8658_CTRL7_ACC_EN = 0x01;
static const uint8_t QMI8658_CTRL7_GYR_EN = 0x02;

/// Accelerometer scale options (CTRL2 bits 4-6)
enum AccelScale : uint8_t {
  ACCEL_SCALE_2G = 0x00,   ///< +/- 2g, 16384 LSB/g
  ACCEL_SCALE_4G = 0x10,   ///< +/- 4g, 8192 LSB/g
  ACCEL_SCALE_8G = 0x20,   ///< +/- 8g, 4096 LSB/g
  ACCEL_SCALE_16G = 0x30,  ///< +/- 16g, 2048 LSB/g
};

/// Gyroscope scale options (CTRL3 bits 4-6)
enum GyroScale : uint8_t {
  GYRO_SCALE_16DPS = 0x00,    ///< +/- 16 dps, 2048 LSB/dps
  GYRO_SCALE_32DPS = 0x10,    ///< +/- 32 dps, 1024 LSB/dps
  GYRO_SCALE_64DPS = 0x20,    ///< +/- 64 dps, 512 LSB/dps
  GYRO_SCALE_128DPS = 0x30,   ///< +/- 128 dps, 256 LSB/dps
  GYRO_SCALE_256DPS = 0x40,   ///< +/- 256 dps, 128 LSB/dps
  GYRO_SCALE_512DPS = 0x50,   ///< +/- 512 dps, 64 LSB/dps
  GYRO_SCALE_1024DPS = 0x60,  ///< +/- 1024 dps, 32 LSB/dps
  GYRO_SCALE_2048DPS = 0x70,  ///< +/- 2048 dps, 16 LSB/dps
};

/// Accelerometer ODR options (CTRL2 bits 0-3)
enum AccelODR : uint8_t {
  ACCEL_ODR_8000HZ = 0x00,   ///< 8000 Hz
  ACCEL_ODR_4000HZ = 0x01,   ///< 4000 Hz
  ACCEL_ODR_2000HZ = 0x02,   ///< 2000 Hz
  ACCEL_ODR_1000HZ = 0x03,   ///< 1000 Hz
  ACCEL_ODR_500HZ = 0x04,    ///< 500 Hz
  ACCEL_ODR_250HZ = 0x05,    ///< 250 Hz
  ACCEL_ODR_125HZ = 0x06,    ///< 125 Hz
  ACCEL_ODR_62_5HZ = 0x07,   ///< 62.5 Hz
  ACCEL_ODR_31_25HZ = 0x08,  ///< 31.25 Hz
};

/// Gyroscope ODR options (CTRL3 bits 0-3)
enum GyroODR : uint8_t {
  GYRO_ODR_8000HZ = 0x00,   ///< 8000 Hz
  GYRO_ODR_4000HZ = 0x01,   ///< 4000 Hz
  GYRO_ODR_2000HZ = 0x02,   ///< 2000 Hz
  GYRO_ODR_1000HZ = 0x03,   ///< 1000 Hz
  GYRO_ODR_500HZ = 0x04,    ///< 500 Hz
  GYRO_ODR_250HZ = 0x05,    ///< 250 Hz
  GYRO_ODR_125HZ = 0x06,    ///< 125 Hz
  GYRO_ODR_62_5HZ = 0x07,   ///< 62.5 Hz
  GYRO_ODR_31_25HZ = 0x08,  ///< 31.25 Hz
};

/// Low-pass filter mode options (CTRL5)
/// Bandwidth is expressed as percentage of ODR
enum LPFMode : uint8_t {
  LPF_DISABLED = 0x00,  ///< Low-pass filter disabled
  LPF_2_66PCT = 0x01,   ///< 2.66% of ODR (most smoothing)
  LPF_3_63PCT = 0x02,   ///< 3.63% of ODR
  LPF_5_39PCT = 0x03,   ///< 5.39% of ODR
  LPF_13_37PCT = 0x04,  ///< 13.37% of ODR (least smoothing)
};

class QMI8658Component : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Sensor setters
  void set_accel_x_sensor(sensor::Sensor *accel_x_sensor) { accel_x_sensor_ = accel_x_sensor; }
  void set_accel_y_sensor(sensor::Sensor *accel_y_sensor) { accel_y_sensor_ = accel_y_sensor; }
  void set_accel_z_sensor(sensor::Sensor *accel_z_sensor) { accel_z_sensor_ = accel_z_sensor; }
  void set_gyro_x_sensor(sensor::Sensor *gyro_x_sensor) { gyro_x_sensor_ = gyro_x_sensor; }
  void set_gyro_y_sensor(sensor::Sensor *gyro_y_sensor) { gyro_y_sensor_ = gyro_y_sensor; }
  void set_gyro_z_sensor(sensor::Sensor *gyro_z_sensor) { gyro_z_sensor_ = gyro_z_sensor; }
  void set_temperature_sensor(sensor::Sensor *temperature_sensor) { temperature_sensor_ = temperature_sensor; }
  void set_pitch_sensor(sensor::Sensor *pitch_sensor) { pitch_sensor_ = pitch_sensor; }
  void set_roll_sensor(sensor::Sensor *roll_sensor) { roll_sensor_ = roll_sensor; }

  // Configuration setters
  void set_accel_range(AccelScale range) { accel_range_ = range; }
  void set_gyro_range(GyroScale range) { gyro_range_ = range; }
  void set_accel_odr(AccelODR odr) { accel_odr_ = odr; }
  void set_gyro_odr(GyroODR odr) { gyro_odr_ = odr; }
  void set_accel_lpf(LPFMode mode) { accel_lpf_ = mode; }
  void set_gyro_lpf(LPFMode mode) { gyro_lpf_ = mode; }

  // Accessors for current acceleration values (for binary/text sensors)
  float get_accel_x() const { return last_accel_x_; }
  float get_accel_y() const { return last_accel_y_; }
  float get_accel_z() const { return last_accel_z_; }

 protected:
  // Sensor pointers
  sensor::Sensor *accel_x_sensor_{nullptr};
  sensor::Sensor *accel_y_sensor_{nullptr};
  sensor::Sensor *accel_z_sensor_{nullptr};
  sensor::Sensor *gyro_x_sensor_{nullptr};
  sensor::Sensor *gyro_y_sensor_{nullptr};
  sensor::Sensor *gyro_z_sensor_{nullptr};
  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *pitch_sensor_{nullptr};
  sensor::Sensor *roll_sensor_{nullptr};

  // Configuration
  AccelScale accel_range_{ACCEL_SCALE_2G};
  GyroScale gyro_range_{GYRO_SCALE_2048DPS};
  AccelODR accel_odr_{ACCEL_ODR_500HZ};
  GyroODR gyro_odr_{GYRO_ODR_500HZ};
  LPFMode accel_lpf_{LPF_2_66PCT};
  LPFMode gyro_lpf_{LPF_2_66PCT};

  // Scaling factors based on configured ranges
  float accel_sensitivity_{16384.0f};  ///< Default: 2g scale (LSB/g)
  float gyro_sensitivity_{16.0f};      ///< Default: 2048 dps scale (LSB/dps)

  // Cached values for derived sensors
  float last_accel_x_{0.0f};
  float last_accel_y_{0.0f};
  float last_accel_z_{0.0f};

  // Helper methods
  float get_accel_sensitivity_for_range_(AccelScale range);
  float get_gyro_sensitivity_for_range_(GyroScale range);
};

/// Motion detection binary sensor - detects deviation from gravity
class QMI8658MotionBinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  void setup() override {}
  void loop() override;
  float get_setup_priority() const override { return setup_priority::DATA - 1.0f; }

  void set_parent(QMI8658Component *parent) { parent_ = parent; }
  void set_threshold(float threshold) { threshold_ = threshold; }

 protected:
  QMI8658Component *parent_{nullptr};
  float threshold_{0.5f};  ///< Motion threshold in m/s² deviation from gravity
  bool last_state_{false};

  static constexpr float GRAVITY_EARTH = 9.80665f;
};

/// Orientation detection text sensor - reports device orientation
class QMI8658OrientationTextSensor : public text_sensor::TextSensor, public Component {
 public:
  void setup() override {}
  void loop() override;
  float get_setup_priority() const override { return setup_priority::DATA - 1.0f; }

  void set_parent(QMI8658Component *parent) { parent_ = parent; }

 protected:
  QMI8658Component *parent_{nullptr};
  std::string last_orientation_{""};

  static constexpr float ORIENTATION_THRESHOLD = 7.0f;  ///< ~45 degree threshold in m/s²
};

/// Wake on Motion binary sensor - hardware-based motion detection via interrupt
class QMI8658WoMBinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::DATA - 1.0f; }

  void set_parent(QMI8658Component *parent) { parent_ = parent; }
  void set_threshold(uint8_t threshold) { threshold_ = threshold; }
  void set_interrupt_pin(InternalGPIOPin *pin) { interrupt_pin_ = pin; }
  void set_interrupt_number(uint8_t num) { interrupt_number_ = num; }

 protected:
  QMI8658Component *parent_{nullptr};
  InternalGPIOPin *interrupt_pin_{nullptr};
  uint8_t threshold_{100};  ///< WoM threshold in mg (1-255)
  uint8_t interrupt_number_{1};  ///< Which interrupt pin to use (1 or 2)
  bool last_state_{false};
  uint32_t motion_detected_time_{0};  ///< Timestamp when motion was detected

  bool configure_wom_();
  static void IRAM_ATTR gpio_interrupt_(QMI8658WoMBinarySensor *sensor);
  volatile bool interrupt_triggered_{false};

  static constexpr uint32_t MOTION_HOLD_TIME_MS = 500;  ///< Hold motion state for 500ms
};

}  // namespace qmi8658
}  // namespace esphome
