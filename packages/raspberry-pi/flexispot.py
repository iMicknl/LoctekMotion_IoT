import serial
import RPi.GPIO as GPIO

SERIAL_PORT = "/dev/ttyS0" # GPIO14 (TX) and GPIO15 (RX)
PIN_20 = 12 # GPIO 12

SUPPORTED_COMMANDS = {
    "up": bytearray(b'\x9b\x06\x02\x01\x00\xfc\xa0\x9d'),
    "down": None,
    "m": None,
    "wake_up": None,
    "preset_1": None,
    "preset_2": None,
    "preset_3": None,
    "preset_4": None,
}

class LoctekMotion():

    def __init__(self, serial, pin_20):
        """Initialize LoctekMotion"""
        self.serial = serial

        # Or GPIO.BOARD - GPIO Numbering vs Pin numbering
        GPIO.setmode(GPIO.BCM)

        # Turn desk in operating mode by setting controller pin20 to HIGH
        # This will allow us to send commands and to receive the current height
        GPIO.setup(pin_20, GPIO.OUT)
        GPIO.output(pin_20, GPIO.HIGH)

    def execute_command(self, command_name: str):
        """Execute command"""
        command = SUPPORTED_COMMANDS.get(command_name)

        if not command:
            raise Exception("Command not found")
        
        self.serial.write(command)

    def current_height(self):
        """(not implemented yet)"""
        while ser.in_waiting:
            print ser.readline()

def main():
    serial = serial.Serial(SERIAL_PORT, 9600, timeout=500)
    loctek = LoctekMotion(serial, PIN_20)

    loctek.execute_command("up")

if __name__ == "__main__":
    main()