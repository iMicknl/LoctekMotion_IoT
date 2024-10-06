#pragma once

#include <bitset>
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

namespace esphome {
namespace loctek {

class LoctekComponent : public Component, public uart::UARTDevice {
 public:
  LoctekComponent() = default;
  void set_height(sensor::Sensor *height) { height_ = height; }

  void setup() override {};
  void loop() override;
  void dump_config() override;

  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  float height_val_ = 0.0;

  float value_ = 0;
  float lastPublished_ = -1;
  unsigned long history_[5];

  int msg_len_ = 0;
  unsigned long msg_type_;
  bool valid_ = false;

  sensor::Sensor *height_{nullptr};

  int hex_to_int(byte s);
  bool is_decimal(byte b);

  void publish_nans_();
  void set_values_(const uint8_t *buffer);
};

}  // namespace loctek
}  // namespace esphome