"""Desk height sensor for Loctek motion desk."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, sensor, uart
from esphome.const import (
    DEVICE_CLASS_DISTANCE,
    ICON_ARROW_EXPAND_VERTICAL,
    STATE_CLASS_MEASUREMENT,
    UNIT_CENTIMETER,
)

CODEOWNERS = ["@iMicknl"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["binary_sensor"]

CONF_CHILD_LOCK = "child_lock"

loctekmotion_ns = cg.esphome_ns.namespace("loctekmotion_desk_height")
DeskHeightSensor = loctekmotion_ns.class_(
    "DeskHeightSensor", sensor.Sensor, cg.Component, uart.UARTDevice
)

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        DeskHeightSensor,
        unit_of_measurement=UNIT_CENTIMETER,
        icon=ICON_ARROW_EXPAND_VERTICAL,
        accuracy_decimals=1,
        state_class=STATE_CLASS_MEASUREMENT,
        device_class=DEVICE_CLASS_DISTANCE,
    )
    .extend(
        {
            # ON while the display shows "LoC" (child lock active), OFF while it
            # shows a height or the memory menu "S-". Unknown until the display
            # has shown one of them since boot.
            cv.Optional(CONF_CHILD_LOCK): binary_sensor.binary_sensor_schema(
                icon="mdi:account-lock",
            ),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "loctekmotion_desk_height",
    baud_rate=9600,
    require_tx=False,
    require_rx=True,
    data_bits=8,
    parity=None,
    stop_bits=1,
)


async def to_code(config):
    """Generate code for the desk height sensor."""
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_CHILD_LOCK in config:
        child_lock = await binary_sensor.new_binary_sensor(config[CONF_CHILD_LOCK])
        cg.add(var.set_child_lock_sensor(child_lock))
