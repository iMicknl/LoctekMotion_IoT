# ESPHome

>ESPHome is a system to control your ESP8266/ESP32 by simple yet powerful configuration files and control them remotely through Home Automation systems.

| ESP8266 | Loctek Motion |
| ------- | ------------- |
| GND     | GND           |
| D6      | RX            |
| D5      | TX            |
| D2      | PIN 20        |

See [README](../../README.md#control-panels) for more details. If your board supports a 5V input, you could use the 5V provided by the control box to power your controller as well. 

## Installation

Please refer to the [ESPHome documentation](https://esphome.io/guides/getting_started_command_line.html).

You can use `flexispot_ek5.yaml` as a boilerplate for your own implementation. This implementation has been created for the ESP8266 nodemcu, but can easily be adopted for other platforms.

## Features

- Up to 4 presets as switch entities
- Up / Down controls via cover entity
- Current height via sensor entity
- M button via switch entity
- Wake up button via switch entity (currently just used for testing, doesn't seem functional yet)

## Screenshots
![ESPHome in Home Assistant](../../images/esphome.png)
