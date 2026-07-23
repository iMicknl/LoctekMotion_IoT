# Raspberry Pi (FlexiSpot E7 / HS13B-1)

A standalone Python script and CLI to control a **FlexiSpot E7** desk directly from a
**Raspberry Pi**, without ESPHome or Home Assistant. It talks straight to the desk's
control box over its RJ45 serial port.

> [!WARNING]
> Be careful. Tinkering with electronics can be risky. Use this guide at your own risk.

> [!NOTE]
> For Home Assistant integration, the [ESP32 + ESPHome path](../README.md) documented in the
> main README is simpler to maintain and gives you native Home Assistant entities, OTA
> updates, etc. for free. Use this Raspberry Pi script if you specifically want a
> standalone script/CLI, or already have a Pi you want to reuse.

## Is this for my desk?

This script is written for the **HS13B-1** control panel, which is what a FlexiSpot E7 ships
with (tested control box: `CB38M2J(IB)-1`, per the [main README](../README.md#hs13b-1)). The
command protocol itself is the same across LoctekMotion control panels; only the RJ45 pinout
below is specific to HS13B-1/E7. If your control box PCB is printed with a different model
number, check the [Control Panels section](../README.md#control-panels) of the main README for
its pinout before wiring.

## Hardware / pinout

The control box exposes GND, TX, RX and PIN 20 (a "wake up the display / enable serial"
signal) on its RJ45 port. Do **not** connect RJ45 pin 8 (+5V) to the Raspberry Pi — the Pi has
its own power supply, and back-feeding it is unnecessary and risky.

| RJ45 pin (control box) | Signal            | Connect to Raspberry Pi (physical pin) | BCM GPIO |
| ----------------------- | ----------------- | --------------------------------------- | -------- |
| 7                        | GND                | Pin 6 (GND)                              | –        |
| 5                        | RX                 | Pin 8 (UART TXD)                         | GPIO 14  |
| 6                        | TX                 | Pin 10 (UART RXD)                        | GPIO 15  |
| 4                        | PIN 20             | Pin 12                                   | GPIO 18  |
| 8                        | +5V (VDD)          | **Do not connect**                       | –        |
| 1, 2, 3                  | RESET, SWIM, EMPTY | Not used                                 | –        |

See the main README's [RJ45 connector layout](../images/RJ-45_connector.jpg) and
[T568B color reference](../images/RJ45-Pinout-T568B.jpg) if you're wiring with a cut ethernet
cable, or use an RJ45-to-screw-terminal breakout board with jumper wires.

Note: an earlier prototype of a Raspberry Pi script existed in [`../archive/raspberry-pi`](../archive/raspberry-pi/),
but its GPIO pin constant (`PIN_20 = 12`, i.e. BCM GPIO 12) didn't actually match its own wiring
table (physical pin 12 = BCM GPIO 18). This script uses GPIO 18, matching the wiring table above.

## Prerequisites

- Raspberry Pi with GPIO header and Raspberry Pi OS (or similar Linux).
- Python 3.9+.

## Setup

### 1. Enable the hardware UART

Run `sudo raspi-config`, go to **Interface Options → Serial Port**, then:
- "Would you like a login shell to be accessible over serial?" → **No**
- "Would you like the serial port hardware to be enabled?" → **Yes**

Reboot afterwards. This frees up `/dev/ttyS0` (GPIO14/15) for the script instead of the Linux
console.

### 2. Wire the desk to the Pi

Connect the desk's control box to the Raspberry Pi as described in the [pinout table](#hardware--pinout)
above, with the Pi powered off.

### 3. Set up a Python virtual environment

A virtual environment (venv) is used here so the project's dependencies (`pyserial`,
`RPi.GPIO`) are installed in an isolated environment instead of system-wide. This also avoids
conflicts with Raspberry Pi OS's "externally managed environment" protection (PEP 668), which
otherwise blocks a plain `pip install` outside of a venv.

```bash
cd raspberry_pi
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

> [!NOTE]
> On a Raspberry Pi 5, `RPi.GPIO` doesn't support the newer GPIO chip. Install the
> API-compatible drop-in replacement instead: `pip install rpi-lgpio` instead of `RPi.GPIO`
> (keep the `pyserial` line as-is in `requirements.txt`, just swap the GPIO package).

You'll need to re-run `source venv/bin/activate` in any new terminal session before using the
script.

### 4. Test your wiring first (read-only, safe)

Before sending any command that could move the desk, verify your wiring with
[`test_wiring.py`](test_wiring.py). It only ever sends the Wake Up command (which just
enables the display/serial interface for a few seconds, exactly like touching a key on the
physical keypad - it never moves the desk), then listens and reports what it received:

```bash
python3 test_wiring.py
```

Example output when wired correctly:

```
✅ Decoded 1 valid height reading(s), last: 91.5 cm
RX, TX, GND and PIN20 all look correctly wired.
```

If nothing is received, or bytes arrive but no height is decoded, the script prints a
checklist (RX/TX swapped, missing GND, PIN20 wiring, `raspi-config` serial settings, wrong
serial device, control box unpowered) to help you debug before trying `up`/`down`/presets.

Run `python3 test_wiring.py --no-wake` to listen completely passively without sending
anything at all (useful e.g. to check for keypad-originated traffic on a pass-through wiring).

### 5. Try it

Once `test_wiring.py` confirms your wiring is correct:

```bash
python3 flexispot_e7.py wake
python3 flexispot_e7.py height
python3 flexispot_e7.py up --duration 2
```

## Usage

```
python3 flexispot_e7.py [--port /dev/ttyS0] [--pin20 18] <command> [options]
```

| Command                        | Description                                                        |
| ------------------------------- | -------------------------------------------------------------------- |
| `up [--duration SECONDS]`       | Move the desk up. Runs until Ctrl+C if `--duration` is omitted.      |
| `down [--duration SECONDS]`     | Move the desk down. Runs until Ctrl+C if `--duration` is omitted.    |
| `preset {1,2,3,4}`               | Recall a stored preset.                                              |
| `sit`                            | Recall preset 4 ("Sit" on most panels).                              |
| `stand`                          | Recall preset 3 ("Stand" on most panels).                            |
| `memory`                         | Send the Memory (M) command (used together with a preset key on the physical keypad to store the current height). |
| `alarm`                          | Toggle the desk's alarm.                                             |
| `child-lock`                     | Toggle child lock. *(Note: on the wire this currently uses the same command bytes as `memory` — see comment in `flexispot_e7.py`.)* |
| `wake`                           | Send the Wake Up command (wakes the display / enables height reporting for a few seconds). |
| `height [--watch]`               | Read the current height once, or continuously with `--watch` (Ctrl+C to stop). `--watch` automatically resends Wake Up every few seconds, since the desk only broadcasts height for a short time after each Wake Up. |
| `goto HEIGHT_CM [--min] [--max]` | Move the desk to a target height in cm.                              |

Examples:

```bash
python3 flexispot_e7.py preset 1
python3 flexispot_e7.py stand
python3 flexispot_e7.py height --watch
python3 flexispot_e7.py goto 100
```

### Known limitations

- There is no explicit "stop" command byte in this protocol — the desk simply stops moving
  shortly after it stops receiving Up/Down packets, which is how `up`/`down`/`goto` work here.
- Like the ESPHome `number` entity described in the [main README](../README.md#known-issues),
  `goto` may slightly overshoot the target height due to reporting delays. Use the physical
  presets (`preset`, `sit`, `stand`) for precise, repeatable positioning.
- This script assumes your control box has a spare RJ45 port for a direct connection. If your
  desk only has one RJ45 port (shared with the keypad), you'll need a pass-through wiring setup
  like the one described for ESPHome in the [main README](../README.md#pass-through-configuations);
  this script doesn't currently include keypad pass-through decoding.

## Using it as a library

```python
from flexispot_e7 import FlexispotE7Desk

with FlexispotE7Desk() as desk:
    desk.stand()
    height = desk.get_height()
    print(height)
```
