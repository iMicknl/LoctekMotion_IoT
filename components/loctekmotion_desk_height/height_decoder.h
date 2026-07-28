#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace esphome {
namespace loctekmotion_desk_height {

struct Frame {
  static constexpr size_t CAPACITY = 12;

  std::array<uint8_t, CAPACITY> bytes{};
  size_t size{0};
};

class FrameReader {
public:
  const Frame *push(uint8_t byte) {
    if (byte == START_BYTE) {
      this->size_ = 1;
      this->expected_size_ = 0;
      this->buffer_[0] = byte;
      return nullptr;
    }

    if (this->size_ == 0)
      return nullptr;

    if (this->size_ >= this->buffer_.size()) {
      this->reset_();
      return nullptr;
    }

    this->buffer_[this->size_++] = byte;

    if (this->size_ == 2) {
      this->expected_size_ = static_cast<size_t>(byte) + 2;
      if (this->expected_size_ > this->buffer_.size()) {
        this->reset_();
        return nullptr;
      }
    }

    if (byte == END_BYTE && this->size_ != this->expected_size_) {
      this->reset_();
      return nullptr;
    }

    if (this->expected_size_ != 0 && this->size_ == this->expected_size_) {
      if (byte != END_BYTE) {
        this->reset_();
        return nullptr;
      }

      this->completed_.size = this->size_;
      for (size_t index = 0; index < this->size_; index++)
        this->completed_.bytes[index] = this->buffer_[index];
      this->reset_();
      return &this->completed_;
    }

    return nullptr;
  }

protected:
  static constexpr uint8_t START_BYTE = 0x9B;
  static constexpr uint8_t END_BYTE = 0x9D;

  void reset_() {
    this->size_ = 0;
    this->expected_size_ = 0;
  }

  std::array<uint8_t, Frame::CAPACITY> buffer_{};
  size_t size_{0};
  size_t expected_size_{0};
  Frame completed_{};
};

inline bool decode_digit(uint8_t segments, int *digit) {
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

inline bool decode_height(const Frame &frame, float *height) {
  if (frame.size < 6 || frame.bytes[0] != 0x9B ||
      frame.size != static_cast<size_t>(frame.bytes[1]) + 2 ||
      frame.bytes[frame.size - 1] != 0x9D || frame.bytes[2] != 0x12 ||
      (frame.bytes[1] != 7 && frame.bytes[1] != 10)) {
    return false;
  }

  int hundreds;
  int tens;
  int ones;
  if (!decode_digit(frame.bytes[3], &hundreds) ||
      !decode_digit(frame.bytes[4], &tens) ||
      !decode_digit(frame.bytes[5], &ones)) {
    return false;
  }

  *height = static_cast<float>(hundreds * 100 + tens * 10 + ones);
  if ((frame.bytes[4] & 0x80) != 0)
    *height /= 10.0F;
  return true;
}

class HeightDecoder {
public:
  bool push(uint8_t byte, float *height) {
    const Frame *frame = this->reader_.push(byte);
    float decoded;
    if (frame == nullptr || !decode_height(*frame, &decoded))
      return false;

    if (this->has_last_height_ && decoded == this->last_height_)
      return false;

    this->has_last_height_ = true;
    this->last_height_ = decoded;
    *height = decoded;
    return true;
  }

protected:
  FrameReader reader_{};
  bool has_last_height_{false};
  float last_height_{0.0F};
};

} // namespace loctekmotion_desk_height
} // namespace esphome
