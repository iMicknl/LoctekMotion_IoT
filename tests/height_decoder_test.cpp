#include "components/loctekmotion_desk_height/height_decoder.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <vector>

namespace {

using esphome::loctekmotion_desk_height::decode_digit;
using esphome::loctekmotion_desk_height::decode_height;
using esphome::loctekmotion_protocol::Frame;
using esphome::loctekmotion_protocol::FrameReader;

int failures = 0;

void expect(bool condition, const char *message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    failures++;
  }
}

bool feed(FrameReader &reader, std::initializer_list<uint8_t> bytes,
          Frame *frame) {
  bool completed = false;
  for (uint8_t byte : bytes) {
    completed = reader.push(byte, frame) || completed;
  }
  return completed;
}

void test_captured_height_frame() {
  FrameReader reader;
  Frame frame;
  expect(feed(reader, {0x9B, 0x07, 0x12, 0x07, 0xED, 0x3F, 0x39, 0x28, 0x9D},
              &frame),
         "captured height frame should complete");

  float height = 0.0F;
  expect(decode_height(frame, &height), "captured height should decode");
  expect(std::fabs(height - 75.0F) < 0.001F,
         "captured height should equal 75.0");
}

void test_all_digits_and_decimal_bit() {
  constexpr uint8_t segments[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66,
                                  0x6D, 0x7D, 0x07, 0x7F, 0x6F};
  for (uint8_t digit = 0; digit < 10; digit++) {
    uint8_t decoded = 0xFF;
    expect(decode_digit(segments[digit], &decoded),
           "standard seven-segment digit should decode");
    expect(decoded == digit, "decoded digit should match its segment pattern");
    expect(decode_digit(static_cast<uint8_t>(segments[digit] | 0x80), &decoded),
           "decimal bit should not change digit decoding");
    expect(decoded == digit, "decimal digit should retain its numeric value");
  }

  uint8_t decoded = 0;
  expect(!decode_digit(0x00, &decoded), "blank display should be invalid");
  expect(!decode_digit(0x01, &decoded), "unknown segment pattern should fail");
}

void test_noise_resynchronization_and_back_to_back_frames() {
  FrameReader reader;
  Frame frame;
  expect(!feed(reader, {0x00, 0xFF, 0x9D, 0x12}, &frame),
         "leading noise should not complete a frame");
  expect(feed(reader, {0x9B, 0x07, 0x12, 0x07, 0xED, 0x3F, 0x39, 0x28, 0x9D},
              &frame),
         "reader should synchronize after noise");

  float height = 0.0F;
  expect(decode_height(frame, &height),
         "first back-to-back frame should decode");
  expect(feed(reader, {0x9B, 0x07, 0x12, 0x7D, 0xEF, 0x5B, 0x00, 0x00, 0x9D},
              &frame),
         "second back-to-back frame should complete");
  expect(decode_height(frame, &height),
         "second back-to-back frame should decode");
  expect(std::fabs(height - 69.2F) < 0.001F,
         "second back-to-back height should equal 69.2");
}

void test_malformed_frames_are_rejected() {
  FrameReader reader;
  Frame frame;
  float height = 0.0F;

  expect(!feed(reader, {0x9B, 0x07, 0x12, 0x07, 0xED, 0x3F}, &frame),
         "truncated frame should not complete");
  reader.reset();
  expect(!feed(reader, {0x9B, 0x07, 0x12, 0x07, 0x9D}, &frame),
         "early terminator should reject a frame");
  expect(!feed(reader, {0x9B, 0xFF, 0x12, 0x07, 0xED, 0x3F, 0x9D}, &frame),
         "oversized length should reject a frame");

  expect(feed(reader, {0x9B, 0x07, 0x11, 0x07, 0xED, 0x3F, 0x00, 0x00, 0x9D},
              &frame),
         "well-framed unrelated message should complete");
  expect(!decode_height(frame, &height),
         "unrelated message type should not decode");

  expect(feed(reader, {0x9B, 0x07, 0x12, 0x07, 0x80, 0x6D, 0x00, 0x00, 0x9D},
              &frame),
         "invalid-segment frame should still complete structurally");
  expect(!decode_height(frame, &height),
         "invalid middle digit must not become a plausible zero");

  expect(feed(reader, {0x9B, 0x07, 0x12, 0x87, 0xED, 0x3F, 0x00, 0x00, 0x9D},
              &frame),
         "misplaced-decimal frame should complete structurally");
  expect(!decode_height(frame, &height),
         "decimal marker outside the middle digit should be rejected");
}

void test_supported_long_frame() {
  FrameReader reader;
  Frame frame;
  expect(feed(reader,
              {0x9B, 0x0A, 0x12, 0x07, 0xED, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00,
               0x9D},
              &frame),
         "length-10 height frame should complete");
  float height = 0.0F;
  expect(decode_height(frame, &height), "length-10 height frame should decode");
  expect(std::fabs(height - 75.0F) < 0.001F,
         "length-10 height should equal 75.0");
}

} // namespace

int main() {
  test_captured_height_frame();
  test_all_digits_and_decimal_bit();
  test_noise_resynchronization_and_back_to_back_frames();
  test_malformed_frames_are_rejected();
  test_supported_long_frame();
  return failures == 0 ? 0 : 1;
}
