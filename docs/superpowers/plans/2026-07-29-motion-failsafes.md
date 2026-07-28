# Motion Failsafes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bound every recurring movement command and reject positional movement without trustworthy feedback.

**Architecture:** Restartable start scripts impose wake-before-move ordering. A separate failsafe script owns maximum runtime, while Stop cancels pending work and disables both UART switches.

**Tech Stack:** ESPHome scripts, template cover and number automations, package configuration tests.

## Global Constraints

- Default maximum movement duration is 30 seconds.
- Polling, preset mapping, and target-tolerance behavior stay unchanged.
- Apply equivalent behavior to normal and pass-through packages.
- Push the branch without opening or commenting on a PR.

---

### Task 1: Specify observable safety contracts

**Files:**
- Modify: `tests/test_configurations.py`

**Interfaces:**
- Consumes: resolved standard and pass-through package configurations.
- Produces: failing tests for a movement timeout, cancellable start scripts, and finite/in-range feedback guards.

- [ ] **Step 1: Add failing tests**

Parse the resolved configurations and assert:

- `movement_timeout` resolves to `30s`;
- start-up and start-down scripts exist;
- a failsafe script delays by the configured timeout then disables both
  switches;
- Stop cancels both start scripts before disabling switches;
- both position actions include a timeout;
- the number action checks `has_state()`, `std::isfinite`, and configured bounds.

- [ ] **Step 2: Verify RED**

Run: `uv run python -m unittest tests/test_configurations.py -v`

Expected: safety contract assertions fail because no scripts or timeouts exist.

### Task 2: Introduce ordered, cancellable movement scripts

**Files:**
- Modify: `packages/office-desk-esp32.yaml`
- Modify: `packages/office-desk-esp32-passthrough.yaml`

**Interfaces:**
- Produces: `start_moving_up`, `start_moving_down`, and
  `movement_failsafe` scripts.

- [ ] **Step 1: Add `movement_timeout: 30s`**

Add the substitution to both packages.

- [ ] **Step 2: Add direction scripts**

Each direction script uses `mode: restart`, stops the opposite pending script
and switch, sends wake, waits 200 ms, starts its UART switch, and restarts the
failsafe.

- [ ] **Step 3: Add the failsafe**

The failsafe delays `${movement_timeout}` and then turns off both movement
switches.

- [ ] **Step 4: Route cover and keypad starts through scripts**

Replace direct movement switch starts with `script.execute`.

### Task 3: Make Stop and positional movement safe

**Files:**
- Modify: both published package files.

- [ ] **Step 1: Harden Stop**

Stop both direction scripts and the failsafe before turning off both switches.

- [ ] **Step 2: Guard number movement**

Before choosing a direction, require:

```cpp
id(desk_height).has_state() &&
std::isfinite(id(desk_height).state) &&
id(desk_height).state >= float(${min_height}) &&
id(desk_height).state <= float(${max_height})
```

Log a warning and invoke Stop when the guard fails.

- [ ] **Step 3: Bound feedback waits**

Add `timeout: ${movement_timeout}` to cover and number `wait_until` actions.
Keep `cover.stop` immediately after every wait so it runs on success or timeout.

- [ ] **Step 4: Verify GREEN**

Run the safety contract tests.

### Task 4: Full verification and commit

- [ ] **Step 1: Validate and compile all four fixtures**
- [ ] **Step 2: Validate both production packages**
- [ ] **Step 3: Run all unittests and pre-commit hooks**
- [ ] **Step 4: Commit**

```bash
git commit -m "fix: add desk movement failsafes"
```
