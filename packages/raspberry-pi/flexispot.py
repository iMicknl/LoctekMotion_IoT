import serial
import RPi.GPIO as GPIO

SERIAL_PORT = "/dev/ttyS0" # GPIO14 (TX) and GPIO15 (RX)
PIN_20 = 12 # GPIO 12

SUPPORTED_COMMANDS = {
    "up": bytearray(b'\x9b\x06\x02\x01\x00\xfc\xa0\x9d'),
    "down": bytearray(b'\x9b\x06\x02\x02\x00\x0c\xa0\x9d'),
    "m": bytearray(b'\x9b\x06\x02\x20\x00\xac\xb8\x9d'),
    "wake_up": bytearray(b'\x9b\x06\x02\x00\x00\x6c\xa1\x9d'),
    "preset_1": bytearray(b'\x9b\x06\x02\x04\x00\xac\xa3\x9d'),
    "preset_2": bytearray(b'\x9b\x06\x02\x08\x00\xac\xa6\x9d'),
    "preset_3": bytearray(b'\x9b\x06\x02\x10\x00\xac\xac\x9d'),
    "preset_4": bytearray(b'\x9b\x06\x02\x00\x01\xac\x60\x9d'),
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
    try:
        ser = serial.Serial(SERIAL_PORT, 9600, timeout=500)
        locktek = LoctekMotion(ser, PIN_20)
        locktek.execute_command("up")
    except serial.SerialException as e:
        print(e)
        return

if __name__ == "__main__":
    main()
