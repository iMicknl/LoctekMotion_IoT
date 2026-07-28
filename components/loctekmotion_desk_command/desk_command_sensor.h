#pragma once

#include "command_decoder.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

namespace esphome {
namespace loctekmotion_desk_command {

class DeskCommandSensor : public sensor::Sensor,
                          public Component,
                          public uart::UARTDevice {
public:
  float get_setup_priority() const override { return setup_priority::DATA; }
  void loop() override;
  void dump_config() override;

protected:
  loctekmotion_protocol::FrameReader frame_reader_{};
  uint8_t last_published_{0};
  bool has_published_{false};
};

} // namespace loctekmotion_desk_command
} // namespace esphome
