#include "qmi8658.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <cmath>

namespace esphome {
namespace qmi8658 {

static const char *const TAG = "qmi8658";

/// Standard gravity constant for acceleration conversion
static const float GRAVITY_EARTH = 9.80665f;

/// Radians to degrees conversion factor
static const float RAD_TO_DEG = 57.295779513f;

float QMI8658Component::get_accel_sensitivity_for_range_(AccelScale range) {
  switch (range) {
    case AccelScale::ACCEL_SCALE_2G:
      return 16384.0f;  // LSB/g
    case AccelScale::ACCEL_SCALE_4G:
      return 8192.0f;
    case AccelScale::ACCEL_SCALE_8G:
      return 4096.0f;
    case AccelScale::ACCEL_SCALE_16G:
      return 2048.0f;
    default:
      return 16384.0f;
  }
}

float QMI8658Component::get_gyro_sensitivity_for_range_(GyroScale range) {
  switch (range) {
    case GyroScale::GYRO_SCALE_16DPS:
      return 2048.0f;  // LSB/dps
    case GyroScale::GYRO_SCALE_32DPS:
      return 1024.0f;
    case GyroScale::GYRO_SCALE_64DPS:
      return 512.0f;
    case GyroScale::GYRO_SCALE_128DPS:
      return 256.0f;
    case GyroScale::GYRO_SCALE_256DPS:
      return 128.0f;
    case GyroScale::GYRO_SCALE_512DPS:
      return 64.0f;
    case GyroScale::GYRO_SCALE_1024DPS:
      return 32.0f;
    case GyroScale::GYRO_SCALE_2048DPS:
      return 16.0f;
    default:
      return 16.0f;
  }
}

void QMI8658Component::setup() {
  ESP_LOGI(TAG, "QMI8658 component version %s", QMI8658_VERSION);
  ESP_LOGI(TAG, "Setting up QMI8658...");

  // Soft reset the device
  if (!this->write_byte(QMI8658_REG_RESET, 0xB0)) {
    ESP_LOGE(TAG, "Failed to reset QMI8658");
    this->mark_failed();
    return;
  }
  delay(10);  // Wait for reset to complete

  // Verify device ID
  uint8_t chip_id;
  if (!this->read_byte(QMI8658_REG_WHO_AM_I, &chip_id)) {
    ESP_LOGE(TAG, "Failed to read WHO_AM_I register");
    this->mark_failed();
    return;
  }

  if (chip_id != QMI8658_CHIP_ID) {
    ESP_LOGE(TAG, "Invalid chip ID: 0x%02X (expected 0x%02X)", chip_id, QMI8658_CHIP_ID);
    this->mark_failed();
    return;
  }

  ESP_LOGD(TAG, "Chip ID: 0x%02X", chip_id);

  // Read revision for debugging
  uint8_t revision;
  if (this->read_byte(QMI8658_REG_REVISION, &revision)) {
    ESP_LOGD(TAG, "Revision: 0x%02X", revision);
  }

  // Configure CTRL1: Address auto-increment enabled
  if (!this->write_byte(QMI8658_REG_CTRL1, 0x40)) {
    ESP_LOGE(TAG, "Failed to configure CTRL1");
    this->mark_failed();
    return;
  }

  // Configure accelerometer: scale and ODR from config
  // CTRL2: [7:6] reserved, [6:4] scale, [3:0] ODR
  uint8_t ctrl2 = static_cast<uint8_t>(this->accel_range_) | static_cast<uint8_t>(this->accel_odr_);
  if (!this->write_byte(QMI8658_REG_CTRL2, ctrl2)) {
    ESP_LOGE(TAG, "Failed to configure accelerometer");
    this->mark_failed();
    return;
  }
  this->accel_sensitivity_ = this->get_accel_sensitivity_for_range_(this->accel_range_);

  // Configure gyroscope: scale and ODR from config
  // CTRL3: [7] reserved, [6:4] scale, [3:0] ODR
  uint8_t ctrl3 = static_cast<uint8_t>(this->gyro_range_) | static_cast<uint8_t>(this->gyro_odr_);
  if (!this->write_byte(QMI8658_REG_CTRL3, ctrl3)) {
    ESP_LOGE(TAG, "Failed to configure gyroscope");
    this->mark_failed();
    return;
  }
  this->gyro_sensitivity_ = this->get_gyro_sensitivity_for_range_(this->gyro_range_);

  // Configure CTRL5: Low-pass filter settings
  // Bit layout: [7:reserved][6:5 gLPF_MODE][4 gLPF_EN][3:reserved][2:1 aLPF_MODE][0 aLPF_EN]
  uint8_t ctrl5 = 0x00;
  if (this->accel_lpf_ != LPF_DISABLED) {
    uint8_t mode = static_cast<uint8_t>(this->accel_lpf_) - 1;  // Convert to 0-3 range
    ctrl5 |= 0x01;         // aLPF_EN
    ctrl5 |= (mode << 1);  // aLPF_MODE
  }
  if (this->gyro_lpf_ != LPF_DISABLED) {
    uint8_t mode = static_cast<uint8_t>(this->gyro_lpf_) - 1;  // Convert to 0-3 range
    ctrl5 |= 0x10;         // gLPF_EN
    ctrl5 |= (mode << 5);  // gLPF_MODE
  }
  if (!this->write_byte(QMI8658_REG_CTRL5, ctrl5)) {
    ESP_LOGE(TAG, "Failed to configure CTRL5");
    this->mark_failed();
    return;
  }

  // Enable accelerometer and gyroscope
  uint8_t ctrl7 = QMI8658_CTRL7_ACC_EN | QMI8658_CTRL7_GYR_EN;
  if (!this->write_byte(QMI8658_REG_CTRL7, ctrl7)) {
    ESP_LOGE(TAG, "Failed to enable sensors");
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "QMI8658 initialized successfully");
}

void QMI8658Component::update() {
  // Read all sensor data in one burst read (14 bytes: temp + accel + gyro)
  // Starting from TEMP_L (0x33): 2 bytes temp, 6 bytes accel, 6 bytes gyro
  uint8_t data[14];
  if (!this->read_bytes(QMI8658_REG_TEMP_L, data, 14)) {
    ESP_LOGW(TAG, "Failed to read sensor data");
    this->status_set_warning();
    return;
  }
  this->status_clear_warning();

  // Parse temperature (registers 0x33-0x34)
  int16_t raw_temp = (static_cast<int16_t>(data[1]) << 8) | data[0];
  float temperature = static_cast<float>(raw_temp) / 256.0f;

  // Parse accelerometer data (registers 0x35-0x3A)
  int16_t raw_accel_x = (static_cast<int16_t>(data[3]) << 8) | data[2];
  int16_t raw_accel_y = (static_cast<int16_t>(data[5]) << 8) | data[4];
  int16_t raw_accel_z = (static_cast<int16_t>(data[7]) << 8) | data[6];

  // Convert to m/s^2
  float accel_x = (static_cast<float>(raw_accel_x) / this->accel_sensitivity_) * GRAVITY_EARTH;
  float accel_y = (static_cast<float>(raw_accel_y) / this->accel_sensitivity_) * GRAVITY_EARTH;
  float accel_z = (static_cast<float>(raw_accel_z) / this->accel_sensitivity_) * GRAVITY_EARTH;

  // Cache accel values for derived sensors (motion detection, orientation)
  this->last_accel_x_ = accel_x;
  this->last_accel_y_ = accel_y;
  this->last_accel_z_ = accel_z;

  // Parse gyroscope data (registers 0x3B-0x40)
  int16_t raw_gyro_x = (static_cast<int16_t>(data[9]) << 8) | data[8];
  int16_t raw_gyro_y = (static_cast<int16_t>(data[11]) << 8) | data[10];
  int16_t raw_gyro_z = (static_cast<int16_t>(data[13]) << 8) | data[12];

  // Convert to degrees per second
  float gyro_x = static_cast<float>(raw_gyro_x) / this->gyro_sensitivity_;
  float gyro_y = static_cast<float>(raw_gyro_y) / this->gyro_sensitivity_;
  float gyro_z = static_cast<float>(raw_gyro_z) / this->gyro_sensitivity_;

  // Calculate pitch and roll from accelerometer (gravity vector)
  // Pitch: rotation around Y axis (nose up/down)
  // Roll: rotation around X axis (bank left/right)
  float pitch = std::atan2(accel_x, std::sqrt(accel_y * accel_y + accel_z * accel_z)) * RAD_TO_DEG;
  float roll = std::atan2(accel_y, std::sqrt(accel_x * accel_x + accel_z * accel_z)) * RAD_TO_DEG;

  ESP_LOGV(TAG, "Accel: X=%.2f Y=%.2f Z=%.2f m/s^2", accel_x, accel_y, accel_z);
  ESP_LOGV(TAG, "Gyro: X=%.2f Y=%.2f Z=%.2f dps", gyro_x, gyro_y, gyro_z);
  ESP_LOGV(TAG, "Pitch: %.1f Roll: %.1f deg", pitch, roll);
  ESP_LOGV(TAG, "Temp: %.1f C", temperature);

  // Publish sensor values
  if (this->accel_x_sensor_ != nullptr)
    this->accel_x_sensor_->publish_state(accel_x);
  if (this->accel_y_sensor_ != nullptr)
    this->accel_y_sensor_->publish_state(accel_y);
  if (this->accel_z_sensor_ != nullptr)
    this->accel_z_sensor_->publish_state(accel_z);

  if (this->gyro_x_sensor_ != nullptr)
    this->gyro_x_sensor_->publish_state(gyro_x);
  if (this->gyro_y_sensor_ != nullptr)
    this->gyro_y_sensor_->publish_state(gyro_y);
  if (this->gyro_z_sensor_ != nullptr)
    this->gyro_z_sensor_->publish_state(gyro_z);

  if (this->temperature_sensor_ != nullptr)
    this->temperature_sensor_->publish_state(temperature);

  if (this->pitch_sensor_ != nullptr)
    this->pitch_sensor_->publish_state(pitch);
  if (this->roll_sensor_ != nullptr)
    this->roll_sensor_->publish_state(roll);
}

void QMI8658Component::dump_config() {
  ESP_LOGCONFIG(TAG, "QMI8658:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with QMI8658 failed!");
  }

