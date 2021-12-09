# Arduino

| ESP8266     | Loctek Motion |
| ----------- | ------------- |
| GND         | GND           |
| D5 (GPIO12) | RX            |
| D6 (GPIO14) | TX            |
| D2 (GPIO4)  | PIN 20        |

#### flexispot_ek5.ino

- outdated

#### flexispot_e8.ino

- tested with HS13A-1 + CB38M2L(IB)-1
- only works if HS13A-1 is wired like HS01B-1
- based on flexispot_ek5.ino
- 'unsigned long' replaced with 'const byte' to prevent compiler errors
- implements
  - demo()
  - turnon()
  - wake()
  - sit()
  - stand()
