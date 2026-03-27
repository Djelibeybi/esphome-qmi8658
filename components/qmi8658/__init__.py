import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import i2c
from esphome.const import CONF_ID, CONF_TRIGGER_ID, CONF_THRESHOLD

CODEOWNERS = ["@Djelibeybi"]
DEPENDENCIES = ["i2c"]
MULTI_CONF = True

CONF_QMI8658_ID = "qmi8658_id"

qmi8658_ns = cg.esphome_ns.namespace("qmi8658")
QMI8658Component = qmi8658_ns.class_(
    "QMI8658Component", cg.PollingComponent, i2c.I2CDevice
)

# Enums need to be defined here so they can be shared across platforms
AccelScale = qmi8658_ns.enum("AccelScale")
GyroScale = qmi8658_ns.enum("GyroScale")
AccelODR = qmi8658_ns.enum("AccelODR")
GyroODR = qmi8658_ns.enum("GyroODR")
LPFMode = qmi8658_ns.enum("LPFMode")

ACCEL_RANGE = {
    "2G": AccelScale.ACCEL_SCALE_2G,
    "4G": AccelScale.ACCEL_SCALE_4G,
    "8G": AccelScale.ACCEL_SCALE_8G,
    "16G": AccelScale.ACCEL_SCALE_16G,
}

GYRO_RANGE = {
    "16DPS": GyroScale.GYRO_SCALE_16DPS,
    "32DPS": GyroScale.GYRO_SCALE_32DPS,
    "64DPS": GyroScale.GYRO_SCALE_64DPS,
    "128DPS": GyroScale.GYRO_SCALE_128DPS,
    "256DPS": GyroScale.GYRO_SCALE_256DPS,
    "512DPS": GyroScale.GYRO_SCALE_512DPS,
    "1024DPS": GyroScale.GYRO_SCALE_1024DPS,
    "2048DPS": GyroScale.GYRO_SCALE_2048DPS,
}

ACCEL_ODR = {
    "8000HZ": AccelODR.ACCEL_ODR_8000HZ,
    "4000HZ": AccelODR.ACCEL_ODR_4000HZ,
    "2000HZ": AccelODR.ACCEL_ODR_2000HZ,
    "1000HZ": AccelODR.ACCEL_ODR_1000HZ,
    "500HZ": AccelODR.ACCEL_ODR_500HZ,
    "250HZ": AccelODR.ACCEL_ODR_250HZ,
    "125HZ": AccelODR.ACCEL_ODR_125HZ,
    "62.5HZ": AccelODR.ACCEL_ODR_62_5HZ,
    "31.25HZ": AccelODR.ACCEL_ODR_31_25HZ,
}

GYRO_ODR = {
    "8000HZ": GyroODR.GYRO_ODR_8000HZ,
    "4000HZ": GyroODR.GYRO_ODR_4000HZ,
    "2000HZ": GyroODR.GYRO_ODR_2000HZ,
    "1000HZ": GyroODR.GYRO_ODR_1000HZ,
    "500HZ": GyroODR.GYRO_ODR_500HZ,
    "250HZ": GyroODR.GYRO_ODR_250HZ,
    "125HZ": GyroODR.GYRO_ODR_125HZ,
    "62.5HZ": GyroODR.GYRO_ODR_62_5HZ,
    "31.25HZ": GyroODR.GYRO_ODR_31_25HZ,
}

LPF_MODE = {
    "DISABLED": LPFMode.LPF_DISABLED,
    "2.66%": LPFMode.LPF_2_66PCT,
    "3.63%": LPFMode.LPF_3_63PCT,
    "5.39%": LPFMode.LPF_5_39PCT,
    "13.37%": LPFMode.LPF_13_37PCT,
}

# Automation trigger constants
CONF_ON_MOTION = "on_motion"
CONF_ON_ORIENTATION_CHANGE = "on_orientation_change"

# Trigger classes for automations
OnMotionTrigger = qmi8658_ns.class_("OnMotionTrigger", automation.Trigger.template())
OnOrientationChangeTrigger = qmi8658_ns.class_(
    "OnOrientationChangeTrigger", automation.Trigger.template(cg.std_string)
)

CONF_ACCEL_RANGE = "accel_range"
CONF_GYRO_RANGE = "gyro_range"
CONF_ACCEL_ODR = "accel_odr"
CONF_GYRO_ODR = "gyro_odr"
CONF_ACCEL_LPF = "accel_lpf"
CONF_GYRO_LPF = "gyro_lpf"
CONF_ROTATION = "rotation"
CONF_INVERT_Z = "invert_z"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(QMI8658Component),
            cv.Optional(CONF_ACCEL_RANGE, default="2G"): cv.enum(ACCEL_RANGE, upper=True),
            cv.Optional(CONF_GYRO_RANGE, default="2048DPS"): cv.enum(GYRO_RANGE, upper=True),
            cv.Optional(CONF_ACCEL_ODR, default="500HZ"): cv.enum(ACCEL_ODR, upper=True),
            cv.Optional(CONF_GYRO_ODR, default="500HZ"): cv.enum(GYRO_ODR, upper=True),
            cv.Optional(CONF_ACCEL_LPF, default="2.66%"): cv.enum(LPF_MODE, upper=True),
            cv.Optional(CONF_GYRO_LPF, default="2.66%"): cv.enum(LPF_MODE, upper=True),
            cv.Optional(CONF_ROTATION, default=0): cv.one_of(0, 90, 180, 270, int=True),
            cv.Optional(CONF_INVERT_Z, default=False): cv.boolean,
            # Automation triggers
            cv.Optional(CONF_ON_MOTION): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(OnMotionTrigger),
                    cv.Optional(CONF_THRESHOLD, default=0.5): cv.float_range(min=0.1, max=10.0),
                }
            ),
            cv.Optional(CONF_ON_ORIENTATION_CHANGE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(OnOrientationChangeTrigger),
                }
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x6B))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    cg.add(var.set_accel_range(config[CONF_ACCEL_RANGE]))
    cg.add(var.set_gyro_range(config[CONF_GYRO_RANGE]))
    cg.add(var.set_accel_odr(config[CONF_ACCEL_ODR]))
    cg.add(var.set_gyro_odr(config[CONF_GYRO_ODR]))
    cg.add(var.set_accel_lpf(config[CONF_ACCEL_LPF]))
    cg.add(var.set_gyro_lpf(config[CONF_GYRO_LPF]))
    cg.add(var.set_rotation(config[CONF_ROTATION]))
    cg.add(var.set_invert_z(config[CONF_INVERT_Z]))

    # Handle on_motion automation triggers
    for conf in config.get(CONF_ON_MOTION, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)
        cg.add(var.set_motion_threshold(conf[CONF_THRESHOLD]))

    # Handle on_orientation_change automation triggers
    for conf in config.get(CONF_ON_ORIENTATION_CHANGE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_string, "orientation")], conf)
        cg.add(var.set_orientation_detection_enabled(True))
