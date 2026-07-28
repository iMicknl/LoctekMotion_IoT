#pragma once

#include "../loctekmotion_protocol/frame_reader.h"

#include <cstdint>

namespace esphome {
namespace loctekmotion_desk_height {

inline bool decode_digit(uint8_t segments, uint8_t *digit) {
  if (digit == nullptr) {
    return false;
  }

  switch (segments & 0x7F) {
  case 0x3F:
    *digit = 0;
    return true;
  case 0x06:
    *digit = 1;
    return true;
  case 0x5B:
    *digit = 2;
    return true;
  case 0x4F:
    *digit = 3;
    return true;
  case 0x66:
    *digit = 4;
    return true;
  case 0x6D:
    *digit = 5;
    return true;
  case 0x7D:
    *digit = 6;
    return true;
  case 0x07:
    *digit = 7;
    return true;
  case 0x7F:
    *digit = 8;
    return true;
  case 0x6F:
    *digit = 9;
    return true;
  default:
    return false;
  }
}

inline bool decode_height(const loctekmotion_protocol::Frame &frame,
                          float *height) {
  if (height == nullptr || frame.size < 7 ||
      frame.bytes[0] != loctekmotion_protocol::FRAME_START ||
      frame.bytes[frame.size - 1] != loctekmotion_protocol::FRAME_END) {
    return false;
  }

  const uint8_t message_length = frame.bytes[1];
  if (frame.size != static_cast<size_t>(message_length) + 2 ||
      (message_length != 7 && message_length != 10) || frame.bytes[2] != 0x12) {
    return false;
  }

  if ((frame.bytes[3] & 0x80) != 0 || (frame.bytes[5] & 0x80) != 0) {
    return false;
  }

  uint8_t hundreds = 0;
  uint8_t tens = 0;
  uint8_t ones = 0;
  if (!decode_digit(frame.bytes[3], &hundreds) ||
      !decode_digit(frame.bytes[4], &tens) ||
      !decode_digit(frame.bytes[5], &ones)) {
    return false;
  }

  const uint16_t encoded = static_cast<uint16_t>(hundreds) * 100 +
                           static_cast<uint16_t>(tens) * 10 +
                           static_cast<uint16_t>(ones);
  *height = (frame.bytes[4] & 0x80) != 0 ? static_cast<float>(encoded) / 10.0F
                                         : static_cast<float>(encoded);
  return true;
}

} // namespace loctekmotion_desk_height
} // namespace esphome
