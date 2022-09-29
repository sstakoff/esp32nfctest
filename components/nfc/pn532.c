#include "pn532.h"
#include "i2c_wrap.h"
#include <string.h>

static int pn532_reset_pin_num = 0;
void pn532_set_reset_pin(int reset_pin_num) {
  pn532_reset_pin_num = reset_pin_num;

  ESP_ERROR_CHECK(gpio_set_direction(reset_pin_num, GPIO_MODE_OUTPUT));

}

void pn532_reset() {
  // Reset the PN532

  vTaskDelay(500 / portTICK_PERIOD_MS);

  gpio_set_level(pn532_reset_pin_num, 0);  // Pull pin low to reset
  vTaskDelay(1000 / portTICK_PERIOD_MS); // Hold for 1/2 sec
  gpio_set_level(pn532_reset_pin_num, 1); // Set back to high

  vTaskDelay(1000 / portTICK_PERIOD_MS); // Give it a sec to reset

}


void build_frame(uint8_t *pCmdBuf, size_t cmdLen, uint8_t *pFrameBuf, size_t *pFrameBufLen)
{
    const uint8_t preamble_and_start[] = { 0x00, 0x00, 0xff };
    memcpy(pFrameBuf, preamble_and_start, sizeof(preamble_and_start));

    pFrameBuf[3] = cmdLen + 1;  //LEN - includes the TFI
    pFrameBuf[4] = 256 - (cmdLen + 1);  // LCS
    pFrameBuf[5] = 0xd4; // TFI
    memcpy(pFrameBuf + 6, pCmdBuf, cmdLen);

    uint8_t dcs = 256 - 0xd4; // DCS
    for (size_t i = 0; i < cmdLen; ++i) {
      dcs -= pCmdBuf[i];
    }
    pFrameBuf[6 + cmdLen] = dcs;
    pFrameBuf[7 + cmdLen] = 0x00;

    *pFrameBufLen = 8 + cmdLen;
}

esp_err_t send_pn532_command(const uint8_t *pCmdBuf, size_t cmdBufLen) {
  // Create a frame to send

  static uint8_t frameBuf[256];
  size_t frameLen = 0;
  build_frame(pCmdBuf, cmdBufLen, &frameBuf, &frameLen);

  return i2c_write(&frameBuf, frameLen, 200);

}

void pn532_wake() {

  // The 532 can stretch the SCL clock up to 1ms after a wakeup.
  // Need to change the i2c timeout to allow for this

  int savedTimeout = get_i2c_timeout();
  set_i2c_timeout(2); // 2ms to be safe

  // Send a SAMConfiguration command - this is how you wake the 532
  // The second param 0x01 indicated Normal mode where no SAM is used
  uint8_t wakeupCommand[] = {0x14, 0x01};
  ESP_ERROR_CHECK(send_pn532_command(*wakeupCommand, sizeof(wakeupCommand)));

  // Reset the timeout
  set_i2c_timeout(savedTimeout);

}
