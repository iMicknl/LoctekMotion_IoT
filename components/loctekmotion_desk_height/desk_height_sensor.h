#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "height_decoder.h"

namespace esphome {
namespace loctekmotion_desk_height {

class DeskHeightSensor : public sensor::Sensor,
                         public Component,
                         public uart::UARTDevice {
public:
  float get_setup_priority() const override { return setup_priority::DATA; }
  void loop() override;
  void dump_config() override;

protected:
  loctekmotion_protocol::FrameReader frame_reader_{};
  float last_published_{0.0F};
  bool has_published_{false};
};

} // namespace loctekmotion_desk_height
} // namespace esphome
