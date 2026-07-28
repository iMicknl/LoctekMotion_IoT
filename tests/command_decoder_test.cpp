#include "components/loctekmotion_desk_command/command_decoder.h"

#include <cstdint>
#include <initializer_list>
#include <iostream>

namespace {

using esphome::loctekmotion_desk_command::decode_command;
using esphome::loctekmotion_protocol::Frame;
using esphome::loctekmotion_protocol::FrameReader;

int failures = 0;

void expect(bool condition, const char *message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    failures++;
  }
}

bool decode(std::initializer_list<uint8_t> bytes, uint8_t *command) {
  FrameReader reader;
  Frame frame;
  bool completed = false;
  for (uint8_t byte : bytes) {
    completed = reader.push(byte, &frame) || completed;
  }
  return completed && decode_command(frame, command);
}

void test_known_commands() {
  uint8_t command = 0;
  expect(decode({0x9B, 0x06, 0x02, 0x01, 0x00, 0xFC, 0xA0, 0x9D}, &command),
         "up frame should decode");
  expect(command == 1, "up should publish value 1");

  expect(decode({0x9B, 0x06, 0x02, 0x40, 0x00, 0xAC, 0x90, 0x9D}, &command),
         "alarm frame should decode");
  expect(command == 7, "alarm should publish value 7");

  expect(decode({0x9B, 0x06, 0x02, 0x00, 0x00, 0x6C, 0xA1, 0x9D}, &command),
         "release frame should decode");
  expect(command == 8, "release should publish value 8");

  expect(decode({0x9B, 0x06, 0x02, 0x00, 0x01, 0xAC, 0x60, 0x9D}, &command),
         "M4 frame should decode");
  expect(command == 9, "M4 should publish value 9");
}

void test_length_five_frame() {
  uint8_t command = 0;
  expect(decode({0x9B, 0x05, 0x02, 0x04, 0xAC, 0xA3, 0x9D}, &command),
         "single-payload-byte command should decode");
  expect(command == 3, "single-payload-byte preset should publish value 3");
}

void test_invalid_commands() {
  uint8_t command = 0;
  expect(!decode({0x9B, 0x06, 0x02, 0x03, 0x00, 0x00, 0x00, 0x9D}, &command),
         "multi-bit command should be rejected");
  expect(!decode({0x9B, 0x06, 0x02, 0x80, 0x00, 0x00, 0x00, 0x9D}, &command),
         "reserved low-byte bit must not collide with release");
  expect(!decode({0x9B, 0x06, 0x02, 0x00, 0x02, 0x00, 0x00, 0x9D}, &command),
         "unknown high-byte command should be rejected");
  expect(!decode({0x9B, 0x06, 0x03, 0x01, 0x00, 0x00, 0x00, 0x9D}, &command),
         "unrelated message type should be rejected");
  expect(!decode({0x9B, 0x06, 0x02, 0x01, 0x9D}, &command),
         "truncated command should not decode");
}

} // namespace

int main() {
  test_known_commands();
  test_length_five_frame();
  test_invalid_commands();
  return failures == 0 ? 0 : 1;
}
