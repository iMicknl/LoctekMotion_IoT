#include "components/loctekmotion_desk_height/height_decoder.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <iostream>

using esphome::loctekmotion_desk_height::decode_digit;
using esphome::loctekmotion_desk_height::decode_height;
using esphome::loctekmotion_desk_height::Frame;
using esphome::loctekmotion_desk_height::FrameReader;
using esphome::loctekmotion_desk_height::HeightDecoder;

namespace {

int failures = 0;

void expect(bool condition, const char *message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    failures++;
  }
}

Frame make_frame(std::initializer_list<uint8_t> bytes) {
  Frame frame;
  frame.size = bytes.size();
  size_t index = 0;
  for (uint8_t byte : bytes)
    frame.bytes[index++] = byte;
  return frame;
}

void test_all_digits() {
  constexpr std::array<uint8_t, 10> SEGMENTS{0x3F, 0x06, 0x5B, 0x4F, 0x66,
                                             0x6D, 0x7D, 0x07, 0x7F, 0x6F};

  for (size_t expected = 0; expected < SEGMENTS.size(); expected++) {
    int actual = -1;
    expect(decode_digit(SEGMENTS[expected], &actual), "valid digit rejected");
    expect(actual == static_cast<int>(expected), "digit decoded incorrectly");
  }

  int decimal_digit = -1;
  expect(decode_digit(0xED, &decimal_digit), "decimal bit rejected");
  expect(decimal_digit == 5, "decimal bit changed digit");
  expect(!decode_digit(0x00, &decimal_digit), "invalid segments accepted");
}

void test_height_frames() {
  const Frame captured =
      make_frame({0x9B, 0x07, 0x12, 0x07, 0xED, 0x3F, 0x39, 0x28, 0x9D});
  float height = 0.0F;
  expect(decode_height(captured, &height), "captured frame rejected");
  expect(std::fabs(height - 75.0F) < 0.001F, "captured height incorrect");

  const Frame long_frame = make_frame(
      {0x9B, 0x0A, 0x12, 0x06, 0x5B, 0x4F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9D});
  expect(decode_height(long_frame, &height), "length-10 frame rejected");
  expect(std::fabs(height - 123.0F) < 0.001F, "length-10 height incorrect");

  const Frame wrong_type =
      make_frame({0x9B, 0x07, 0x11, 0x07, 0xED, 0x3F, 0x39, 0x28, 0x9D});
  expect(!decode_height(wrong_type, &height), "unrelated frame accepted");

  const Frame invalid_digit =
      make_frame({0x9B, 0x07, 0x12, 0x07, 0x00, 0x3F, 0x39, 0x28, 0x9D});
  expect(!decode_height(invalid_digit, &height), "invalid digit accepted");
}

void test_frame_reader() {
  FrameReader reader;
  const Frame *frame = nullptr;
  for (uint8_t byte :
       {0x01, 0x02, 0x9B, 0x07, 0x12, 0x07, 0xED, 0x3F, 0x39, 0x28, 0x9D})
    frame = reader.push(byte);
  expect(frame != nullptr, "leading noise prevented frame");

  reader = FrameReader{};
  for (uint8_t byte : {0x9B, 0x07, 0x12, 0x07, 0x9D})
    frame = reader.push(byte);
  expect(frame == nullptr, "early terminator completed frame");

  reader = FrameReader{};
  for (uint8_t byte : {0x9B, 0xFF, 0x12, 0x07, 0xED, 0x3F, 0x9D})
    frame = reader.push(byte);
  expect(frame == nullptr, "oversized frame completed");

  reader = FrameReader{};
  for (uint8_t byte : {0x9B, 0x07, 0x12, 0x07, 0xED})
    frame = reader.push(byte);
  expect(frame == nullptr, "truncated frame completed");

  int completed = 0;
  reader = FrameReader{};
  for (uint8_t byte : {0x9B, 0x07, 0x12, 0x07, 0xED, 0x3F, 0x39, 0x28, 0x9D,
                       0x9B, 0x07, 0x12, 0x06, 0xDB, 0x4F, 0x39, 0x28, 0x9D}) {
    if (reader.push(byte) != nullptr)
      completed++;
  }
  expect(completed == 2, "back-to-back frames were lost");
}

void test_duplicate_suppression() {
  HeightDecoder decoder;
  int published = 0;
  float height = 0.0F;
  for (uint8_t byte : {0x9B, 0x07, 0x12, 0x07, 0xED, 0x3F, 0x39, 0x28, 0x9D,
                       0x9B, 0x07, 0x12, 0x07, 0xED, 0x3F, 0x39, 0x28, 0x9D}) {
    if (decoder.push(byte, &height))
      published++;
  }
  expect(published == 1, "duplicate height produced another state");
}

} // namespace

int main() {
  test_all_digits();
  test_height_frames();
  test_frame_reader();
  test_duplicate_suppression();
  return failures == 0 ? 0 : 1;
}
