#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

namespace esphome {
namespace loctekmotion_desk_height {

class DeskHeightSensor : public sensor::Sensor,
                         public Component,
                         public uart::UARTDevice {
public:
  float get_setup_priority() const override {
    return esphome::setup_priority::DATA;
  }

  void set_child_lock_sensor(binary_sensor::BinarySensor *sensor) {
    this->child_lock_sensor_ = sensor;
  }

  // ========== INTERNAL METHODS ==========
  void loop() override;
  void dump_config() override;

protected:
  void update_child_lock_(uint8_t digit1, uint8_t digit2, uint8_t digit3);

  binary_sensor::BinarySensor *child_lock_sensor_{nullptr};

  float value = 0;
  float lastPublished = -1;
  uint8_t history[5];

  uint8_t msg_len = 0;
  uint8_t msg_type;
  bool valid = false;
};

} // namespace loctekmotion_desk_height
} // namespace esphome
