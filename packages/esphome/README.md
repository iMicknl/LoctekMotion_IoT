# ESPHome (work in progress)

| ESP8266 | Loctek Motion |
| ------- | ------------- |
| GND     | GND           |
| D6      | RX            |
| D5      | TX            |
| D2      | PIN 20        |

If your board supports a 5V input, you could use the 5V provided by the control box to power your controller as well.

## Installation

Please refer to the [ESPHome documentation](https://esphome.io/guides/getting_started_command_line.html).

You can use `flexispot_ek5.yaml` as a boilerplate for your own implementation. This implementation has been created for the ESP8266 nodemcu, but can easily be adopted for other platforms.
