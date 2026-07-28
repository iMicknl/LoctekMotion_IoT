# Pass-Through Keypad Routing Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Route existing keypad command values 1–8 through exact-value ESPHome automations.

**Architecture:** Replace threshold-entry automations with one publish-triggered dispatcher. Test the resolved ESPHome configuration rather than matching source text.

**Tech Stack:** ESPHome YAML automations, Python `unittest`, ESP32/ESP8266 compile matrix.

## Global Constraints

- Do not modify `loctekmotion_desk_command`; PR #208 owns byte-5 decoding.
- Do not add value 9/M4 routing in this branch.
- Preserve the current value-to-action mapping for values 1–8.
- Push the branch without opening or commenting on a PR.

---

### Task 1: Reproduce the empty-range bug

**Files:**
- Modify: `tests/test_configurations.py`

**Interfaces:**
- Consumes: resolved output from `packages/office-desk-esp32-passthrough.yaml`.
- Produces: a regression test that rejects an empty exact-value range and requires an `on_value` trigger.

- [ ] **Step 1: Add a failing behavior test**

Run `esphome config` for the pass-through fixture and assert the resolved
command sensor contains `on_value`, while no command mapping contains identical
`above` and `below` thresholds.

- [ ] **Step 2: Verify RED**

Run: `uv run python -m unittest tests/test_configurations.py -v`

Expected: the routing test fails against the current `on_value_range` entries.

### Task 2: Implement exact dispatch

**Files:**
- Modify: `packages/office-desk-esp32-passthrough.yaml`

**Interfaces:**
- Consumes: published sensor value `x`.
- Produces: actions for 1 up, 2 down, 3 preset 1, 4 preset 2, 5 stand, 6 memory, 7 alarm, and 8 release.

- [ ] **Step 1: Replace `on_value_range` with `on_value`**

Use sequential `if` actions with lambda conditions such as:

```yaml
- if:
    condition:
      lambda: return x == 1;
    then:
      - switch.turn_on: switch_up
```

Unknown values have no matching action.

- [ ] **Step 2: Verify GREEN**

Run: `uv run python -m unittest tests/test_configurations.py -v`

- [ ] **Step 3: Validate and compile pass-through targets**

Run `esphome config` and `esphome compile` for the ESP32 and ESP8266
pass-through fixtures.

### Task 3: Full verification and commit

- [ ] **Step 1: Run all configuration tests**

Run: `uv run python -m unittest tests/test_configurations.py -v`

- [ ] **Step 2: Run pre-commit**

Run: `uv run pre-commit run --all-files`

- [ ] **Step 3: Commit**

```bash
git commit -m "fix: route pass-through keypad commands"
```
