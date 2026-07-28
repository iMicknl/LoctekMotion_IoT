# Reliability Test Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make CI compile the real published packages and both local UART components on every supported test target.

**Architecture:** Published packages gain a source substitution whose production default is unchanged. ESP32 fixtures include the production packages with local overrides; ESP8266 fixtures directly instantiate both components for compatibility coverage.

**Tech Stack:** ESPHome 2026.6.5, YAML packages, Python `unittest`, GitHub Actions.

## Global Constraints

- Production component source remains `github://iMicknl/LoctekMotion_IoT`.
- No desk entity behavior changes belong in this branch.
- Commands and checks run inside the dev container.
- Push the branch, but create no PR, comment, or other GitHub post.

---

### Task 1: Add an executable fixture contract

**Files:**
- Create: `tests/test_configurations.py`
- Modify: `.github/workflows/test.yaml`

**Interfaces:**
- Consumes: the six ESPHome YAML paths already used by CI.
- Produces: `python -m unittest tests/test_configurations.py`, failing when a pass-through fixture omits the command component.

- [ ] **Step 1: Write the failing test**

Create a table-driven `unittest.TestCase` that runs
`esphome config <fixture>` with `subprocess.run(check=True, capture_output=True,
text=True)` and asserts the resolved output for both pass-through fixtures
contains `platform: loctekmotion_desk_command`.

- [ ] **Step 2: Verify the test fails for the empty fixtures**

Run: `uv run python -m unittest tests/test_configurations.py -v`

Expected: both pass-through cases fail because the resolved configurations do
not contain `loctekmotion_desk_command`.

- [ ] **Step 3: Add the unittest command to CI**

Add `uv run python -m unittest tests/test_configurations.py -v` before the
matrix validation step.

### Task 2: Make production packages locally testable

**Files:**
- Modify: `packages/office-desk-esp32.yaml`
- Modify: `packages/office-desk-esp32-passthrough.yaml`
- Modify: `tests/office-desk-esp32.yaml`
- Modify: `tests/office-desk-esp32-passthrough.yaml`

**Interfaces:**
- Consumes: substitution `external_components_source`.
- Produces: production defaults pointing at GitHub and test overrides pointing at `../components`.

- [ ] **Step 1: Add the source substitution**

Add:

```yaml
external_components_source: github://iMicknl/LoctekMotion_IoT
```

and change each package source to:

```yaml
source: ${external_components_source}
```

- [ ] **Step 2: Replace ESP32 copies with package consumers**

Each ESP32 test supplies literal test secrets, pin substitutions, and:

```yaml
external_components_source: ../components

packages:
  desk: !include ../packages/<package-file>.yaml
```

- [ ] **Step 3: Verify the fixture contract is green**

Run: `uv run python -m unittest tests/test_configurations.py -v`

Expected: both pass-through assertions pass.

### Task 3: Add real ESP8266 component compatibility fixtures

**Files:**
- Modify: `tests/office-desk-esp8266.yaml`
- Modify: `tests/office-desk-esp8266-passthrough.yaml`

**Interfaces:**
- Consumes: local `../components`, ESP8266 UART pins.
- Produces: compile targets that instantiate height alone and height plus command.

- [ ] **Step 1: Reduce the regular fixture to the real height boundary**

Configure ESP8266, logger, one UART, local external components, and a
`loctekmotion_desk_height` sensor.

- [ ] **Step 2: Build the pass-through fixture**

Configure two UART receivers and instantiate both
`loctekmotion_desk_height` and `loctekmotion_desk_command` with explicit
`uart_id` values.

- [ ] **Step 3: Validate all fixtures**

Run:

```bash
for config in tests/office-desk-*.yaml; do
  uv run esphome config "$config"
done
```

Expected: four valid configurations.

### Task 4: Full verification and commit

**Files:**
- All files changed in Tasks 1–3.

- [ ] **Step 1: Compile the four-target matrix**

Run:

```bash
for config in tests/office-desk-*.yaml; do
  uv run esphome compile "$config"
done
```

Expected: four successful firmware builds.

- [ ] **Step 2: Validate published packages**

Run `uv run esphome config` for both files under `packages/`.

- [ ] **Step 3: Run repository checks**

Run: `uv run pre-commit run --all-files`

- [ ] **Step 4: Commit**

Stage only the intended files and commit:

```bash
git commit -m "test: exercise real ESPHome packages"
```
