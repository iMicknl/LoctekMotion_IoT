#include "desk_height_sensor.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include <bitset>

namespace esphome {
namespace loctekmotion_desk_height {

static const char *const TAG = "loctekmotion_desk_height.sensor";

// ========== UTILITY METHODS ==========
int hex_to_int(uint8_t s) {
  std::bitset<8> b(s);

  if (b[0] && b[1] && b[2] && b[3] && b[4] && b[5] && !b[6]) {
    return 0;
  }
  if (not b[0] && b[1] && b[2] && !b[3] && !b[4] && !b[5] && !b[6]) {
    return 1;
  }
  if (b[0] && b[1] && !b[2] && b[3] && b[4] && !b[5] && b[6]) {
    return 2;
  }
  if (b[0] && b[1] && b[2] && b[3] && !b[4] && !b[5] && b[6]) {
    return 3;
  }
  if (not b[0] && b[1] && b[2] && !b[3] && !b[4] && b[5] && b[6]) {
    return 4;
  }
  if (b[0] && !b[1] && b[2] && b[3] && !b[4] && b[5] && b[6]) {
    return 5;
  }
  if (b[0] && !b[1] && b[2] && b[3] && b[4] && b[5] && b[6]) {
    return 6;
  }
  if (b[0] && b[1] && b[2] && !b[3] && !b[4] && !b[5] && !b[6]) {
    return 7;
  }
  if (b[0] && b[1] && b[2] && b[3] && b[4] && b[5] && b[6]) {
    return 8;
  }
  if (b[0] && b[1] && b[2] && b[3] && !b[4] && b[5] && b[6]) {
    return 9;
  }
  if (!b[0] && !b[1] && !b[2] && !b[3] && !b[4] && !b[5] && b[6]) {
    return 10;
  }
  return 0;
}

bool is_decimal(uint8_t b) { return (b & 0x80) == 0x80; }

// ========== INTERNAL METHODS ==========
void DeskHeightSensor::loop() {
  uint8_t incomingByte;
  while (this->available() > 0) {
    if (this->read_byte(&incomingByte)) {
      // ESP_LOGD("DEBUG", "Incoming byte is: %08x", incomingByte);

      // First byte, start of a packet
      if (incomingByte == 0x9b) {
        // Reset message length
        this->msg_len = 0;
        this->valid = false;
      }

      // Second byte defines the message length
      if (this->history[0] == 0x9b) {
        this->msg_len = incomingByte;
      }

      // Third byte is message type
      if (this->history[1] == 0x9b) {
        this->msg_type = incomingByte;
      }

      // Fourth byte is first height digit, if msg type 0x12 & msg len 7
      if (this->history[2] == 0x9b) {

        if (this->msg_type == 0x12 &&
            (this->msg_len == 7 || this->msg_len == 10)) {
          // Empty height
          if (incomingByte == 0) {
            // ESP_LOGD("DEBUG", "Height 1 is EMPTY -> 0x%02x", incomingByte);
            // deskSerial.write(command_wakeup, sizeof(command_wakeup));
          } else if (hex_to_int(incomingByte) == 0) {
            // ESP_LOGD("DEBUG", "Invalid height 1 -> 0x%02x", incomingByte);
            // deskSerial.write(command_wakeup, sizeof(command_wakeup));
          } else {
            this->valid = true;
            //   ESP_LOGD("DEBUG", "Height 1 is: 0x%02x", incomingByte);
          }
        }
      }

      // Fifth byte is second height digit
      if (this->history[3] == 0x9b) {
        if (this->valid == true) {
          // ESP_LOGD("DEBUG", "Height 2 is: 0x%02x", incomingByte);
        }
      }

      // Sixth byte is third height digit
      if (this->history[4] == 0x9b) {
        if (this->valid == true) {
          int height1 = hex_to_int(this->history[1]) * 100;
          int height2 = hex_to_int(this->history[0]) * 10;
          int height3 = hex_to_int(incomingByte);
          if (height2 == 100) // check if 'number' is a hyphen, return value 10
                              // multiplied by 10
          {
          } else {
            float finalHeight = height1 + height2 + height3;
            if (is_decimal(this->history[0])) {
              finalHeight = finalHeight / 10;
            }
            this->value = finalHeight;
            // ESP_LOGD("DeskHeightSensor", "Current height is: %f",
            // finalHeight);
          }
        }
      }

      // Save byte buffer to history arrary
      this->history[4] = this->history[3];
      this->history[3] = this->history[2];
      this->history[2] = this->history[1];
      this->history[1] = this->history[0];
      this->history[0] = incomingByte;

      // End byte
      if (incomingByte == 0x9d) {
        if (this->value && this->value != this->lastPublished) {
          this->publish_state(this->value);
          this->lastPublished = this->value;
        }
      }
    }
  }
}

void DeskHeightSensor::dump_config() {
  LOG_SENSOR("", "LoctekMotion Desk Height Sensor", this);
}

} // namespace loctekmotion_desk_height
} // namespace esphome