  // Log configuration
  const char *accel_range_str;
  switch (this->accel_range_) {
    case AccelScale::ACCEL_SCALE_2G: accel_range_str = "2G"; break;
    case AccelScale::ACCEL_SCALE_4G: accel_range_str = "4G"; break;
    case AccelScale::ACCEL_SCALE_8G: accel_range_str = "8G"; break;
    case AccelScale::ACCEL_SCALE_16G: accel_range_str = "16G"; break;
    default: accel_range_str = "Unknown"; break;
  }

  const char *gyro_range_str;
  switch (this->gyro_range_) {
    case GyroScale::GYRO_SCALE_16DPS: gyro_range_str = "16 DPS"; break;
    case GyroScale::GYRO_SCALE_32DPS: gyro_range_str = "32 DPS"; break;
    case GyroScale::GYRO_SCALE_64DPS: gyro_range_str = "64 DPS"; break;
    case GyroScale::GYRO_SCALE_128DPS: gyro_range_str = "128 DPS"; break;
    case GyroScale::GYRO_SCALE_256DPS: gyro_range_str = "256 DPS"; break;
    case GyroScale::GYRO_SCALE_512DPS: gyro_range_str = "512 DPS"; break;
    case GyroScale::GYRO_SCALE_1024DPS: gyro_range_str = "1024 DPS"; break;
    case GyroScale::GYRO_SCALE_2048DPS: gyro_range_str = "2048 DPS"; break;
    default: gyro_range_str = "Unknown"; break;
  }

