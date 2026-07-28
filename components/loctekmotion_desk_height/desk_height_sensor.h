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
  float get_setup_priority() const override {
    return esphome::setup_priority::DATA;
  }

  // ========== INTERNAL METHODS ==========
  void loop() override;
  void dump_config() override;

protected:
  HeightDecoder decoder_{};
};

} // namespace loctekmotion_desk_height
} // namespace esphome
