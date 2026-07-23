#!/usr/bin/env python3
"""Read-only wiring test for a FlexiSpot E7 / HS13B-1 Raspberry Pi setup.

Verifies your GND/RX/TX/PIN20 wiring is correct WITHOUT risking the desk:
this script never sends Up, Down, a preset, Memory, Alarm or Child Lock.
The only thing it ever sends is the Wake Up command, which - just like
touching any key on the physical keypad - only enables the control box's
display/serial interface for a few seconds and does not move the desk. It
does not need to be sent at all; see --no-wake below.

Usage:
    python3 test_wiring.py
    python3 test_wiring.py --no-wake         # listen passively, send nothing
    python3 test_wiring.py --duration 15
"""

from __future__ import annotations

import argparse
import sys

from flexispot_e7 import PIN20_GPIO, SERIAL_PORT, FlexispotE7Desk

import serial


def run_test(port: str, pin20: int, duration: float, send_wake: bool) -> int:
    print(f"Opening {port} at 9600 baud, PIN20 on BCM GPIO {pin20} ...")

    try:
        with FlexispotE7Desk(port=port, pin20=pin20) as desk:
            print(
                "Serial port opened and PIN20 set HIGH. This only enables the "
                "control box's display/serial interface, the same as touching "
                "a key on the physical keypad - it does not move the desk."
            )

            if send_wake:
                print("Sending Wake Up command (safe, does not move the desk) ...")
            else:
                print("--no-wake set: sending nothing, listening passively only.")

            print(f"Listening for {duration:.0f}s ...\n")

            result = desk.diagnostic_listen(duration=duration, send_wake=send_wake)
    except serial.SerialException as exc:
        print(f"\nERROR opening serial port: {exc}", file=sys.stderr)
        print(f"Check that {port} exists (some Pi models use /dev/serial0),")
        print("and that raspi-config has the serial *hardware* enabled.")
        return 1

    total_bytes = result["total_bytes"]
    raw_hex = result["raw_hex"]
    heights = result["heights"]

    print("--- Summary ---")
    print(f"Bytes received: {total_bytes}")
    if raw_hex:
        preview = " ".join(raw_hex[:64])
        suffix = " ..." if len(raw_hex) > 64 else ""
        print(f"Raw bytes (first {min(64, len(raw_hex))}): {preview}{suffix}")

    if heights:
        print(f"\n✅ Decoded {len(heights)} valid height reading(s), last: {heights[-1]} cm")
        print("RX, TX, GND and PIN20 all look correctly wired.")
        return 0
    elif total_bytes:
        print("\n⚠️  Received bytes, but no valid height frame was decoded.")
        print("RX and GND are likely wired correctly, but double-check:")
        print("  - PIN20 wiring (RJ45 pin 4) - the control box may not be fully awake")
        print("  - that you're testing within a few seconds of the Wake Up command")
        return 1
    else:
        print("\n❌ No bytes received at all.")
        print("Check:")
        print("  - RX and TX may be swapped (Pi TXD -> desk RX pin 5, Pi RXD <- desk TX pin 6)")
        print("  - GND is connected between the Pi and the control box")
        print("  - PIN20 wiring (RJ45 pin 4 -> BCM GPIO %d)" % pin20)
        print(
            "  - raspi-config: Interface Options -> Serial Port -> hardware "
            "enabled, login shell disabled, then reboot"
        )
        print(f"  - correct serial device ({port}); some Pi models use /dev/serial0")
        print("  - the control box is powered on")
        return 1


def main(argv: list | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", default=SERIAL_PORT, help=f"Serial port (default: {SERIAL_PORT})")
    parser.add_argument("--pin20", type=int, default=PIN20_GPIO, help=f"BCM GPIO pin wired to PIN 20 (default: {PIN20_GPIO})")
    parser.add_argument("--duration", type=float, default=8.0, help="Seconds to listen (default: 8)")
    parser.add_argument(
        "--no-wake",
        action="store_true",
        help="Never send anything, not even Wake Up - listen completely passively",
    )
    args = parser.parse_args(argv)

    return run_test(args.port, args.pin20, args.duration, send_wake=not args.no_wake)


if __name__ == "__main__":
    sys.exit(main())
