#pragma once

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

  // ========== INTERNAL METHODS ==========
  void loop() override;
  void dump_config() override;

protected:
  float value = 0;
  float lastPublished = -1;
  uint8_t history[5];

  int msg_len = 0;
  uint8_t msg_type;
  bool valid = false;
};

} // namespace loctekmotion_desk_height
} // namespace esphome
