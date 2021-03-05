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

    def __init__(self, serial):
        self.serial = serial

    def execute_command(self, command_name: str):
        """Execute command"""
        command = SUPPORTED_COMMANDS.get(command_name)

        if not command:
            raise Exception("Command not found")
        
        self.serial.write(command)

