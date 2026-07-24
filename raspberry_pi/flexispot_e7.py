#!/usr/bin/env python3
"""Control a FlexiSpot E7 desk (HS13B-1 control panel) from a Raspberry Pi.

Talks directly to the desk's control box over the RJ45 serial port (UART,
9600 8N1) and drives PIN 20 via GPIO to keep the control box awake, using the
same command protocol documented in the repository's main README.md.

See raspberry_pi/README.md in this repository for wiring and setup instructions.
"""

from __future__ import annotations

import argparse
import sys
import time
from typing import Iterator, Optional

import RPi.GPIO as GPIO
import serial

from protocol import COMMANDS, HeightFrameParser

# ========== CONFIGURATION ==========

SERIAL_PORT = "/dev/ttyS0"
BAUDRATE = 9600

# BCM numbering. Physical pin 12 == BCM GPIO 18, wired to RJ45 pin 4 (PIN 20).
PIN20_GPIO = 18

# Defaults taken from packages/office-desk-esp32.yaml. Override per desk if needed.
MIN_HEIGHT_CM = 73.5
MAX_HEIGHT_CM = 123.0

# How often a movement command must be resent to keep the desk moving, matching
# the `send_every: 108ms` used by the UART switches in the ESPHome package. The
# desk stops on its own shortly after it stops receiving Up/Down packets - there
# is no explicit "stop" command byte.
MOVE_RESEND_INTERVAL_S = 0.108
WAKE_SCREEN_DELAY_S = 0.2

# The control box only broadcasts height for a few seconds after a Wake Up
# command. When watching height continuously, Wake Up is resent at this
# interval to keep the broadcast going.
WAKE_KEEPALIVE_INTERVAL_S = 3.0


# ========== DESK CONTROLLER ==========


