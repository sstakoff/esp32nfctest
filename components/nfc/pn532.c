#include "pn532.h"
#include "i2c_wrap.h"
#include <string.h>
#include <sys/time.h>
#include <sys/param.h>
#include "esp_log.h"

static char *TAG = "pn532.c";

static int pn532_reset_pin_num = 0;
static struct timeval __transaction_stop;

void pn532_set_reset_pin(int reset_pin_num) {
  pn532_reset_pin_num = reset_pin_num;

  ESP_ERROR_CHECK(gpio_set_direction(reset_pin_num, GPIO_MODE_OUTPUT));

}

void pn532_bus_delay() {
  // Ensure that the i2c bus has had enough time since the last i2c_stop.
  // See section 12.25 of the pnc532 datasheet. The min time between the
  // last i2c_stop and the next i2c_start is 1.3ms. We will use 5ms

  struct timeval now;
  gettimeofday(&now, NULL);

  time_t now_msec = now.tv_sec * 1000L + (now.tv_usec / 1000);
  time_t stop_time_msec = __transaction_stop.tv_sec * 1000L + (__transaction_stop.tv_usec / 1000);

  time_t delay_needed_ms = 5 - (now_msec - stop_time_msec);
  if (delay_needed_ms > 0) {
    printf("Delaying i2c bus for %ld ms", delay_needed_ms);
    vTaskDelay(delay_needed_ms / portTICK_PERIOD_MS);
  }
}

void pn532_reset() {
  // Reset the PN532

  vTaskDelay(500 / portTICK_PERIOD_MS);

  gpio_set_level(pn532_reset_pin_num, 0);  // Pull pin low to reset
  vTaskDelay(1000 / portTICK_PERIOD_MS); // Hold for 1/2 sec
  gpio_set_level(pn532_reset_pin_num, 1); // Set back to high

  vTaskDelay(1000 / portTICK_PERIOD_MS); // Give it a sec to reset

}


void _build_frame(uint8_t *pCmdBuf, size_t cmdLen, uint8_t *pFrameBuf, size_t *pFrameBufLen)
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

void send_pn532_ack() {
  pn532_bus_delay();
  uint8_t buf[] = {0x00, 0x00, 0xff, 0x00, 0xff, 0x00};
  ESP_ERROR_CHECK(i2c_write(&buf, sizeof(buf), 500));
  gettimeofday(&__transaction_stop, NULL);
}

void send_pn532_nack() {
  pn532_bus_delay();
  uint8_t buf[] = {0x00, 0x00, 0xff, 0xff, 0x00, 0x00};
  ESP_ERROR_CHECK(i2c_write(&buf, sizeof(buf), 500));
  gettimeofday(&__transaction_stop, NULL);
}


esp_err_t send_pn532_command(const uint8_t *pCmdBuf, size_t cmdBufLen) {
  // Create a frame to send

  static uint8_t frameBuf[256];
  size_t frameLen = 0;
  _build_frame(pCmdBuf, cmdBufLen, &frameBuf, &frameLen);

  pn532_bus_delay();
  esp_err_t res = i2c_write(&frameBuf, frameLen, 200);
  gettimeofday(&__transaction_stop, NULL);

  return res;

}

esp_err_t read_pn532_data(const uint8_t *pRxBuf, size_t rxBufLen, int timeout_ms)
{
  uint8_t tmpBuf[512];
  bool done = false;
  esp_err_t res = ESP_OK;

  struct timeval now;
  gettimeofday(&now, NULL);
  
  time_t maxtime_ms = now.tv_sec * 1000 + (now.tv_usec / 1000) + timeout_ms;

  do {
    pn532_bus_delay();
    res = i2c_read(tmpBuf, sizeof(tmpBuf), 500);
    ESP_ERROR_CHECK(res);
    gettimeofday(&__transaction_stop, NULL);

    // Check the ready byte
    if (pRxBuf[0] & 0x01) {
      // Got the ready bit!
      // Copy over the data - exclude the ready byte
      memcpy((void*)pRxBuf, &(tmpBuf[1]), MIN(sizeof(tmpBuf)-1, rxBufLen));
      done = true;

    } else {
      // Check for timeout - we can use the __transaction_stop time
      if (__transaction_stop.tv_sec * 1000 + (__transaction_stop.tv_usec / 1000) > maxtime_ms) {
        res = ESP_ERR_TIMEOUT;
        done = true;
      }
    }

  } while (!done);

  return res;
}


int check_ack(const uint8_t *buf, size_t buflen) {
  if (buflen < 6) return false;
  uint8_t ack[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
  return memcmp(buf, &ack, 6);
}

int check_nack(const uint8_t *buf, size_t buflen) {
  if (buflen < 6) return false;
  uint8_t nack[] = {0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00};
  return memcmp(buf, &nack, 6);
}

int check_error(const uint8_t *buf, size_t buflen) {
  if (buflen < 6) return false;
  uint8_t nack[] = {0x00, 0x00, 0xFF, 0x01, 0xff, 0x7f, 0x81, 0x00};
  return memcmp(buf, &nack, 8);
}

void pn532_extract_command_response(const uint8_t *pRxBuf, size_t rxBufLen, uint8_t expectedCommandCode,
   uint8_t *pRespBuf, size_t respBufLen) 
{
  // Check preamble & start of frame
  const uint8_t preamble_and_start[] = { 0x00, 0x00, 0xff };
  if (memcmp(pRxBuf, preamble_and_start, sizeof(preamble_and_start)) != 0) {
    ESP_LOGE(TAG, "Did not find preamble & start in pn532_extract_command_response");
    abort();
  }

  uint16_t datalen = pRxBuf[3];
  uint8_t *pLCS = &pRxBuf[4];
  // Check for extended length format
  if (datalen == 0xff && pRxBuf[4] == 0xff) {
    datalen = (pRxBuf[5] << 8) + pRxBuf[6];
    pLCS = &pRxBuf[7];
  }



}

 



