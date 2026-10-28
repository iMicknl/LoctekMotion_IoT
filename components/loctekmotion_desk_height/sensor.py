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

CONF_HEARTBEAT = "heartbeat"
CONF_HEARTBEAT_TIMEOUT = "heartbeat_timeout"

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
            # ON while the control box streams its heartbeat frame (9b 04 11),
            # i.e. while PIN 20 is HIGH on either RJ45 jack. OFF after `heartbeat_timeout`
            # without a heartbeat frame. The frame period is about 200 ms, so the
            # timeout must be well above that or the sensor flaps.
            cv.Optional(CONF_HEARTBEAT): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_HEARTBEAT_TIMEOUT, default="3s"): cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(min=cv.TimePeriod(milliseconds=500)),
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

    cg.add(var.set_heartbeat_timeout(config[CONF_HEARTBEAT_TIMEOUT]))
    if CONF_HEARTBEAT in config:
        heartbeat = await binary_sensor.new_binary_sensor(config[CONF_HEARTBEAT])
        cg.add(var.set_heartbeat_sensor(heartbeat))