class FlexispotE7Desk:
    """High-level controller for a FlexiSpot E7 desk via Raspberry Pi UART + GPIO."""

    def __init__(
        self,
        port: str = SERIAL_PORT,
        pin20: int = PIN20_GPIO,
        baud: int = BAUDRATE,
    ) -> None:
        self.port = port
        self.pin20 = pin20
        self.baud = baud
        self._serial: Optional[serial.Serial] = None

    def __enter__(self) -> "FlexispotE7Desk":
        self.open()
        return self

    def __exit__(self, *exc_info) -> None:
        self.close()

    def open(self) -> None:
        self._serial = serial.Serial(self.port, self.baud, timeout=0.05)

        GPIO.setmode(GPIO.BCM)
        GPIO.setup(self.pin20, GPIO.OUT)
        # Keep PIN 20 permanently HIGH, equivalent to the "Virtual screen"
        # switch with restore_mode: ALWAYS_ON in the ESPHome package - this
        # keeps the control box's serial interface enabled for the whole
        # session.
        GPIO.output(self.pin20, GPIO.HIGH)

    def close(self) -> None:
        if self._serial is not None:
            self._serial.close()
            self._serial = None
        GPIO.cleanup()

    def _require_serial(self) -> serial.Serial:
        if self._serial is None:
            raise RuntimeError(
                "Desk connection is not open, use 'with FlexispotE7Desk() as desk:'"
            )
        return self._serial

    def send_command(self, name: str) -> None:
        """Send one of the raw commands from COMMANDS by name."""
        command = COMMANDS.get(name)
        if command is None:
            raise ValueError(f"Unknown command: {name!r}")
        self._require_serial().write(command)

    def wake_up(self) -> None:
        """Send the software Wake Up command (in addition to the PIN 20 line)."""
        self.send_command("wake_up")

    def _move(self, command_name: str, duration: Optional[float]) -> None:
        """Repeatedly send an Up/Down command every MOVE_RESEND_INTERVAL_S.

        Runs for `duration` seconds, or until interrupted (Ctrl+C) if
        `duration` is None. The desk stops moving shortly after packets stop
        arriving - there's no explicit stop byte to send.
        """
        self.wake_up()
        time.sleep(WAKE_SCREEN_DELAY_S)

        end_time = None if duration is None else time.monotonic() + duration
        try:
            while end_time is None or time.monotonic() < end_time:
                self.send_command(command_name)
                time.sleep(MOVE_RESEND_INTERVAL_S)
        except KeyboardInterrupt:
            pass

    def move_up(self, duration: Optional[float] = None) -> None:
        """Move the desk up. Runs until `duration` elapses, or until Ctrl+C."""
        self._move("up", duration)

    def move_down(self, duration: Optional[float] = None) -> None:
        """Move the desk down. Runs until `duration` elapses, or until Ctrl+C."""
        self._move("down", duration)

    def preset(self, number: int) -> None:
        """Recall preset 1-4. Preset 3 is commonly "Stand", preset 4 "Sit",
        but this can vary per control panel."""
        if number not in (1, 2, 3, 4):
            raise ValueError("Preset number must be 1, 2, 3 or 4")
        self.wake_up()
        time.sleep(WAKE_SCREEN_DELAY_S)
        self.send_command(f"preset_{number}")

    def sit(self) -> None:
        """Alias for preset 4, labeled "Sit" in packages/office-desk-esp32.yaml."""
        self.preset(4)

    def stand(self) -> None:
        """Alias for preset 3, labeled "Stand" in packages/office-desk-esp32.yaml."""
        self.preset(3)

    def memory(self) -> None:
        """Send the Memory (M) command."""
        self.send_command("memory")

    def alarm(self) -> None:
        """Toggle the desk's alarm."""
        self.send_command("alarm")

    def child_lock(self) -> None:
        """Toggle child lock. Shares its command byte with `memory()` upstream."""
        self.send_command("child_lock")

    def read_height(self, timeout: Optional[float] = None) -> Iterator[float]:
        """Yield decoded height values (cm) as they're broadcast by the desk.

        The control box only broadcasts height for a short window after a
        Wake Up command, so call `wake_up()` first (or use `get_height()`).
        """
        ser = self._require_serial()
        parser = HeightFrameParser()
        end_time = None if timeout is None else time.monotonic() + timeout

        while end_time is None or time.monotonic() < end_time:
            data = ser.read(1)
            if not data:
                continue
            height = parser.feed(data[0])
            if height is not None:
                yield height

    def watch_height(self) -> Iterator[float]:
        """Yield height readings indefinitely, resending Wake Up periodically
        to keep the broadcast alive. Runs until interrupted (Ctrl+C) or the
        serial connection is closed."""
        ser = self._require_serial()
        parser = HeightFrameParser()
        last_wake = 0.0

        while True:
            now = time.monotonic()
            if now - last_wake >= WAKE_KEEPALIVE_INTERVAL_S:
                self.wake_up()
                last_wake = now

            data = ser.read(1)
            if not data:
                continue
            height = parser.feed(data[0])
            if height is not None:
                yield height

    def diagnostic_listen(self, duration: float = 8.0, send_wake: bool = True) -> dict:
        """Listen on the serial line for `duration` seconds for wiring diagnostics.

        This never sends a movement, preset, memory, alarm or child-lock
        command, so it cannot move the desk. If `send_wake` is True
        (default), only the Wake Up command is sent once at the start - the
        same safe, non-movement command normally used to enable the display
        and prompt a height broadcast. Set `send_wake=False` to listen
        completely passively (e.g. to check for keypad-originated traffic on
        a pass-through wiring, without sending anything at all).

        Returns a dict with `total_bytes`, `raw_hex` (list of hex byte
        strings, in the order received) and `heights` (list of decoded
        height readings, in the order decoded).
        """
        ser = self._require_serial()
        if send_wake:
            self.wake_up()

        parser = HeightFrameParser()
        total_bytes = 0
        raw_hex: list = []
        heights: list = []

        end_time = time.monotonic() + duration
        while time.monotonic() < end_time:
            data = ser.read(1)
            if not data:
                continue
            byte = data[0]
            total_bytes += 1
            raw_hex.append(f"{byte:02x}")
            height = parser.feed(byte)
            if height is not None:
                heights.append(height)

        return {"total_bytes": total_bytes, "raw_hex": raw_hex, "heights": heights}

    def get_height(self, timeout: float = 5.0) -> Optional[float]:
        """Wake the desk and return a single freshly-read height, or None on timeout."""
        self.wake_up()
        for height in self.read_height(timeout=timeout):
            return height
        return None

    def move_to_height(
        self,
        target_cm: float,
        tolerance: float = 0.3,
        timeout: float = 60.0,
        min_height: float = MIN_HEIGHT_CM,
        max_height: float = MAX_HEIGHT_CM,
    ) -> float:
        """Move the desk to `target_cm`, returning the last known height.

        Known limitation (see main README.md "Known issues"): the desk keeps
        moving until the reported height matches the target, which may
        overshoot slightly due to reporting delays. Use physical presets for
        precise, repeatable positioning.
        """
        target_cm = max(min_height, min(max_height, target_cm))
        ser = self._require_serial()

        self.wake_up()
        time.sleep(WAKE_SCREEN_DELAY_S)

        parser = HeightFrameParser()
        current: Optional[float] = None
        end_time = time.monotonic() + timeout
        last_send = 0.0

        # Interleave reading incoming height bytes with resending the
        # movement command every MOVE_RESEND_INTERVAL_S, since the desk stops
        # moving shortly after Up/Down packets stop arriving (see move_up()).
        while time.monotonic() < end_time:
            if current is not None and abs(current - target_cm) <= tolerance:
                break

            data = ser.read(1)
            if data:
                height = parser.feed(data[0])
                if height is not None:
                    current = height

            now = time.monotonic()
            if now - last_send >= MOVE_RESEND_INTERVAL_S:
                direction = "up" if current is None or target_cm > current else "down"
                self.send_command(direction)
                last_send = now

        if current is None:
            raise TimeoutError("Could not read the current desk height")

        return current


