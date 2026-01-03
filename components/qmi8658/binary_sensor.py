import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, gpio
from esphome import pins
from esphome.const import (
    CONF_ID,
    CONF_PIN,
    DEVICE_CLASS_MOTION,
    DEVICE_CLASS_MOVING,
)
from . import qmi8658_ns, QMI8658Component

CONF_QMI8658_ID = "qmi8658_id"
CONF_MOTION = "motion"
CONF_THRESHOLD = "threshold"
CONF_WOM = "wom"
CONF_INTERRUPT = "interrupt"

QMI8658MotionBinarySensor = qmi8658_ns.class_(
    "QMI8658MotionBinarySensor", binary_sensor.BinarySensor, cg.Component
)

QMI8658WoMBinarySensor = qmi8658_ns.class_(
    "QMI8658WoMBinarySensor", binary_sensor.BinarySensor, cg.Component
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_QMI8658_ID): cv.use_id(QMI8658Component),
        cv.Optional(CONF_MOTION): binary_sensor.binary_sensor_schema(
            QMI8658MotionBinarySensor,
            device_class=DEVICE_CLASS_MOTION,
        ).extend(
            {
                cv.Optional(CONF_THRESHOLD, default=0.5): cv.float_range(
                    min=0.1, max=10.0
                ),
            }
        ),
        cv.Optional(CONF_WOM): binary_sensor.binary_sensor_schema(
            QMI8658WoMBinarySensor,
            device_class=DEVICE_CLASS_MOTION,
        ).extend(
            {
                cv.Optional(CONF_THRESHOLD, default=100): cv.int_range(
                    min=1, max=255
                ),  # mg units
                cv.Required(CONF_PIN): pins.internal_gpio_input_pin_schema,
                cv.Optional(CONF_INTERRUPT, default=1): cv.int_range(
                    min=1, max=2
                ),  # INT1 or INT2
            }
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_QMI8658_ID])

    if CONF_MOTION in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_MOTION])
        await cg.register_component(sens, config[CONF_MOTION])
        cg.add(sens.set_parent(parent))
        cg.add(sens.set_threshold(config[CONF_MOTION][CONF_THRESHOLD]))

    if CONF_WOM in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_WOM])
        await cg.register_component(sens, config[CONF_WOM])
        cg.add(sens.set_parent(parent))
        cg.add(sens.set_threshold(config[CONF_WOM][CONF_THRESHOLD]))
        cg.add(sens.set_interrupt_number(config[CONF_WOM][CONF_INTERRUPT]))
        pin = await cg.gpio_pin_expression(config[CONF_WOM][CONF_PIN])
        cg.add(sens.set_interrupt_pin(pin))
