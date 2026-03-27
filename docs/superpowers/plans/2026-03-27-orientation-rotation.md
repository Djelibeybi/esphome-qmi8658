# Orientation Rotation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `rotation` and `invert_z` configuration options to the QMI8658 component so users can compensate for non-default physical mounting orientations.

**Architecture:** Two new config keys on the main `qmi8658:` component transform accelerometer X/Y/Z values immediately after I2C read and m/s² conversion, before caching or publishing. All downstream consumers (sensors, orientation, triggers) automatically get corrected values.

**Tech Stack:** ESPHome external component (C++ for firmware, Python for config/codegen), compiled with `esphome compile`.

**Spec:** `docs/superpowers/specs/2026-03-27-orientation-rotation-design.md`

---

### Task 1: Add C++ member variables and setters

**Files:**
- Modify: `components/qmi8658/qmi8658.h:133` (after existing config setters)
- Modify: `components/qmi8658/qmi8658.h:166` (after existing config members)

- [ ] **Step 1: Add setter methods to the public section**

In `components/qmi8658/qmi8658.h`, after the line `void set_gyro_lpf(LPFMode mode) { gyro_lpf_ = mode; }` (line 133), add:

```cpp
  void set_rotation(uint16_t rotation) { rotation_ = rotation; }
  void set_invert_z(bool invert_z) { invert_z_ = invert_z; }
```

- [ ] **Step 2: Add member variables to the protected section**

In `components/qmi8658/qmi8658.h`, after the line `LPFMode gyro_lpf_{LPF_2_66PCT};` (line 166), add:

```cpp
  uint16_t rotation_{0};      ///< Mounting rotation: 0, 90, 180, 270 degrees clockwise
  bool invert_z_{false};      ///< Invert Z axis (swap face_up / face_down)
```

- [ ] **Step 3: Commit**

```bash
git add components/qmi8658/qmi8658.h
git commit -s --no-gpg-sign -m "feat: add rotation and invert_z member variables to QMI8658Component"
```

---

### Task 2: Apply axis transform in update()

**Files:**
- Modify: `components/qmi8658/qmi8658.cpp:169-174` (between m/s² conversion and caching)

- [ ] **Step 1: Add rotation transform after accel conversion**

In `components/qmi8658/qmi8658.cpp`, replace the block at lines 171-174:

```cpp
  // Cache accel values for derived sensors (motion detection, orientation)
  this->last_accel_x_ = accel_x;
  this->last_accel_y_ = accel_y;
  this->last_accel_z_ = accel_z;
```

with:

```cpp
  // Apply mounting rotation to accelerometer axes
  if (this->rotation_ == 90) {
    float tmp = accel_x;
    accel_x = accel_y;
    accel_y = -tmp;
  } else if (this->rotation_ == 180) {
    accel_x = -accel_x;
    accel_y = -accel_y;
  } else if (this->rotation_ == 270) {
    float tmp = accel_x;
    accel_x = -accel_y;
    accel_y = tmp;
  }

  // Apply Z-axis inversion (swap face_up / face_down)
  if (this->invert_z_) {
    accel_z = -accel_z;
  }

  // Cache accel values for derived sensors (motion detection, orientation)
  this->last_accel_x_ = accel_x;
  this->last_accel_y_ = accel_y;
  this->last_accel_z_ = accel_z;
```

Note: The transform is applied to the local `accel_x`, `accel_y`, `accel_z` variables *before* they are cached and *before* they are used for pitch/roll calculation and sensor publishing. This means pitch, roll, and all published accel values are also rotated.

- [ ] **Step 2: Commit**

```bash
git add components/qmi8658/qmi8658.cpp
git commit -s --no-gpg-sign -m "feat: apply mounting rotation transform in update()"
```

---

### Task 3: Log rotation config in dump_config()

**Files:**
- Modify: `components/qmi8658/qmi8658.cpp:274` (after gyroscope LPF log line)

- [ ] **Step 1: Add rotation logging**

In `components/qmi8658/qmi8658.cpp`, after the line `ESP_LOGCONFIG(TAG, "  Gyroscope LPF: %s", gyro_lpf_str);` (line 274), add:

```cpp
  if (this->rotation_ != 0) {
    ESP_LOGCONFIG(TAG, "  Mounting Rotation: %d°", this->rotation_);
  }
  if (this->invert_z_) {
    ESP_LOGCONFIG(TAG, "  Z-Axis Inverted: YES");
  }
```

- [ ] **Step 2: Commit**

```bash
git add components/qmi8658/qmi8658.cpp
git commit -s --no-gpg-sign -m "feat: log rotation and invert_z in dump_config()"
```

---

### Task 4: Add Python config keys and codegen

**Files:**
- Modify: `components/qmi8658/__init__.py:90-91` (add CONF constants)
- Modify: `components/qmi8658/__init__.py:101` (add to CONFIG_SCHEMA)
- Modify: `components/qmi8658/__init__.py:131` (add to to_code)

