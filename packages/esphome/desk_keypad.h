#include "esphome.h"
#include "esphome.h"
#include <bitset>

class DeskKeypad : public Component, public UARTDevice, public Sensor
{

public:
  DeskKeypad(UARTComponent *parent) : UARTDevice(parent) {}

  enum Command { Up = 1, Down = 2, Preset1 = 3 , Preset2 = 4 , Preset3 = 5, M = 6, Alarm = 7, Empty = 8};
  Command mReturnCommand;
  Command lastPublished = Command::Empty;
  unsigned long history[3];

  int msg_len = 0;
  unsigned long msg_type;
  bool valid = false;

  float get_setup_priority() const override { return esphome::setup_priority::DATA; }

  void setup() override
  {
    // nothing to do here
  }

   void loop() override
  {
    while (available() > 0)
    {
      byte incomingByte = read();
      //ESP_LOGD("DEBUG", "Incoming byte is: %08x", incomingByte);
      // First byte, start of a packet
      if (incomingByte == 0x9b)
      {
        // Reset message length
        msg_len = 0;
        valid = false;
      }

      // Second byte defines the message length
      if (history[0] == 0x9b)
      {
        msg_len = (int)incomingByte;
      }

      // Third byte is message type
      if (history[1] == 0x9b)
      {
        msg_type = incomingByte;
      }

      // Fourth byte is first height digit, if msg type 0x12 & msg len 7
      if (history[2] == 0x9b)
      {
        switch(incomingByte)
        {
          case 0x00: mReturnCommand = Command::Empty; break;
          case 0x01: mReturnCommand = Command::Up; break;
          case 0x02: mReturnCommand = Command::Down; break;
          case 0x04: mReturnCommand = Command::Preset1; break;
          case 0x08: mReturnCommand = Command::Preset2; break;
          case 0x10: mReturnCommand = Command::Preset3; break;
          case 0x20: mReturnCommand = Command::M; break;
          case 0x40: mReturnCommand = Command::Alarm; break;
        }
      }

        if (incomingByte == 0x9d && msg_type == 0x02 && msg_len == 6 && mReturnCommand && mReturnCommand != lastPublished)
        {
          publish_state(mReturnCommand);
          lastPublished = mReturnCommand;
        }

        // Save byte buffer to history arrary
        history[2] = history[1];
        history[1] = history[0];
        history[0] = incomingByte;
    }
  }
};