  const char *accel_lpf_str;
  switch (this->accel_lpf_) {
    case LPF_DISABLED: accel_lpf_str = "Disabled"; break;
    case LPF_2_66PCT: accel_lpf_str = "2.66% of ODR"; break;
    case LPF_3_63PCT: accel_lpf_str = "3.63% of ODR"; break;
    case LPF_5_39PCT: accel_lpf_str = "5.39% of ODR"; break;
    case LPF_13_37PCT: accel_lpf_str = "13.37% of ODR"; break;
    default: accel_lpf_str = "Unknown"; break;
  }

  const char *gyro_lpf_str;
  switch (this->gyro_lpf_) {
    case LPF_DISABLED: gyro_lpf_str = "Disabled"; break;
    case LPF_2_66PCT: gyro_lpf_str = "2.66% of ODR"; break;
    case LPF_3_63PCT: gyro_lpf_str = "3.63% of ODR"; break;
    case LPF_5_39PCT: gyro_lpf_str = "5.39% of ODR"; break;
    case LPF_13_37PCT: gyro_lpf_str = "13.37% of ODR"; break;
    default: gyro_lpf_str = "Unknown"; break;
  }

  ESP_LOGCONFIG(TAG, "  Accelerometer Range: %s", accel_range_str);
  ESP_LOGCONFIG(TAG, "  Accelerometer LPF: %s", accel_lpf_str);
  ESP_LOGCONFIG(TAG, "  Gyroscope Range: %s", gyro_range_str);
  ESP_LOGCONFIG(TAG, "  Gyroscope LPF: %s", gyro_lpf_str);

  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Acceleration X", this->accel_x_sensor_);
  LOG_SENSOR("  ", "Acceleration Y", this->accel_y_sensor_);
  LOG_SENSOR("  ", "Acceleration Z", this->accel_z_sensor_);
  LOG_SENSOR("  ", "Gyroscope X", this->gyro_x_sensor_);
  LOG_SENSOR("  ", "Gyroscope Y", this->gyro_y_sensor_);
  LOG_SENSOR("  ", "Gyroscope Z", this->gyro_z_sensor_);
  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
  LOG_SENSOR("  ", "Pitch", this->pitch_sensor_);
  LOG_SENSOR("  ", "Roll", this->roll_sensor_);
}

// Motion Binary Sensor implementation
void QMI8658MotionBinarySensor::loop() {
  if (this->parent_ == nullptr)
    return;

  // Get current acceleration values
  float ax = this->parent_->get_accel_x();
  float ay = this->parent_->get_accel_y();
  float az = this->parent_->get_accel_z();

  // Calculate magnitude of acceleration vector
  float magnitude = std::sqrt(ax * ax + ay * ay + az * az);

  // Calculate deviation from expected gravity
  float deviation = std::abs(magnitude - GRAVITY_EARTH);

  // Motion detected if deviation exceeds threshold
  bool motion = deviation > this->threshold_;

  // Only publish if state changed (with small hysteresis)
  if (motion != this->last_state_) {
    ESP_LOGV(TAG, "Motion %s: magnitude=%.2f m/s², deviation=%.2f, threshold=%.2f",
             motion ? "DETECTED" : "cleared", magnitude, deviation, this->threshold_);
    this->last_state_ = motion;
    this->publish_state(motion);
  }
}

// Orientation Text Sensor implementation
void QMI8658OrientationTextSensor::loop() {
  if (this->parent_ == nullptr)
    return;

  // Get current acceleration values
  float ax = this->parent_->get_accel_x();
  float ay = this->parent_->get_accel_y();
  float az = this->parent_->get_accel_z();

  // Determine orientation based on which axis has the dominant gravity component
  std::string orientation;

  if (az > ORIENTATION_THRESHOLD) {
    orientation = "face_up";
  } else if (az < -ORIENTATION_THRESHOLD) {
    orientation = "face_down";
  } else if (ax > ORIENTATION_THRESHOLD) {
    orientation = "portrait";
  } else if (ax < -ORIENTATION_THRESHOLD) {
    orientation = "portrait_inverted";
  } else if (ay > ORIENTATION_THRESHOLD) {
    orientation = "landscape_right";
  } else if (ay < -ORIENTATION_THRESHOLD) {
    orientation = "landscape_left";
  } else {
    orientation = "unknown";
  }

  // Only publish if orientation changed
  if (orientation != this->last_orientation_) {
    this->last_orientation_ = orientation;
    this->publish_state(orientation);
  }
}

// Wake on Motion Binary Sensor implementation
void IRAM_ATTR QMI8658WoMBinarySensor::gpio_interrupt_(QMI8658WoMBinarySensor *sensor) {
  sensor->interrupt_triggered_ = true;
}

void QMI8658WoMBinarySensor::setup() {
  if (this->parent_ == nullptr || this->interrupt_pin_ == nullptr) {
    ESP_LOGE(TAG, "WoM sensor not properly configured");
    this->mark_failed();
    return;
  }

  // Configure the interrupt pin
  // Per datasheet: "For each WoM event, the state of the selected interrupt line is toggled"
  // We need to catch both edges to detect all motion events
  this->interrupt_pin_->setup();
  this->interrupt_pin_->attach_interrupt(
      QMI8658WoMBinarySensor::gpio_interrupt_, this, gpio::INTERRUPT_ANY_EDGE);

  // Configure Wake on Motion on the sensor
  if (!this->configure_wom_()) {
    ESP_LOGE(TAG, "Failed to configure Wake on Motion");
    this->mark_failed();
    return;
  }

  // Read back and log key registers for debugging
  uint8_t ctrl1_val, ctrl7_val, cal1_l_val, cal1_h_val;
  this->parent_->read_byte(QMI8658_REG_CTRL1, &ctrl1_val);
  this->parent_->read_byte(QMI8658_REG_CTRL7, &ctrl7_val);
  this->parent_->read_byte(QMI8658_REG_CAL1_L, &cal1_l_val);
  this->parent_->read_byte(QMI8658_REG_CAL1_H, &cal1_h_val);
  ESP_LOGI(TAG, "WoM configured: threshold=%d mg, INT%d, blanking=%d samples",
           this->threshold_, this->interrupt_number_, cal1_h_val & 0x3F);
  ESP_LOGD(TAG, "WoM registers: CTRL1=0x%02X CTRL7=0x%02X CAL1_L=0x%02X CAL1_H=0x%02X",
           ctrl1_val, ctrl7_val, cal1_l_val, cal1_h_val);
  ESP_LOGW(TAG, "WoM mode active: Gyroscope disabled per datasheet requirements");

  // Publish initial state so Home Assistant doesn't show "Unknown"
  this->publish_state(false);
}

bool QMI8658WoMBinarySensor::configure_wom_() {
  // WoM Configuration Procedure (per datasheet):
  // 1. Disable sensors (CTRL7 = 0x00)
  // 2. Set accelerometer sample rate and scale (CTRL2) - already done in main setup
  // 3. Set WoM threshold (CAL1_L) and interrupt config (CAL1_H)
  // 4. Execute CTRL9 command
  // 5. Re-enable accelerometer (CTRL7)

  ESP_LOGV(TAG, "Configuring WoM with INT%d", this->interrupt_number_);

  // Step 1: Disable sensors
  if (!this->parent_->write_byte(QMI8658_REG_CTRL7, 0x00)) {
    return false;
  }

  // Step 3a: WoM threshold in CAL1_L register, 1mg per LSB
  if (!this->parent_->write_byte(QMI8658_REG_CAL1_L, this->threshold_)) {
    return false;
  }

  // Step 3b: CAL1_H interrupt config (per datasheet Table 33):
  // Bits 7:6 - Interrupt select:
  //   00 = INT1 (initial value 0)
  //   10 = INT1 (initial value 1)
  //   01 = INT2 (initial value 0)
  //   11 = INT2 (initial value 1)
  // Bits 5:0 - Blanking time (number of accel samples to ignore at startup)
  //   Setting to 4 samples prevents spurious interrupts from startup transients
  uint8_t cal1_h = (this->interrupt_number_ == 2) ? 0x44 : 0x04;  // INT2 or INT1, with 4 sample blanking
  if (!this->parent_->write_byte(QMI8658_REG_CAL1_H, cal1_h)) {
    return false;
  }

  // Enable interrupt physical output in CTRL1
  // Read current CTRL1 value (preserves address auto-increment setting)
  uint8_t ctrl1;
  if (!this->parent_->read_byte(QMI8658_REG_CTRL1, &ctrl1)) {
    return false;
  }
  // Bit 3 = INT1 output enable, Bit 4 = INT2 output enable
  if (this->interrupt_number_ == 2) {
    ctrl1 |= 0x10;  // Enable INT2 output (push-pull mode)
  } else {
    ctrl1 |= 0x08;  // Enable INT1 output (push-pull mode)
  }
  if (!this->parent_->write_byte(QMI8658_REG_CTRL1, ctrl1)) {
    return false;
  }

  // Step 4: Send CTRL9 command to configure WoM
  if (!this->parent_->write_byte(QMI8658_REG_CTRL9, QMI8658_CTRL9_CMD_WOM)) {
    return false;
  }

  // Poll STATUS1 for command completion (bit 0 = CMD_DONE)
  // Per datasheet: after writing CTRL9, poll until STATUSINT shows command done
  uint8_t status = 0;
  uint8_t statusint = 0;
  int retries = 0;
  const int max_retries = 100;  // 100 * 1ms = 100ms max wait

  // First check STATUSINT for command acknowledgment (bit 7 = CmdDone)
  do {
    delay(1);
    if (!this->parent_->read_byte(QMI8658_REG_STATUSINT, &statusint)) {
      ESP_LOGW(TAG, "WoM: Failed to read STATUSINT");
      return false;
    }
    retries++;
  } while (((statusint & 0x80) == 0) && (retries < max_retries));

  if (retries >= max_retries) {
    ESP_LOGW(TAG, "WoM: CTRL9 command timeout (STATUSINT=0x%02X after %d retries)", statusint, retries);
  } else {
    ESP_LOGV(TAG, "WoM: CTRL9 command done after %d ms (STATUSINT=0x%02X)", retries, statusint);
  }

  // Read STATUS1 to check WoM status and clear any pending flags
  if (!this->parent_->read_byte(QMI8658_REG_STATUS1, &status)) {
    return false;
  }
  ESP_LOGV(TAG, "WoM STATUS1=0x%02X after command", status);

  // Acknowledge the CTRL9 command (required per datasheet)
  if (!this->parent_->write_byte(QMI8658_REG_CTRL9, QMI8658_CTRL9_CMD_ACK)) {
    return false;
  }

  // Wait for acknowledge to be processed
  retries = 0;
  do {
    delay(1);
    if (!this->parent_->read_byte(QMI8658_REG_STATUSINT, &statusint)) {
      return false;
    }
    retries++;
  } while (((statusint & 0x80) != 0) && (retries < max_retries));

  ESP_LOGV(TAG, "WoM: CTRL9 ACK processed after %d ms", retries);

  // Step 5: Enable ONLY accelerometer for WoM mode (per datasheet Table 31)
  // WoM mode requires: CTRL7 aEN=1, gEN=0, mEN=0
  // Note: Gyroscope readings will not update while WoM is active
  if (!this->parent_->write_byte(QMI8658_REG_CTRL7, QMI8658_CTRL7_ACC_EN)) {
    return false;
  }

  return true;
}

void QMI8658WoMBinarySensor::loop() {
  if (this->interrupt_triggered_) {
    this->interrupt_triggered_ = false;
    ESP_LOGV(TAG, "WoM: GPIO interrupt (edge detected)");

    // Read STATUS1 to check WoM event
    // Per datasheet: bit 2 is the WoM flag, reading STATUS1 clears it and resets INT pin
    uint8_t status;
    if (this->parent_->read_byte(QMI8658_REG_STATUS1, &status)) {
      bool wom_event = (status & 0x04) != 0;
      if (wom_event) {
        ESP_LOGI(TAG, "WoM: Motion detected! (STATUS1=0x%02X)", status);
        this->publish_state(true);
        this->last_state_ = true;
        this->motion_detected_time_ = millis();
      } else {
        // Interrupt fired but WoM flag not set - this can happen because:
        // 1. Interrupt toggles on each event (we catch both edges)
        // 2. Reading STATUS1 cleared the flag before we read it
        ESP_LOGV(TAG, "WoM: Edge detected, WoM flag not set (STATUS1=0x%02X)", status);
      }
    } else {
      ESP_LOGW(TAG, "WoM: Failed to read STATUS1");
    }
  }

  // Auto-clear motion state after hold time
  // This gives Home Assistant time to register the motion event
  if (this->last_state_ && (millis() - this->motion_detected_time_ >= MOTION_HOLD_TIME_MS)) {
    ESP_LOGV(TAG, "WoM: Motion cleared (hold time elapsed)");
    this->publish_state(false);
    this->last_state_ = false;
  }
}

}  // namespace qmi8658
}  // namespace esphome
