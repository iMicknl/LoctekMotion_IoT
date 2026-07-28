# Pass-Through Keypad Routing Design

## Goal

Restore physical keypad routing in the pass-through ESPHome package without
overlapping the open byte-5 decoder work in PR #208.

## Root Cause

The package uses `on_value_range` entries whose `above` and `below` thresholds
are identical. ESPHome ranges are entered between strict thresholds, so every
one of these ranges is empty and no keypad command can match.

## Scope

- Replace the empty ranges with one `on_value` dispatcher.
- Preserve the current command contract for values 1 through 8:
  up, down, preset 1, preset 2, stand, memory, alarm, and release.
- Keep value 9/M4 out of this branch because PR #208 owns byte-5 decoding.
- Add an executable configuration test that publishes each value and verifies
  that ESPHome accepts the exact-value branches.

## Data Flow

The command sensor publishes an integer. `on_value` evaluates exact integer
conditions and invokes the existing switch or button action. Value 8 turns off
both recurring movement switches. Unknown values do nothing.

## Verification

- First run a contract test that fails on the empty `on_value_range` mapping.
- Implement the dispatcher and rerun the test.
- Validate and compile the pass-through fixtures on ESP32 and ESP8266.
- Run the full validation matrix and pre-commit hooks.

## Branch Relationship

This branch is based on the test-foundation branch. It does not modify the
command decoder.
