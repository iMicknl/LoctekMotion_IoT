# Height Decoder Hardening Design

## Goal

Decode height only from complete, well-framed Loctek packets and protect the
behavior with native tests built from captured protocol frames.

## Root Cause

The current component searches a sliding, uninitialized history buffer and
publishes its stored value whenever any `0x9d` byte arrives. It does not prove
that the terminator belongs to the frame that supplied the digits, and invalid
digit patterns collapse to zero.

## Scope

- Introduce a small fixed-capacity frame reader driven by the packet length
  byte. It resynchronizes on `0x9b`, rejects impossible lengths and overflow,
  and produces a frame only when `0x9d` appears at the expected position.
- Extract seven-segment digit and height decoding into ESPHome-independent C++
  code.
- Accept height messages of type `0x12` and lengths 7 or 10, matching current
  behavior.
- Reject invalid segment patterns rather than converting them to zero.
- Publish only a successfully decoded complete frame and preserve duplicate
  suppression.
- Leave command decoding untouched so the branch does not overlap PR #208.

Checksum validation is excluded because the repository does not document the
checksum algorithm.

## Tests

Native C++ tests cover:

- the captured `75.0` frame `9B 07 12 07 ED 3F 39 28 9D`;
- all decimal digits and the decimal-point bit;
- noise before a frame and back-to-back frames;
- truncated frames, early terminators, oversized lengths, invalid segments,
  and unrelated message types;
- duplicate complete frames not producing a new state.

ESPHome compilation still verifies integration with both ESP32 and ESP8266.

## Branch Relationship

This branch is based on the test-foundation branch and targets only the height
component plus its native tests.
