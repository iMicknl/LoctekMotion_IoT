#!/usr/bin/env python3
"""Passive UART sniffer for reverse-engineering LoctekMotion/FlexiSpot button presses.

This script NEVER writes to the serial port and NEVER touches any GPIO pin -
it only opens the serial connection for reading. Run it, then press buttons
on the physical keypad (or move the desk to its top/bottom limit) and watch
which frames show up, to figure out:

  - which payload byte/bitmask corresponds to which physical button
  - what the height broadcast looks like as you move the desk, including the
    minimum and maximum height your desk actually reaches
  - what (if anything) gets sent when the desk hits its physical top/bottom limit

Because it only depends on `pyserial` (no `RPi.GPIO`), you can also run this
on a plain laptop with a USB-serial/RS232 adapter if you'd rather not tie up
the Raspberry Pi while reverse-engineering a non-standard control panel.

Wiring note: to see the bytes a KEYPAD BUTTON sends, your RX line needs to be
connected to whichever wire actually carries keypad -> control box traffic.
If your desk's control box only has a single RJ45 port (shared with the
keypad), tap into that line (pass-through wiring, see the main README's
"Pass-Through Configurations" section) - listening on the control box's own
spare port may only show you its own broadcasts (e.g. height), not what the
keypad sends to it.

Usage:
    python3 sniff.py                  # run until Ctrl+C, print every frame
    python3 sniff.py --log capture.txt  # also append everything to a file
"""

from __future__ import annotations

import argparse
import sys
import time

import serial

from protocol import TYPE_HEIGHT, Frame, FrameSniffer, decode_seven_segment

DEFAULT_SERIAL_PORT = "/dev/ttyS0"
DEFAULT_BAUDRATE = 9600


def _decode_height_payload(frame: Frame) -> float | None:
    """Decode a height broadcast frame's payload (same validity rules as
    protocol.HeightFrameParser, applied directly to the already-sliced
    payload bytes instead of re-feeding them byte by byte)."""
    if len(frame.payload) < 3:
        return None

    d1_byte, d2_byte, d3_byte = frame.payload[0], frame.payload[1], frame.payload[2]
    if d1_byte == 0:
        return None  # blank / suppressed leading digit

    d1, _ = decode_seven_segment(d1_byte)
    if d1 <= 0:
        return None

    d2, decimal2 = decode_seven_segment(d2_byte)
    d3, _ = decode_seven_segment(d3_byte)
    if d2 == 10 or d1 < 0 or d2 < 0 or d3 < 0:
        return None

    height = d1 * 100 + d2 * 10 + d3
    if decimal2:
        height /= 10
    return height


def _describe_frame(frame: Frame) -> str:
    parts = [f"type=0x{frame.msg_type:02x}", f"len={frame.length_field}"]

    if frame.known_command:
        parts.append(f"KNOWN COMMAND: '{frame.known_command}'")

    if frame.msg_type == TYPE_HEIGHT:
        parts.append("(height broadcast)")

    if frame.payload:
        parts.append(f"payload={frame.payload_hex} ({frame.payload_binary})")

    if frame.checksum:
        parts.append(f"checksum={frame.checksum.hex()}")

    if not frame.framing_ok:
        parts.append("[!] did not end with the expected end byte - framing may be off")

    return f"raw=[{frame.raw_hex}] " + " ".join(parts)


def sniff(port: str, baud: int, duration: float | None, log_path: str | None) -> int:
    print(f"Opening {port} at {baud} baud, read-only (no writes, no GPIO) ...")
    try:
        ser = serial.Serial(port, baud, timeout=0.1)
    except serial.SerialException as exc:
        print(f"ERROR opening serial port: {exc}", file=sys.stderr)
        print(f"Check that {port} exists (some Pi models use /dev/serial0),")
        print("and that raspi-config has the serial *hardware* enabled.")
        return 1

    log_file = open(log_path, "a") if log_path else None

    print("Listening for frames. Press keypad buttons now, or move the desk")
    print("to its top/bottom limit to see what that reports. Ctrl+C to stop.\n")

    sniffer = FrameSniffer()
    frame_count = 0
    height_min: float | None = None
    height_max: float | None = None

    end_time = None if duration is None else time.monotonic() + duration

    try:
        while end_time is None or time.monotonic() < end_time:
            data = ser.read(1)
            if not data:
                continue

            frame = sniffer.feed(data[0])
            if frame is None:
                continue

            frame_count += 1
            timestamp = time.strftime("%H:%M:%S")
            line = f"[{timestamp}] #{frame_count} {_describe_frame(frame)}"
            print(line)
            if log_file:
                log_file.write(line + "\n")
                log_file.flush()

            if frame.msg_type == TYPE_HEIGHT:
                height = _decode_height_payload(frame)
                if height is not None:
                    height_min = height if height_min is None else min(height_min, height)
                    height_max = height if height_max is None else max(height_max, height)
                    print(f"           height={height} cm  (seen so far: min={height_min}, max={height_max})")
    except KeyboardInterrupt:
        print()
    finally:
        ser.close()
        if log_file:
            log_file.close()

    print("\n--- Summary ---")
    print(f"Frames captured: {frame_count}")
    if height_min is not None:
        print(f"Height range observed this run: {height_min} cm - {height_max} cm")
        print(
            "(Move the desk fully down then fully up while sniffing to find "
            "its true min/max - the range above only reflects what you moved through.)"
        )
    else:
        print("No height broadcasts observed. If you want height readings, press")
        print("a keypad button (or run flexispot_e7.py wake in another terminal)")
        print("to wake the display first - the control box only broadcasts height")
        print("for a few seconds after being woken.")

    return 0


def main(argv: list | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", default=DEFAULT_SERIAL_PORT, help=f"Serial port (default: {DEFAULT_SERIAL_PORT})")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUDRATE, help=f"Baud rate (default: {DEFAULT_BAUDRATE})")
    parser.add_argument("--duration", type=float, default=None, help="Seconds to listen (default: until Ctrl+C)")
    parser.add_argument("--log", default=None, help="Also append captured frames to this file")
    args = parser.parse_args(argv)

    return sniff(args.port, args.baud, args.duration, args.log)


if __name__ == "__main__":
    sys.exit(main())
