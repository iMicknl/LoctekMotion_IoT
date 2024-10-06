import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor
from esphome.const import (
  CONF_ID,
  CONF_HEIGHT,
  DEVICE_CLASS_EMPTY,
  UNIT_CENTIMETER,
  STATE_CLASS_MEASUREMENT
)

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@phntxx"]

loctek_ns = cg.esphome_ns.namespace("loctek")
loctekCore = loctek_ns.class_("LoctekComponent", uart.UARTDevice, cg.Component)

CONFIG_SCHEMA = (
  cv.Schema(
    {
      cv.GenerateID(): cv.declare_id(loctekCore),
      cv.Optional(CONF_HEIGHT): sensor.sensor_schema(
        unit_of_measurement=UNIT_CENTIMETER,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_EMPTY,
        state_class=STATE_CLASS_MEASUREMENT
      )
    } 
  )
  .extend(cv.COMPONENT_SCHEMA)
  .extend(uart.UART_DEVICE_SCHEMA)
)

async def to_code(config):
  var = cg.new_Pvariable(config[CONF_ID])
  await cg.register_component(var, config)
  await uart.register_uart_device(var, config)

  if height_config := config.get(CONF_HEIGHT):
    sens = await sensor.new_sensor(height_config)
    cg.add(var.set_height(sens))
