#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace esphome {
namespace loctekmotion_protocol {

constexpr uint8_t FRAME_START = 0x9B;
constexpr uint8_t FRAME_END = 0x9D;
constexpr size_t MAX_FRAME_SIZE = 12;

struct Frame {
  std::array<uint8_t, MAX_FRAME_SIZE> bytes{};
  size_t size{0};
};

class FrameReader {
public:
  bool push(uint8_t byte, Frame *completed) {
    if (byte == FRAME_START) {
      this->start_frame_();
      return false;
    }

    if (this->frame_.size == 0) {
      return false;
    }

    if (this->frame_.size >= MAX_FRAME_SIZE) {
      this->reset();
      return false;
    }

    this->frame_.bytes[this->frame_.size++] = byte;
    if (this->frame_.size == 2) {
      this->expected_size_ = static_cast<size_t>(byte) + 2;
      if (this->expected_size_ < 3 || this->expected_size_ > MAX_FRAME_SIZE) {
        this->reset();
      }
      return false;
    }

    if (byte == FRAME_END && this->frame_.size != this->expected_size_) {
      this->reset();
      return false;
    }

    if (this->frame_.size != this->expected_size_) {
      return false;
    }

    if (byte != FRAME_END || completed == nullptr) {
      this->reset();
      return false;
    }

    *completed = this->frame_;
    this->reset();
    return true;
  }

  void reset() {
    this->frame_ = {};
    this->expected_size_ = 0;
  }

private:
  void start_frame_() {
    this->frame_ = {};
    this->frame_.bytes[0] = FRAME_START;
    this->frame_.size = 1;
    this->expected_size_ = 0;
  }

  Frame frame_{};
  size_t expected_size_{0};
};

} // namespace loctekmotion_protocol
} // namespace esphome
