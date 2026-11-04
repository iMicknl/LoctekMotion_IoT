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

  void set_heartbeat_sensor(binary_sensor::BinarySensor *sensor) {
    this->heartbeat_sensor_ = sensor;
  }
  void set_heartbeat_timeout(uint32_t timeout_ms) {
    this->heartbeat_timeout_ = timeout_ms;
  }

  // ========== INTERNAL METHODS ==========
  void setup() override;
  void loop() override;
  void dump_config() override;

protected:
  binary_sensor::BinarySensor *heartbeat_sensor_{nullptr};
  uint32_t heartbeat_timeout_{3000};
  uint32_t last_heartbeat_{0};

  float value = 0;
  float lastPublished = -1;
  uint8_t history[5];

  uint8_t msg_len = 0;
  uint8_t msg_type{0};
  bool valid = false;
};

} // namespace loctekmotion_desk_height
} // namespace esphome
