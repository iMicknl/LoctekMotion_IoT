# Motion Failsafes Design

## Goal

Ensure every software-initiated movement has a bounded lifetime and can be
cancelled safely even when height feedback is missing, stale, or interrupted.

## Scope

- Add a `movement_timeout` substitution with a conservative default of 30
  seconds.
- Route movement starts through restartable scripts that stop the opposite
  direction, send the existing wake frame, wait 200 ms, then start the recurring
  UART switch.
- Cancel pending start scripts when Stop is requested, preventing a delayed
  start after a rapid Start/Stop sequence.
- Start a restartable failsafe timer whenever either movement direction starts.
  Expiry turns off both UART switches.
- Require a finite, in-range height state before accepting a requested
  position. Invalid state logs a warning and leaves the desk stopped.
- Add a timeout to every feedback wait and always execute Stop afterward.

The branch does not add polling, change preset mappings, or tune positional
overshoot.

## Data Flow

Cover, number, and pass-through keypad actions request a direction through one
of two scripts. The start script establishes command order. Once a UART switch
is active, a separate failsafe owns the maximum runtime. Stop cancels both
pending starts and the failsafe before switching both directions off.

## Error Handling

- Missing or non-finite sensor state: reject positional movement.
- Height outside configured limits: reject positional movement.
- Target never reached: `wait_until` expires and Stop runs.
- API interruption after manual open/close: the independent failsafe stops
  recurring UART output.
- Stop during the wake delay: pending start scripts are cancelled.

## Verification

- Add failing package-behavior tests for timeout presence, invalid-state guards,
  and Stop cancellation.
- Validate generated ESPHome automation code on normal and pass-through ESP32
  packages and ESP8266 compatibility fixtures.
- Compile the full matrix and run pre-commit hooks.

## Branch Relationship

This branch is based on the pass-through-routing branch so keypad movement uses
the same scripts. It remains separate from polling PR #224.
