// developed by foxytocin
// website https://anfuchs.de
// based on code from iMicknl

#include <SoftwareSerial.h>

#define displayPin20 4 // D2 GPIO4
#define rxPin 12       // D5 GPIO12
#define txPin 14       // D6 GPIO14

SoftwareSerial sSerial(rxPin, txPin); // RX, TX
byte history[2];

// Supported Commands
const byte wakeup[] = {0x9b, 0x06, 0x02, 0x00, 0x00, 0x6c, 0xa1, 0x9d};
const byte command_up[] = {0x9b, 0x06, 0x02, 0x01, 0x00, 0xfc, 0xa0, 0x9d};
const byte command_down[] = {0x9b, 0x06, 0x02, 0x02, 0x00, 0x0c, 0xa0, 0x9d};
const byte command_m[] = {0x9b, 0x06, 0x02, 0x20, 0x00, 0xac, 0xb8, 0x9d};
const byte command_preset_1[] = {0x9b, 0x06, 0x02, 0x04,
                                 0x00, 0xac, 0xa3, 0x9d};
const byte command_preset_2[] = {0x9b, 0x06, 0x02, 0x08,
                                 0x00, 0xac, 0xa6, 0x9d};
const byte command_preset_3[] = {0x9b, 0x06, 0x02, 0x10,
                                 0x00, 0xac, 0xac, 0x9d};
const byte command_preset_4[] = {0x9b, 0x06, 0x02, 0x00,
                                 0x01, 0xac, 0x60, 0x9d};

void setup() {
  Serial.begin(115200); // Debug serial
  sSerial.begin(9600);  // Flexispot E8

  pinMode(displayPin20, OUTPUT);
  digitalWrite(displayPin20, LOW);

  // Executes a demo
  demo();
}

void demo() {

  // Calls sit-preset and waits 20 seconds
  sit();
  delay(20000);

  // Calls stand-preset and waits 20 seconds
  stand();
  delay(20000);

  // Wakeup the table to retrieve the current height
  // At the moment this is only represented as HEX value
  wake();
}

void turnon() {
  // Turn desk in operating mode by setting controller pin20 to HIGH
  Serial.println("sending turn on command");
  digitalWrite(displayPin20, HIGH);
  delay(1000);
  digitalWrite(displayPin20, LOW);
}

void wake() {
  turnon();

  // This will allow us to receive the current height
  Serial.println("sending wakeup command");
  sSerial.flush();
  sSerial.enableTx(true);
  sSerial.write(wakeup, sizeof(wakeup));
  sSerial.enableTx(false);
}

void sit() {
  turnon();

  // This send the preset_4 command to trigger the sit position
  Serial.println("sending sit preset (preset_4)");
  sSerial.flush();
  sSerial.enableTx(true);
  sSerial.write(command_preset_4, sizeof(command_preset_4));
  sSerial.enableTx(false);
}

void stand() {
  turnon();

  // This send the preset_3 command to trigger the stand position
  Serial.println("sending stand preset (preset_3)");
  sSerial.flush();
  sSerial.enableTx(true);
  sSerial.write(command_preset_3, sizeof(command_preset_3));
  sSerial.enableTx(false);
}

void loop() {
  while (sSerial.available()) {
    unsigned long in = sSerial.read();

    // Start of packet
    if (in == 0x9b) {
      Serial.println();
    }

    // Second byte defines the message length
    if (history[0] == 0x9b) {
      int msg_len = (int)in;
      Serial.print("(LENGTH:");
      Serial.print(in);
      Serial.print(")");
    }

    // Get package length (second byte)
    history[1] = history[0];
    history[0] = in;

    // Print hex for debug
    Serial.print(in, HEX);
    Serial.print(" ");
  }
}
