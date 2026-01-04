import esphome.codegen as cg
from esphome.components import sensor, uart
from esphome.const import (
    ICON_COUNTER,
    STATE_CLASS_MEASUREMENT,
)

CODEOWNERS = ["@iMicknl"]
DEPENDENCIES = ["uart"]

loctekmotion_ns = cg.esphome_ns.namespace("loctekmotion_desk_command")
DeskCommandSensor = loctekmotion_ns.class_(
    "DeskCommandSensor", sensor.Sensor, cg.Component, uart.UARTDevice
)

CONFIG_SCHEMA = sensor.sensor_schema(
    DeskCommandSensor,
    icon=ICON_COUNTER,
    accuracy_decimals=0,
    state_class=STATE_CLASS_MEASUREMENT,
).extend(uart.UART_DEVICE_SCHEMA)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "loctekmotion_desk_command",
    baud_rate=9600,
    require_tx=False,
    require_rx=True,
    data_bits=8,
    parity=None,
    stop_bits=1,
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
