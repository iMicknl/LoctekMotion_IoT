#include "loctek.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace loctek {

static const char *const TAG = "loctek";

void LoctekComponent::loop() {
  while (available() > 0)
  {
    byte incomingByte = read();
    // ESP_LOGD("DEBUG", "Incoming byte is: %08x", incomingByte);
    
    // First byte, start of a packet
    if (incomingByte == 0x9b)
    {
      // Reset message length
      this->msg_len_ = 0;
      this->valid_ = false;
    }

    // Second byte defines the message length
    if (this->history_[0] == 0x9b)
    {
      this->msg_len_ = (int) incomingByte;
    }

    // Third byte is message type
    if (this->history_[1] == 0x9b)
    {
      this->msg_type_ = incomingByte;
    }

    // Fourth byte is first height digit, if msg type 0x12 & msg len 7
    if (this->history_[2] == 0x9b)
    {

      if (this->msg_type_ == 0x12 && (this->msg_len_ == 7 || this->msg_len_ == 10))
      {
        // Empty height
        if (incomingByte == 0)
        {
            //ESP_LOGD("DEBUG", "Height 1 is EMPTY -> 0x%02x", incomingByte);
            //deskSerial.write(command_wakeup, sizeof(command_wakeup));
        }
        else if (this->hex_to_int(incomingByte) == 0)
        {
            //ESP_LOGD("DEBUG", "Invalid height 1 -> 0x%02x", incomingByte);
            //deskSerial.write(command_wakeup, sizeof(command_wakeup));
        }
        else
        {
          this->valid_ = true;
        //   ESP_LOGD("DEBUG", "Height 1 is: 0x%02x", incomingByte);
        }
      }
    }

    // Fifth byte is second height digit
    if (this->history_[3] == 0x9b)
    {
      if (this->valid_ == true)
      {
          //ESP_LOGD("DEBUG", "Height 2 is: 0x%02x", incomingByte);
      }
    }

    // Sixth byte is third height digit
    if (this->history_[4] == 0x9b)
    {
      if (this->valid_ == true)
      {
        int height1 = this->hex_to_int(this->history_[1]) * 100;
        int height2 = this->hex_to_int(this->history_[0]) * 10;
        int height3 = this->hex_to_int(incomingByte);
        if (height2 != 100)
        {
          float finalHeight = height1 + height2 + height3;
          if (this->is_decimal(this->history_[0]))
          {
            finalHeight = finalHeight / 10;
          }
          this->value_ = finalHeight;
          // ESP_LOGD("DeskHeightSensor", "Current height is: %f", finalHeight);
        }
      }
    }

    // Save byte buffer to history arrary
    this->history_[4] = this->history_[3];
    this->history_[3] = this->history_[2];
    this->history_[2] = this->history_[1];
    this->history_[1] = this->history_[0];
    this->history_[0] = incomingByte;

          // End byte
    if (incomingByte == 0x9d)
    {
      if (this->value_ && this->value_ != this->lastPublished_)
      {
        this->height_->publish_state(this->value_);
        this->lastPublished_ = this->value_;
      } 
    }
  } 
}

void LoctekComponent::dump_config() {

  ESP_LOGCONFIG(TAG, "Loctek:");

  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with Loctek failed!");
  }
  
  LOG_SENSOR("  ", "HEIGHT", this->height_);

  this->check_uart_settings(9600);
}

bool LoctekComponent::is_decimal(byte b) {
  return (b & 0x80) == 0x80;
}

int LoctekComponent::hex_to_int(byte s)
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

}  // namespace loctek
}  // namespace esphome