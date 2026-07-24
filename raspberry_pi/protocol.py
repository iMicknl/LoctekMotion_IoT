"""Pure, hardware-independent LoctekMotion/FlexiSpot UART protocol logic.

No serial or GPIO access happens in this module - it only encodes/decodes
bytes. This lets debugging tools (see sniff.py) depend on nothing but
`pyserial`, without pulling in `RPi.GPIO`.

Frame format (see the "Execute a command" / "Retrieve current height"
sections of the main README.md):

    START(1) LENGTH(1) TYPE(1) PAYLOAD(N) CHECKSUM(2) END(1)

LENGTH counts itself plus everything up to (and including) the checksum,
i.e. LENGTH = 1(itself) + 1(type) + N(payload) + 2(checksum). So the total
frame size is 1(start) + LENGTH + 1(end), and the payload size is
LENGTH - 4. For example, the "Up" command `9b 06 02 01 00 fc a0 9d` has
LENGTH=6, giving a 2-byte payload (`01 00`) and 2-byte checksum (`fc a0`).
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Optional

START_BYTE = 0x9B
END_BYTE = 0x9D

# Message types seen in this protocol so far.
TYPE_COMMAND = 0x02  # commands sent to the control box (buttons, incl. keypad)
TYPE_HEIGHT = 0x12  # height broadcast from the control box

# Command bytes, as documented in the "Execute a command" section of the main
# README.md and used by packages/office-desk-esp32.yaml. These are identical
# across LoctekMotion control panels; only the RJ45 wiring differs per model.
COMMANDS: dict[str, bytes] = {
    "wake_up": bytes([0x9B, 0x06, 0x02, 0x00, 0x00, 0x6C, 0xA1, 0x9D]),
    "up": bytes([0x9B, 0x06, 0x02, 0x01, 0x00, 0xFC, 0xA0, 0x9D]),
    "down": bytes([0x9B, 0x06, 0x02, 0x02, 0x00, 0x0C, 0xA0, 0x9D]),
    "memory": bytes([0x9B, 0x06, 0x02, 0x20, 0x00, 0xAC, 0xB8, 0x9D]),
    "preset_1": bytes([0x9B, 0x06, 0x02, 0x04, 0x00, 0xAC, 0xA3, 0x9D]),
    "preset_2": bytes([0x9B, 0x06, 0x02, 0x08, 0x00, 0xAC, 0xA6, 0x9D]),
    "preset_3": bytes([0x9B, 0x06, 0x02, 0x10, 0x00, 0xAC, 0xAC, 0x9D]),  # Stand
    "preset_4": bytes([0x9B, 0x06, 0x02, 0x00, 0x01, 0xAC, 0x60, 0x9D]),  # Sit
    "alarm": bytes([0x9B, 0x06, 0x02, 0x40, 0x00, 0xAC, 0x90, 0x9D]),
    # Note: upstream packages/office-desk-esp32.yaml defines "Child lock" with
    # the exact same payload byte (0x20) as "Memory". That is not a typo made
    # here - it is how the control box command is documented upstream - but it
    # does mean the two commands are currently indistinguishable on the wire.
    "child_lock": bytes([0x9B, 0x06, 0x02, 0x20, 0x00, 0xAC, 0xB8, 0x9D]),
}

# Reverse lookup used by the sniffer to label frames that match a known command.
COMMAND_NAMES_BY_BYTES: dict[bytes, str] = {v: k for k, v in COMMANDS.items()}


# ========== SEVEN-SEGMENT DECODING ==========
# Ported from components/loctekmotion_desk_height/desk_height_sensor.cpp, which
# is more complete than the older archive/raspberry-pi/flexispot.py decoder
# (it also handles the 10-byte message variant and the "no digit"/hyphen case).

_SEGMENT_TO_DIGIT = {
    0b0111111: 0,
    0b0000110: 1,
    0b1011011: 2,
    0b1001111: 3,
    0b1100110: 4,
    0b1101101: 5,
    0b1111101: 6,
    0b0000111: 7,
    0b1111111: 8,
    0b1101111: 9,
    0b1000000: 10,  # hyphen / dash segment only
}


def decode_seven_segment(byte: int) -> tuple[int, bool]:
    """Decode a single 7-segment display byte into (digit, is_decimal_point).

    Returns digit -1 for a segment pattern that doesn't match any known digit.
    """
    is_decimal = bool(byte & 0x80)
    digit = _SEGMENT_TO_DIGIT.get(byte & 0x7F, -1)
    return digit, is_decimal


@dataclass
class HeightFrameParser:
    """Incrementally parses the UART byte stream for height-broadcast frames.

    Mirrors the state machine in desk_height_sensor.cpp: frames start with
    START_BYTE, the 2nd byte is the length, the 3rd byte is the type (0x12 for
    height broadcasts), followed by 3 seven-segment digit bytes, ending in
    END_BYTE. Height broadcasts only occur for a short time after a Wake Up
    command has been sent.
    """

    history: list[Optional[int]] = None
    msg_len: int = 0
    msg_type: int = 0
    valid: bool = False
    value: Optional[float] = None

    def __post_init__(self) -> None:
        self.history = [None] * 5

    def feed(self, byte: int) -> Optional[float]:
        """Feed a single incoming byte. Returns a newly decoded height, if any."""
        result = None

        if byte == START_BYTE:
            self.msg_len = 0
            self.valid = False

        if self.history[0] == START_BYTE:
            self.msg_len = byte

        if self.history[1] == START_BYTE:
            self.msg_type = byte

        if self.history[2] == START_BYTE:
            if self.msg_type == TYPE_HEIGHT and self.msg_len in (7, 10):
                digit, _ = decode_seven_segment(byte)
                # byte == 0 means the digit is blank (e.g. a suppressed leading
                # zero); digit <= 0 also excludes segment patterns that don't
                # match any known digit. Both cases keep `valid` False, same
                # as desk_height_sensor.cpp.
                if byte != 0 and digit > 0:
                    self.valid = True

        if self.history[4] == START_BYTE and self.valid:
            height1, _ = decode_seven_segment(self.history[1])
            height2, decimal2 = decode_seven_segment(self.history[0])
            height3, _ = decode_seven_segment(byte)
            if height2 != 10 and height1 >= 0 and height2 >= 0 and height3 >= 0:
                final_height = height1 * 100 + height2 * 10 + height3
                if decimal2:
                    final_height /= 10
                self.value = final_height
                result = final_height

        self.history = [byte] + self.history[:4]

        return result


@dataclass
class Frame:
    """A fully captured, generic protocol frame."""

    raw: bytes
    length_field: int
    msg_type: int
    payload: bytes
    checksum: bytes
    framing_ok: bool  # False if the frame didn't end with END_BYTE where expected
    known_command: Optional[str]  # name from COMMANDS if raw matches exactly, else None

    @property
    def raw_hex(self) -> str:
        return " ".join(f"{b:02x}" for b in self.raw)

    @property
    def payload_hex(self) -> str:
        return " ".join(f"{b:02x}" for b in self.payload)

    @property
    def payload_binary(self) -> str:
        return " ".join(f"{b:08b}" for b in self.payload)


class FrameSniffer:
    """Reassembles raw UART bytes into generic Frame objects.

    Unlike HeightFrameParser (which only extracts height broadcasts), this
    captures every frame - commands (type 0x02, e.g. keypad button presses),
    height broadcasts (type 0x12), or any other type - so you can inspect the
    length/type/payload/checksum of frames the protocol table doesn't
    document yet, e.g. to figure out what a specific button sends, or what a
    desk reports when it hits its physical top/bottom limit.
    """

    def __init__(self) -> None:
        self._buffer: bytearray = bytearray()
        self._expected_total_len: Optional[int] = None

    def feed(self, byte: int) -> Optional[Frame]:
        """Feed a single incoming byte. Returns a completed Frame, if any."""
        if byte == START_BYTE and not self._buffer:
            self._buffer.append(byte)
            return None

        if not self._buffer:
            # Not currently inside a frame and this isn't a start byte - ignore.
            return None

        self._buffer.append(byte)

        if len(self._buffer) == 2:
            # LENGTH counts itself + type + payload + checksum, so the total
            # frame size (incl. start/end) is 1 + length_field + 1.
            self._expected_total_len = 1 + byte + 1
            return None

        if self._expected_total_len is not None and len(self._buffer) >= self._expected_total_len:
            frame = self._finalize()
            self._buffer = bytearray()
            self._expected_total_len = None
            return frame

        return None

    def _finalize(self) -> Frame:
        raw = bytes(self._buffer)
        length_field = raw[1]
        msg_type = raw[2] if len(raw) > 2 else 0
        payload_len = max(length_field - 4, 0)
        payload = raw[3 : 3 + payload_len]
        checksum = raw[3 + payload_len : 3 + payload_len + 2]
        framing_ok = len(raw) >= 4 and raw[-1] == END_BYTE
        known_command = COMMAND_NAMES_BY_BYTES.get(raw)

        return Frame(
            raw=raw,
            length_field=length_field,
            msg_type=msg_type,
            payload=payload,
            checksum=checksum,
            framing_ok=framing_ok,
            known_command=known_command,
        )
