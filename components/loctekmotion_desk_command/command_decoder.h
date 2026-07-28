#pragma once

#include "../loctekmotion_protocol/frame_reader.h"

#include <cstdint>

namespace esphome {
namespace loctekmotion_desk_command {

inline bool decode_command(const loctekmotion_protocol::Frame &frame,
                           uint8_t *command) {
  if (command == nullptr || frame.size < 7 ||
      frame.bytes[0] != loctekmotion_protocol::FRAME_START ||
      frame.bytes[frame.size - 1] != loctekmotion_protocol::FRAME_END) {
    return false;
  }

  const uint8_t message_length = frame.bytes[1];
  if (frame.size != static_cast<size_t>(message_length) + 2 ||
      (message_length != 5 && message_length != 6) || frame.bytes[2] != 0x02) {
    return false;
  }

  const uint8_t high_payload = message_length == 6 ? frame.bytes[4] : 0;
  const uint16_t mask = static_cast<uint16_t>(frame.bytes[3]) |
                        (static_cast<uint16_t>(high_payload) << 8);
  if (mask == 0) {
    *command = 8;
    return true;
  }

  if ((mask & (mask - 1)) != 0 || mask == 0x0080 || mask > 0x0100) {
    return false;
  }

  uint8_t bit_index = 0;
  uint16_t shifted = mask;
  while ((shifted & 1) == 0) {
    shifted >>= 1;
    bit_index++;
  }

  *command = static_cast<uint8_t>(bit_index + 1);
  return true;
}

} // namespace loctekmotion_desk_command
} // namespace esphome