# ========== CLI ==========


def _build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Control a FlexiSpot E7 desk from a Raspberry Pi."
    )
    parser.add_argument("--port", default=SERIAL_PORT, help=f"Serial port (default: {SERIAL_PORT})")
    parser.add_argument(
        "--pin20", type=int, default=PIN20_GPIO, help=f"BCM GPIO pin wired to PIN 20 (default: {PIN20_GPIO})"
    )

    subparsers = parser.add_subparsers(dest="action", required=True)

    up_parser = subparsers.add_parser("up", help="Move the desk up")
    up_parser.add_argument("--duration", type=float, default=None, help="Seconds to move (default: until Ctrl+C)")

    down_parser = subparsers.add_parser("down", help="Move the desk down")
    down_parser.add_argument("--duration", type=float, default=None, help="Seconds to move (default: until Ctrl+C)")

    preset_parser = subparsers.add_parser("preset", help="Recall a preset")
    preset_parser.add_argument("number", type=int, choices=[1, 2, 3, 4])

    subparsers.add_parser("sit", help="Recall the Sit preset (preset 4)")
    subparsers.add_parser("stand", help="Recall the Stand preset (preset 3)")
    subparsers.add_parser("memory", help="Send the Memory (M) command")
    subparsers.add_parser("alarm", help="Toggle the desk alarm")
    subparsers.add_parser("child-lock", help="Toggle child lock")
    subparsers.add_parser("wake", help="Send the Wake Up command")

    height_parser = subparsers.add_parser("height", help="Read the current desk height")
    height_parser.add_argument("--watch", action="store_true", help="Keep printing height updates")

    goto_parser = subparsers.add_parser("goto", help="Move the desk to a target height in cm")
    goto_parser.add_argument("height_cm", type=float)
    goto_parser.add_argument("--min", type=float, default=MIN_HEIGHT_CM, dest="min_height")
    goto_parser.add_argument("--max", type=float, default=MAX_HEIGHT_CM, dest="max_height")

    return parser


def main(argv: Optional[list[str]] = None) -> int:
    args = _build_arg_parser().parse_args(argv)

    try:
        with FlexispotE7Desk(port=args.port, pin20=args.pin20) as desk:
            if args.action == "up":
                desk.move_up(duration=args.duration)
            elif args.action == "down":
                desk.move_down(duration=args.duration)
            elif args.action == "preset":
                desk.preset(args.number)
            elif args.action == "sit":
                desk.sit()
            elif args.action == "stand":
                desk.stand()
            elif args.action == "memory":
                desk.memory()
            elif args.action == "alarm":
                desk.alarm()
            elif args.action == "child-lock":
                desk.child_lock()
            elif args.action == "wake":
                desk.wake_up()
            elif args.action == "height":
                if args.watch:
                    try:
                        for height in desk.watch_height():
                            print(f"Height: {height} cm", end="\r", flush=True)
                    except KeyboardInterrupt:
                        print()
                else:
                    height = desk.get_height()
                    if height is None:
                        print("Timed out waiting for a height reading.")
                        return 1
                    print(f"Height: {height} cm")
            elif args.action == "goto":
                height = desk.move_to_height(
                    args.height_cm, min_height=args.min_height, max_height=args.max_height
                )
                print(f"Reached height: {height} cm")
    except serial.SerialException as exc:
        print(f"Serial error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
