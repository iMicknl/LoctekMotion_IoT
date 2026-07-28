# Reliability Test Foundation Design

## Goal

Make CI exercise the published ESPHome packages and both local external
components on ESP32 and ESP8266, so later reliability fixes cannot pass through
empty or stale fixtures.

## Scope

- Add an overridable external-component source substitution to both published
  ESP32 packages. Production continues to default to this repository on GitHub.
- Make the ESP32 fixtures import the published package files while overriding
  secrets and the component source for local builds.
- Replace the ESP32 and ESP8266 pass-through `# TODO` fixtures with real
  configurations that instantiate both the height and command components.
- Keep the existing four-board validation and compile matrix.
- Add a fast test that loads all four fixtures through ESPHome and proves both
  pass-through fixtures generate the command component.

This branch changes testability, not desk behavior.

## Architecture

Published packages expose `external_components_source` as a substitution. A
consumer sees no change because its default remains
`github://iMicknl/LoctekMotion_IoT`. Test configurations override the value with
`../components`, ensuring pull-request code is compiled instead of `main`.

ESP32 tests consume the published package as their source of truth. ESP8266
tests remain focused compatibility fixtures because the published packages
select an ESP32 board.

## Verification

- Run the fast fixture test and observe it fail before replacing the stubs.
- Validate and compile all four local fixtures with ESPHome 2026.6.5.
- Validate both published packages with their default GitHub component source.
- Run all pre-commit hooks.

## Branch Relationship

This is the base for the routing and decoder-hardening branches. It targets
`main`.
