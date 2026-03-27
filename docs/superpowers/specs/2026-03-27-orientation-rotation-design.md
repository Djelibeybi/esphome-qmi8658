# Orientation Rotation Support

**Issue:** [#3 — Rotate the Orientation sensor reported values](https://github.com/Djelibeybi/esphome-qmi8658/issues/3)
**Date:** 2026-03-27

## Problem

When the QMI8658C is mounted in a non-default physical orientation (e.g. USB-C facing down instead of up), the orientation text sensor and automation trigger report incorrect values. The axis-to-orientation mapping assumes a fixed mounting position with no way to compensate.

## Solution

Add two optional configuration keys to the main `qmi8658:` component that transform the accelerometer axes before any downstream consumption.

### Configuration

```yaml
qmi8658:
  rotation: 0       # 0, 90, 180, 270 (degrees clockwise when viewed from above)
  invert_z: false   # swap face_up / face_down
```

- `rotation` — clockwise rotation in degrees when viewed from above. Accepts `0`, `90`, `180`, or `270`. Defaults to `0`.
- `invert_z` — inverts the Z axis, swapping face-up and face-down. Defaults to `false`.

### Transform Logic

The rotation is applied to the cached accelerometer values in `QMI8658Component::update()`, immediately after conversion to m/s² and before caching, publishing, or any downstream use.

| Config | Transform |
|--------|-----------|
| `rotation: 0` | no change |
| `rotation: 90` | `(ax, ay) -> (ay, -ax)` |
| `rotation: 180` | `(ax, ay) -> (-ax, -ay)` |
| `rotation: 270` | `(ax, ay) -> (-ay, ax)` |
| `invert_z: true` | `az -> -az` |

Rotation is applied first, then Z-invert. Both are independent.

### Scope of Effect

Because the transform is applied at the component level before caching, it affects all downstream consumers:

- Accel X/Y/Z sensor values
- Pitch/roll calculations
- Orientation text sensor
- `on_orientation_change` automation trigger
- Motion binary sensor (magnitude-based, so rotation is irrelevant, but stays consistent)

### Files Changed

1. **`__init__.py`** — Add `CONF_ROTATION` and `CONF_INVERT_Z` config keys with validation (`cv.one_of(0, 90, 180, 270)` and `cv.boolean`), plus codegen calls to `set_rotation()` and `set_invert_z()`.

2. **`qmi8658.h`** — Add `rotation_` (`uint16_t`, default `0`) and `invert_z_` (`bool`, default `false`) member variables with setter methods.

3. **`qmi8658.cpp`** — In `update()`, apply the axis transform after m/s² conversion and before caching/publishing. Log rotation config in `dump_config()`.

4. **`README.md`** — Document `rotation` and `invert_z` options in the Component Configuration section, the Configuration Variables section, and the Complete Example.

No changes to `text_sensor.py`, `binary_sensor.py`, or `sensor.py` — they consume already-transformed values from the parent component.
