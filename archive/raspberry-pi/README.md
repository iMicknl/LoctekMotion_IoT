# Raspberry Pi

The Raspberry Pi is a low cost device which is found in many households. A powerful feature of the Raspberry Pi is the row of GPIO (general-purpose input/output) pins along the top edge of the board. These GPIO pins can be used to interact with the LoctekMotion control box.

In the end I believe the Raspberry Pi is overkill for this scenario, but if you have a spare one or if you want to experiment, give it a try. The Python script in this part is limited, but can easily be extended to fix your scenario.

## Prerequisites

- Raspberry Pi (with GPIO headers)
- Material to connect to the RJ45 port

## Installation

![GPIO Pinout Diagram](../../images/GPIO-Pinout-Diagram-2.png)

Connect the GND, TX, RX and PIN20 (see [README](../../README.md#control-panels)) to the Raspberry Pi. In the Python script we use the following pins, but you can adapt this to your wishes.

| Raspberry Pi | Loctek Motion |
| ------------ | ------------- |
| 6            | GND           |
| 8 (GPIO 14)  | RX            |
| 10 (GPIO 15) | TX            |
| 12 (GPIO 18) | PIN 20        |

## Install

Run `raspi-config` to turn on the serial connection. Go to Advanced Options -> Serial and enable the serial connection. You don't need to enable it for login.

`sudo apt-get install python3-rpi.gpio`

Browse to the folder where you placed the `flexispot.py` file.

`python3 -m venv venv`

`source venv/bin/activate`

`python3 flexispot.py up`

## Usage
Example usage: `python3 flexispot.py [COMMAND]`
- `python3 flexispot.py up`
- `python3 flexispot.py down`
- `python3 flexispot.py preset_1`
