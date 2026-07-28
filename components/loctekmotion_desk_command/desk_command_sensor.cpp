#include "desk_command_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace loctekmotion_desk_command {

static const char *const TAG = "loctekmotion_desk_command.sensor";
static constexpr size_t MAX_BYTES_PER_LOOP = 64;

void DeskCommandSensor::loop() {
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

    uint8_t command = 0;
    if (!decode_command(frame, &command) ||
        (this->has_published_ && command == this->last_published_)) {
      continue;
    }

    this->publish_state(command);
    this->last_published_ = command;
    this->has_published_ = true;
  }
}

void DeskCommandSensor::dump_config() {
  LOG_SENSOR("", "LoctekMotion Desk Command Sensor", this);
}

} // namespace loctekmotion_desk_command
} // namespace esphome
