#include <SoftwareSerial.h>

#define displayPin20 4 // D2 GPIO4
#define rxPin 12 // D5 GPIO12
#define txPin 14 // D6 GPIO14 

SoftwareSerial sSerial(rxPin, txPin); // RX, TX
byte history[2];

// Supported Commands
byte wakeup[] = { 0x9b, 0x06, 0x02, 0x00, 0x00, 0x6c, 0xa1, 0x9d };
byte command_up[] = { 0x9b, 0x06, 0x02, 0x01, 0x00, 0xfc, 0xa0, 0x9d };
byte command_down[] = { 0x9b, 0x06, 0x02, 0x02, 0x00, 0x0c, 0xa0, 0x9d };
byte command_m[] =  {0x9b, 0x06, 0x02, 0x20, 0x00, 0xac, 0xb8, 0x9d };
byte command_preset_1[] = { 0x9b, 0x06, 0x02, 0x04, 0x00, 0xac, 0xa3, 0x9d };
byte command_preset_2[] = { 0x9b, 0x06, 0x02, 0x08, 0x00, 0xac, 0xa6, 0x9d };
byte command_preset_3[] = { 0x9b, 0x06, 0x02, 0x10, 0x00, 0xac, 0xac, 0x9d };
byte command_preset_4[] = { 0x9b, 0x06, 0x02, 0x00, 0x01, 0xac, 0x60, 0x9d };


void setup() {
  Serial.begin(115200);   // debug serial
  sSerial.begin(9600);    // Flexispot EK5
  
  // Turn desk in operating mode by setting controller pin20 to HIGH
  // This will allow us to send commands and to receive the current height
  Serial.println("Turn Operation Mode on");
  pinMode(displayPin20, OUTPUT);
  digitalWrite(displayPin20, LOW);

  // Run command
  Serial.println("Send command");
  sSerial.write(command_down, sizeof(command_down));

  // Wake up to get height
  sSerial.write(wakeup, sizeof(wakeup));
}

void loop() {
  while (sSerial.available())
    {
      byte in = sSerial.read(); 
      
      // Start of packet
      if(in == 0x9b) {
        Serial.println(); 
      }

      // Second byte defines the message length
      if(history[0] == 0x9b) {
        int msg_len = in;
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
