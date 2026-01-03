# Session Summary: 2026-01-03

## Issues Fixed

### 1. Build Error: GPIOPin has no member 'attach_interrupt'
**Root Cause:** The `attach_interrupt()` method only exists on `InternalGPIOPin`, not the base `GPIOPin` class.

**Fix:**
- `qmi8658.h`: Changed `GPIOPin*` to `InternalGPIOPin*` for interrupt pin (lines 195, 199)
- `binary_sensor.py`: Changed `pins.gpio_input_pin_schema` to `pins.internal_gpio_input_pin_schema` (line 47)

### 2. Build Error: Enum class qualification
**Root Cause:** ESPHome's codegen generates unscoped enum references, but C++ `enum class` requires scoped access.

**Fix:**
- `qmi8658.h`: Changed all `enum class` declarations to plain `enum` (lines 46-89)

## Features Added

### Low-Pass Filter (LPF) Configuration
Added hardware-based noise filtering via QMI8658's CTRL5 register.

**Files Modified:**
- `qmi8658.h`: Added `LPFMode` enum, setters, member variables
- `qmi8658.cpp`: Updated `setup()` to configure CTRL5, added LPF to `dump_config()`
- `__init__.py`: Added `LPF_MODE` mapping and configuration schema
- `example.yaml`: Added LPF options with documentation

**Configuration:**
```yaml
qmi8658:
  accel_lpf: 2.66%  # Options: DISABLED, 2.66%, 3.63%, 5.39%, 13.37%
  gyro_lpf: 2.66%   # Default: 2.66% (most smoothing)
```

**CTRL5 Register Bit Layout:**
- Bits 6:5 - gLPF_MODE (gyro filter mode)
- Bit 4 - gLPF_EN (gyro filter enable)
- Bits 2:1 - aLPF_MODE (accel filter mode)
- Bit 0 - aLPF_EN (accel filter enable)

### Example YAML: HA Logging Reduction
Updated `example.yaml` to demonstrate four approaches for reducing Home Assistant recorder overhead:

1. **internal: true** - Not exposed to HA (used for accel_x/y/z)
2. **throttle_average** - Average over period, send once (used for gyro)
3. **delta** - Only send on significant change (used for temperature)
4. **throttle + delta** - Combined filtering (used for pitch/roll)

## Technical Notes

- LPF bandwidth is expressed as % of ODR (Output Data Rate)
- At 500Hz ODR with 2.66% LPF, effective cutoff is ~13Hz
- Lower % = more aggressive filtering = smoother readings
- Default changed from DISABLED to 2.66% for better IoT experience

## Version 1.0.7 Changes (WoM Fixes)

### CTRL9 Command Completion Fix
- Replaced simple 10ms delay with proper polling of STATUSINT register
- Polls bit 7 (CmdDone) up to 100 times with 1ms delays
- Logs actual completion time for debugging
- Also polls for ACK completion after writing 0x00 to CTRL9

### Enhanced Debugging
- Added logging when software motion state changes (shows magnitude, deviation, threshold)
- Added "GPIO interrupt triggered" log when WoM interrupt fires
- Added register dump after WoM configuration (CTRL1, CTRL7, CAL1_L, CAL1_H)
- WoM motion detection now logs at INFO level for visibility

### Key Registers to Verify in Logs
After WoM setup, check:
- CTRL1: Should have bit 3 (INT1) or bit 4 (INT2) set, plus 0x40 for auto-increment
  - INT1 configured: CTRL1=0x48
  - INT2 configured: CTRL1=0x50
- CTRL7: Should be 0x03 (ACC_EN + GYR_EN)
- CAL1_L: Should match threshold in mg (default 100 = 0x64)
- CAL1_H: 0x04 for INT1, 0x44 for INT2 (includes 4-sample blanking time)

### Critical Datasheet Findings (Section 9 - Wake on Motion)

**WoM Mode Requirements (Table 31):**
- CTRL7: aEN=1, gEN=0, mEN=0 (accelerometer ONLY - gyro must be disabled!)
- CTRL2: aODR=11xx (low power mode recommended)

**Configuration Procedure (Figure 10):**
1. Disable sensors (CTRL7 = 0x00)
2. Set accelerometer sample rate and scale (CTRL2)
3. Set WoM threshold in CAL1_L; interrupt config in CAL1_H
4. Execute CTRL9 command (0x08)
5. Enable accelerometer ONLY in CTRL7 (not gyro!)

**CAL1_H Register (Table 33):**
- Bits 7:6: Interrupt select (00=INT1, 01=INT2, 10=INT1 init 1, 11=INT2 init 1)
- Bits 5:0: Blanking time (samples to ignore at startup - prevents spurious interrupts)

**Interrupt Behavior:**
- "For each WoM event, the state of the selected interrupt line is TOGGLED"
- Must use INTERRUPT_ANY_EDGE to catch all events
- Reading STATUS1 clears WoM flag and resets interrupt line

### Bugs Fixed in v1.0.7

1. **Gyroscope re-enabled (WRONG)** → Now only accelerometer enabled per datasheet
2. **Rising edge only** → Changed to ANY_EDGE for toggle behavior
3. **No blanking time** → Added 4-sample blanking to prevent startup glitches
4. **Fixed delay** → Proper STATUSINT polling for CTRL9 completion
5. **Immediate state clear** → Added 500ms hold time for HA visibility