- [ ] **Step 1: Add config key constants**

In `components/qmi8658/__init__.py`, after the line `CONF_GYRO_LPF = "gyro_lpf"` (line 90), add:

```python
CONF_ROTATION = "rotation"
CONF_INVERT_Z = "invert_z"
```

- [ ] **Step 2: Add to CONFIG_SCHEMA**

In `components/qmi8658/__init__.py`, after the line `cv.Optional(CONF_GYRO_LPF, default="2.66%"): cv.enum(LPF_MODE, upper=True),` (line 101), add:

```python
            cv.Optional(CONF_ROTATION, default=0): cv.one_of(0, 90, 180, 270, int=True),
            cv.Optional(CONF_INVERT_Z, default=False): cv.boolean,
```

- [ ] **Step 3: Add to to_code()**

In `components/qmi8658/__init__.py`, after the line `cg.add(var.set_gyro_lpf(config[CONF_GYRO_LPF]))` (line 131), add:

```python
    cg.add(var.set_rotation(config[CONF_ROTATION]))
    cg.add(var.set_invert_z(config[CONF_INVERT_Z]))
```

- [ ] **Step 4: Commit**

```bash
git add components/qmi8658/__init__.py
git commit -s --no-gpg-sign -m "feat: add rotation and invert_z config options to Python codegen"
```

---

### Task 5: Compile test

**Files:**
- Modify: `example.yaml:58` (add rotation config for compile test)

- [ ] **Step 1: Add rotation config to example.yaml**

In `example.yaml`, after the line `gyro_lpf: 2.66%        # Options: DISABLED, 2.66%, 3.63%, 5.39%, 13.37% (default: 2.66%)` (line 58), add:

```yaml
  # Mounting orientation correction (for rotated/inverted sensor mounting)
  rotation: 0            # Options: 0, 90, 180, 270 (degrees clockwise viewed from above)
  invert_z: false        # Set to true to swap face_up/face_down
```

- [ ] **Step 2: Run compile test with default values (0, false)**

Run: `esphome compile example.yaml`

Expected: Compilation succeeds with no errors.

- [ ] **Step 3: Test with non-default rotation value**

Temporarily change `rotation: 0` to `rotation: 90` and `invert_z: false` to `invert_z: true` in `example.yaml`, then:

Run: `esphome compile example.yaml`

Expected: Compilation succeeds with no errors.

- [ ] **Step 4: Revert example.yaml to defaults**

Change back to `rotation: 0` and `invert_z: false`.

- [ ] **Step 5: Commit**

```bash
git add example.yaml
git commit -s --no-gpg-sign -m "feat: add rotation and invert_z to example config"
```

---

### Task 6: Update README.md

**Files:**
- Modify: `README.md:79-84` (Component Configuration YAML example)
- Modify: `README.md:176` (Configuration Variables section)
- Modify: `README.md:306-313` (Complete Example)

- [ ] **Step 1: Add to Component Configuration YAML example**

In `README.md`, after line 83 (`gyro_odr: 500HZ`), add:

```yaml

  # Mounting orientation correction (optional)
  rotation: 0            # Options: 0, 90, 180, 270 (degrees clockwise viewed from above)
  invert_z: false        # Swap face_up/face_down (default: false)
```

- [ ] **Step 2: Add to Configuration Variables section**

In `README.md`, after the `update_interval` entry in the General Options section (line 175), add a new subsection:

```markdown

#### Mounting Orientation

- **rotation** (_Optional_, int): Clockwise rotation in degrees when viewed from above, to compensate for sensor mounting orientation. Options: `0`, `90`, `180`, `270`. Default is `0`.
- **invert_z** (_Optional_, boolean): Invert the Z axis to swap face-up and face-down detection. Default is `false`.
```

- [ ] **Step 3: Add to Complete Example**

In `README.md`, in the Complete Example `qmi8658:` block, after line 313 (`gyro_odr: 500HZ`), add:

```yaml
  rotation: 90          # Compensate for 90° rotated mounting
  invert_z: true        # USB port faces down, so invert Z
```

- [ ] **Step 4: Commit**

```bash
git add README.md
git commit -s --no-gpg-sign -m "docs: document rotation and invert_z options in README"
```

---

### Task 7: Final compile verification

- [ ] **Step 1: Run full compile**

Run: `esphome compile example.yaml`

Expected: Clean compilation with no errors or warnings related to rotation/invert_z.

- [ ] **Step 2: Verify dump_config output mentions rotation**

Check that the generated C++ code includes the rotation configuration logging by searching the compiled output:

Run: `grep -r "set_rotation\|set_invert_z" .esphome/ 2>/dev/null | head -5`

Expected: Lines showing `set_rotation(0)` and `set_invert_z(false)` in the generated code.
