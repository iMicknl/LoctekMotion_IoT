# Height Decoder Hardening Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Publish height only from complete, valid Loctek height frames.

**Architecture:** A header-only protocol boundary owns framing and seven-segment decoding without ESPHome dependencies. The component feeds bytes into it and publishes successful decoded frames.

**Tech Stack:** C++17, ESPHome UART component API, native assertion-based tests, GitHub Actions.

## Global Constraints

- Do not modify the command component or overlap PR #208.
- Preserve accepted message lengths 7 and 10 and duplicate suppression.
- Do not invent checksum validation without a documented algorithm.
- Push the branch without opening or commenting on a PR.

---

### Task 1: Define decoder behavior with native tests

**Files:**
- Create: `tests/height_decoder_test.cpp`
- Modify: `.github/workflows/test.yaml`

**Interfaces:**
- Consumes: `loctekmotion_desk_height::FrameReader` and `decode_height`.
- Produces: a native executable returning nonzero on a framing or decoding regression.

- [ ] **Step 1: Write failing tests**

Use literal frames and expected values. Include:

```cpp
{0x9B, 0x07, 0x12, 0x07, 0xED, 0x3F, 0x39, 0x28, 0x9D}
```

with expected height `75.0F`, plus invalid segments, wrong type, early
terminator, oversized length, leading noise, and back-to-back valid frames.

- [ ] **Step 2: Verify RED**

Run:

```bash
c++ -std=c++17 -I. tests/height_decoder_test.cpp -o /tmp/height_decoder_test
```

Expected: compilation fails because the production decoder API does not exist.

- [ ] **Step 3: Add the native command to CI**

Compile with warnings enabled and run the binary before firmware compilation:

```bash
c++ -std=c++17 -Wall -Wextra -Werror -I. tests/height_decoder_test.cpp -o /tmp/height_decoder_test
/tmp/height_decoder_test
```

### Task 2: Implement framing and digit decoding

**Files:**
- Create: `components/loctekmotion_desk_height/height_decoder.h`

**Interfaces:**
- Produces: fixed-capacity `FrameReader::push(uint8_t)` and
  `bool decode_height(const Frame &, float *)`.

- [ ] **Step 1: Implement minimal frame reading**

Start on `0x9B`, read the length byte, reject lengths exceeding capacity, and
complete only when `0x9D` occurs at index `length + 1`. A new `0x9B`
resynchronizes the reader.

- [ ] **Step 2: Implement exact segment lookup**

Mask the decimal bit and map literals `0x3F`, `0x06`, `0x5B`, `0x4F`, `0x66`,
`0x6D`, `0x7D`, `0x07`, `0x7F`, and `0x6F` to digits 0–9. Any other pattern
returns failure.

- [ ] **Step 3: Implement height decoding**

Require type `0x12`, length 7 or 10, decode payload bytes 3–5, and divide by 10
when byte 4 carries bit `0x80`.

- [ ] **Step 4: Verify GREEN**

Compile and run the native test with warnings treated as errors.

### Task 3: Integrate the tested decoder

**Files:**
- Modify: `components/loctekmotion_desk_height/desk_height_sensor.h`
- Modify: `components/loctekmotion_desk_height/desk_height_sensor.cpp`

**Interfaces:**
- Consumes: `FrameReader` and `decode_height`.
- Produces: ESPHome sensor states only for successful complete frames.

- [ ] **Step 1: Replace sliding history state**

Store a `FrameReader`, read all available UART bytes, and process only completed
frames.

- [ ] **Step 2: Preserve duplicate suppression**

Publish when decoding succeeds and the new value differs from
`last_published_`.

- [ ] **Step 3: Run native and firmware tests**

Run the native test, then compile all four ESPHome fixtures.

### Task 4: Full verification and commit

- [ ] **Step 1: Run all configuration unittests**
- [ ] **Step 2: Run the native decoder test**
- [ ] **Step 3: Validate and compile the full ESPHome matrix**
- [ ] **Step 4: Run pre-commit**
- [ ] **Step 5: Commit**

```bash
git commit -m "fix: validate desk height frames"
```
