#include "desk_height_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace loctekmotion_desk_height {

static const char *const TAG = "loctekmotion_desk_height.sensor";

void DeskHeightSensor::loop() {
  uint8_t byte;
  while (this->available() > 0) {
    if (!this->read_byte(&byte))
      continue;

    float height;
    if (this->decoder_.push(byte, &height))
      this->publish_state(height);
  }
}

void DeskHeightSensor::dump_config() {
  LOG_SENSOR("", "LoctekMotion Desk Height Sensor", this);
}

} // namespace loctekmotion_desk_height
} // namespace esphome
