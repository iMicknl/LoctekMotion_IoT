#include "desk_height_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace loctekmotion_desk_height {

static const char *const TAG = "loctekmotion_desk_height.sensor";
static constexpr size_t MAX_BYTES_PER_LOOP = 64;

void DeskHeightSensor::loop() {
  loctekmotion_protocol::Frame frame;
  uint8_t incoming_byte = 0;
  size_t processed = 0;

  while (processed < MAX_BYTES_PER_LOOP && this->available() > 0) {
    if (!this->read_byte(&incoming_byte)) {
      break;
    }
    processed++;

    if (!this->frame_reader_.push(incoming_byte, &frame)) {
      continue;
    }

    float height = 0.0F;
    if (!decode_height(frame, &height) ||
        (this->has_published_ && height == this->last_published_)) {
      continue;
    }

    this->publish_state(height);
    this->last_published_ = height;
    this->has_published_ = true;
  }
}

void DeskHeightSensor::dump_config() {
  LOG_SENSOR("", "LoctekMotion Desk Height Sensor", this);
}

} // namespace loctekmotion_desk_height
} // namespace esphome
