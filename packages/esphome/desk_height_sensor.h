#include "esphome.h"
#include <bitset>

class DeskHeightSensor : public Component, public UARTDevice, public Sensor
{
public:
  DeskHeightSensor(UARTComponent *parent) : UARTDevice(parent) {}

  float value = 0;
  float lastPublished = -1;
  unsigned long history[5];

  int msg_len = 0;
  unsigned long msg_type;
  bool valid = false;

  float get_setup_priority() const override { return esphome::setup_priority::DATA; }

  int hex_to_int(byte s)
  {
    std::bitset<8> b(s);

    if (b[0] && b[1] && b[2] && b[3] && b[4] && b[5] && !b[6])
    {
      return 0;
    }
    if (not b[0] && b[1] && b[2] && !b[3] && !b[4] && !b[5] && !b[6])
    {
      return 1;
    }
    if (b[0] && b[1] && !b[2] && b[3] && b[4] && !b[5] && b[6])
    {
      return 2;
    }
    if (b[0] && b[1] && b[2] && b[3] && !b[4] && !b[5] && b[6])
    {
      return 3;
    }
    if (not b[0] && b[1] && b[2] && !b[3] && !b[4] && b[5] && b[6])
    {
      return 4;
    }
    if (b[0] && !b[1] && b[2] && b[3] && !b[4] && b[5] && b[6])
    {
      return 5;
    }
    if (b[0] && !b[1] && b[2] && b[3] && b[4] && b[5] && b[6])
    {
      return 6;
    }
    if (b[0] && b[1] && b[2] && !b[3] && !b[4] && !b[5] && !b[6])
    {
      return 7;
    }
    if (b[0] && b[1] && b[2] && b[3] && b[4] && b[5] && b[6])
    {
      return 8;
    }
    if (b[0] && b[1] && b[2] && b[3] && !b[4] && b[5] && b[6])
    {
      return 9;
    }
    if (!b[0] && !b[1] && !b[2] && !b[3] && !b[4] && !b[5] && b[6])
    {
      return 10;
    }
    return 0;
  }

  bool is_decimal(byte b)
  {
    return (b & 0x80) == 0x80;
  }

  void setup() override
  {
    // nothing to do here
  }

  void loop() override
  {
    while (available() > 0)
    {
      byte incomingByte = read();
      // ESP_LOGD("DEBUG", "Incoming byte is: %08x", incomingByte);
      
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

        if (msg_type == 0x12 && msg_len == 7)
        {
          // Empty height
          if (incomingByte == 0)
          {
             //ESP_LOGD("DEBUG", "Height 1 is EMPTY -> 0x%02x", incomingByte);
             //deskSerial.write(command_wakeup, sizeof(command_wakeup));
          }
          else if (hex_to_int(incomingByte) == 0)
          {
             //ESP_LOGD("DEBUG", "Invalid height 1 -> 0x%02x", incomingByte);
             //deskSerial.write(command_wakeup, sizeof(command_wakeup));
          }
          else
          {
            valid = true;
          //   ESP_LOGD("DEBUG", "Height 1 is: 0x%02x", incomingByte);
          }
        }
      }

      // Fifth byte is second height digit
      if (history[3] == 0x9b)
      {
        if (valid == true)
        {
           //ESP_LOGD("DEBUG", "Height 2 is: 0x%02x", incomingByte);
        }
      }

      // Sixth byte is third height digit
      if (history[4] == 0x9b)
      {
        if (valid == true)
        {
          int height1 = hex_to_int(history[1]) * 100;
          int height2 = hex_to_int(history[0]) * 10;
          int height3 = hex_to_int(incomingByte);
          if (height2 == 100) // check if 'number' is a hyphen, return value 10 multiplied by 10
          {
            
          }
          else
          {
            float finalHeight = height1 + height2 + height3;
            if (is_decimal(history[0]))
            {
              finalHeight = finalHeight / 10;
            }
            value = finalHeight;
            // ESP_LOGD("DeskHeightSensor", "Current height is: %f", finalHeight);
          }
        }
      }



      // Save byte buffer to history arrary
      history[4] = history[3];
      history[3] = history[2];
      history[2] = history[1];
      history[1] = history[0];
      history[0] = incomingByte;

           // End byte
      if (incomingByte == 0x9d)
      {
        if (value && value != lastPublished)
        {
          publish_state(value);
            lastPublished = value;
        } 
      }
    } 
  }
};
