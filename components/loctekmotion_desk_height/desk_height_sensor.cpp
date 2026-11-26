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

// 7-segment patterns as sent by the control box (bit 0 = segment a ... bit 6 =
// segment g, bit 7 = decimal point).
static const uint8_t SEGMENT_L = 0x38;
static const uint8_t SEGMENT_O_LOWER = 0x5c;
static const uint8_t SEGMENT_C = 0x39;
static const uint8_t SEGMENT_S = 0x6d; // same pattern as the digit 5
static const uint8_t SEGMENT_DASH = 0x40;

static bool is_digit_segment(uint8_t s) {
  switch (s & 0x7f) {
  case 0x3f: // 0
  case 0x06: // 1
  case 0x5b: // 2
  case 0x4f: // 3
  case 0x66: // 4
  case 0x6d: // 5
  case 0x7d: // 6
  case 0x07: // 7
  case 0x7f: // 8
  case 0x6f: // 9
    return true;
  default:
    return false;
  }
}

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

      // End of a display frame (9b 07 12 <digit 1> <digit 2> <digit 3>
      // <checksum> <checksum> 9d). Before the shift below, the three digits
      // are still in history[4], history[3] and history[2].
      if (incomingByte == 0x9d && this->msg_type == 0x12 &&
          this->msg_len == 7) {
        this->update_child_lock_(this->history[4], this->history[3],
                                 this->history[2]);
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

void DeskHeightSensor::update_child_lock_(uint8_t digit1, uint8_t digit2,
                                          uint8_t digit3) {
  if (this->child_lock_sensor_ == nullptr) {
    return;
  }

  bool locked;
  if (digit1 == SEGMENT_L && digit2 == SEGMENT_O_LOWER && digit3 == SEGMENT_C) {
    // "LoC": child lock is active
    locked = true;
  } else if (digit1 == SEGMENT_S && digit2 == SEGMENT_DASH && digit3 == 0x00) {
    // "S-": memory menu, only reachable while the child lock is off
    locked = false;
  } else if (is_digit_segment(digit1) && is_digit_segment(digit2) &&
             is_digit_segment(digit3)) {
    // A height is shown: child lock is off
    locked = false;
  } else {
    // Display off (00 00 00) or a pattern we do not know yet
    if (digit1 != 0x00 || digit2 != 0x00 || digit3 != 0x00) {
      ESP_LOGV(TAG, "Unknown display content: %02x %02x %02x", digit1, digit2,
               digit3);
    }
    return;
  }

  if (!this->child_lock_sensor_->has_state() ||
      this->child_lock_sensor_->state != locked) {
    this->child_lock_sensor_->publish_state(locked);
  }
}

void DeskHeightSensor::dump_config() {
  LOG_SENSOR("", "LoctekMotion Desk Height Sensor", this);
  LOG_BINARY_SENSOR("  ", "Child lock", this->child_lock_sensor_);
}

} // namespace loctekmotion_desk_height
} // namespace esphome
