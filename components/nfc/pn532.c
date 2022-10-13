#include "pn532.h"
#include "i2c_wrap.h"
#include <string.h>
#include <sys/time.h>
#include <sys/param.h>
#include "esp_log.h"

static char *TAG = "pn532.c";

static int pn532_reset_pin_num = 0;
static struct timeval __transaction_stop;

static uint8_t *tag_data = NULL;
static size_t tag_data_len = 0;

void set_tag_data(uint8_t *data, size_t datalen) {
  tag_data = data;
  tag_data_len = datalen;
}

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
    // printf("Delaying i2c bus for %ld ms\n", delay_needed_ms);
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


void _build_frame(uint8_t commandCode, const uint8_t *pCmdDataBuf, size_t cmdDataLen, uint8_t *pFrameBuf, size_t *pFrameBufLen)
{
    const uint8_t preamble_and_start[] = { 0x00, 0x00, 0xff };
    memcpy(pFrameBuf, preamble_and_start, sizeof(preamble_and_start));

    pFrameBuf[3] = cmdDataLen + 2;  //LEN - includes the TFI and the command code
    pFrameBuf[4] = 256 - (cmdDataLen + 2);  // LCS
    pFrameBuf[5] = 0xd4; // TFI
    pFrameBuf[6] = commandCode;

    memcpy(pFrameBuf + 7, pCmdDataBuf, cmdDataLen);

    uint8_t dcs = 256 - 0xd4; // DCS
    dcs -= commandCode;
    for (size_t i = 0; i < cmdDataLen; ++i) {
      dcs -= pCmdDataBuf[i];
    }
    pFrameBuf[7 + cmdDataLen] = dcs;
    pFrameBuf[8 + cmdDataLen] = 0x00;

    *pFrameBufLen = 9 + cmdDataLen;
}

void pn532_wake() {

  // Send a SAMConfiguration command - this is how you wake the 532
  // The second param 0x01 indicated Normal mode where no SAM is used
  uint8_t wakeupCommand[] = {0x01};

  pn532_tranceive(CMD_SamConfiguration, (const uint8_t*)&wakeupCommand, sizeof(wakeupCommand), NULL, 0, 5, 2000);

  ESP_LOGI(TAG, "Woke up pn532\n");
  
}

void pn532_comms_test() {

  uint8_t testData[] = {0x00, 's', 't', 'o', 'o'};

  uint8_t resp[16];
  size_t respLen = pn532_tranceive(CMD_Diagnose, (const uint8_t *)&testData, sizeof(testData), (uint8_t*)&resp, sizeof(resp), 5, 2000);

  if ((respLen == sizeof(testData)) && (0 == memcmp(testData, resp, respLen))) {
    ESP_LOGI(TAG, "Passed comms test\n");
  } else {
    ESP_LOGE(TAG, "Comms test failed\n");
    abort();
  }
}

void pn532_set_parameters(uint8_t flags) {
  pn532_tranceive(CMD_SetParameters, &flags, 1, NULL, 0, 5, 2000);
  ESP_LOGI(TAG, "Set Parameters to: 0x%x", flags);
}

void pn532_get_firmware_version(uint8_t *IC, uint8_t *Ver, uint8_t *Rev, uint8_t *Support) 
{

  uint8_t resp[4];
  size_t respLen = pn532_tranceive(CMD_GetFirmwareVersion, NULL, 0, (uint8_t *)&resp, sizeof(resp), 5, 2000);

  if (respLen != 4) {
    ESP_LOGE(TAG, "Bad response for GetFirmwareVersion");
    abort();
  }

  *IC = resp[0];
  *Ver = resp[1];
  *Rev = resp[2];
  *Support = resp[3];

  ESP_LOGI(TAG, "Firmware:  IC: %x, Ver: %x, Rev: %x, Support: %x", *IC, *Ver, *Rev, *Support);
}


void send_pn532_ack() {
  pn532_bus_delay();
  uint8_t buf[] = {0x00, 0x00, 0xff, 0x00, 0xff, 0x00};
  ESP_ERROR_CHECK(i2c_write((const uint8_t *)&buf, sizeof(buf), 500));
  gettimeofday(&__transaction_stop, NULL);
}

void send_pn532_nack() {
  pn532_bus_delay();
  uint8_t buf[] = {0x00, 0x00, 0xff, 0xff, 0x00, 0x00};
  ESP_ERROR_CHECK(i2c_write((const uint8_t*)&buf, sizeof(buf), 500));
  gettimeofday(&__transaction_stop, NULL);
}


esp_err_t send_pn532_command(uint8_t commandCode, const uint8_t *pCmdDataBuf, size_t cmdDataBufLen) 
{
  // Create a frame to send

  static uint8_t frameBuf[256];
  size_t frameLen = 0;
  _build_frame(commandCode, pCmdDataBuf, cmdDataBufLen, (uint8_t *)&frameBuf, &frameLen);

  pn532_bus_delay();
  esp_err_t res = i2c_write((const uint8_t*)&frameBuf, frameLen, 200);
  dumphex("Sent cmd", frameBuf, frameLen);
  gettimeofday(&__transaction_stop, NULL);

  return res;

}

void dumphex(const char *msg, const uint8_t *buf, size_t cnt) {
  printf("                                  %s: Dump %d bytes: ", msg, cnt);
  for (size_t i=0; i < cnt; ++i) {
    printf("%x ", buf[i]);
  }
  printf("\n"); fflush(stdout);
}

esp_err_t read_pn532_data(const uint8_t *pRxBuf, size_t rxBufLen, int timeout_ms)
{
  uint8_t tmpBuf[384];
  int done = 0;
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
    if (tmpBuf[0] & 0x01) {
      // Got the ready bit!
      // Copy over the data - exclude the ready byte
      memcpy((void*)pRxBuf, &(tmpBuf[1]), MIN(sizeof(tmpBuf)-1, rxBufLen));
      done = 1;

    } else {
      if (timeout_ms > 0) {
        // Check for timeout - we can use the __transaction_stop time
        if (__transaction_stop.tv_sec * 1000 + (__transaction_stop.tv_usec / 1000) > maxtime_ms) {
          res = ESP_ERR_TIMEOUT;
          done = 1;
        }
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

int check_error(const uint8_t *buf, size_t buflen, uint8_t *errorCode) {
  if (buflen < 6) return false;
  uint8_t nack[] = {0x00, 0x00, 0xFF, 0x01, 0xff };
  *errorCode = buf[5];
  return memcmp(buf, &nack, 5);
}

int pn532_extract_command_response(const uint8_t *pRxBuf, size_t rxBufLen, uint8_t expectedCommandCode,
   uint8_t *pRespBuf, size_t respBufLen) 
{
  // Check preamble & start of frame
  const uint8_t preamble_and_start[] = { 0x00, 0x00, 0xff };
  if (memcmp(pRxBuf, preamble_and_start, sizeof(preamble_and_start)) != 0) {
    ESP_LOGE(TAG, "Did not find preamble & start in pn532_extract_command_response\n");
    abort();
  }

  uint16_t datalen = pRxBuf[3];
  int TFI_idx = 5;
  // Check for extended length format
  if (datalen == 0xff && pRxBuf[4] == 0xff) {
    // extended frame
    datalen = (pRxBuf[5] << 8) + pRxBuf[6];
    // Check LCS
    if (((pRxBuf[5] + pRxBuf[6] + pRxBuf[7]) % 256) != 0) {
      ESP_LOGE(TAG, "Bad extended LCS\n");
      abort();
    }
    TFI_idx = 8;
  } else {
    // normal frame - check LCS
    if ((uint8_t)(pRxBuf[3] + pRxBuf[4])) {
      ESP_LOGE(TAG, "Bad LCS");
      abort();
    }
  }

  // Check TFI
  if (pRxBuf[TFI_idx] != 0xd5) {
    ESP_LOGE(TAG, "Bad TFI: %x", pRxBuf[TFI_idx]);
    abort();
  }

  // Check the command code
  if (pRxBuf[TFI_idx + 1] != expectedCommandCode) {
    ESP_LOGE(TAG, "Bad Command Code on Response: %x\n", pRxBuf[TFI_idx + 1]);
    abort();
  }

  //  Check DCS
  uint8_t DCS = pRxBuf[TFI_idx + datalen];
  uint8_t btDCS = DCS;

  // Compute data checksum
  for (size_t i = 0; i < datalen; i++) {
    btDCS += pRxBuf[TFI_idx + i];
  }

  if (btDCS != 0) {
    ESP_LOGE(TAG, "Data checksum mismatch\n");
    abort();
  }

  if (0x00 != pRxBuf[TFI_idx + datalen + 1]) {
    ESP_LOGE(TAG, "Postamble mismatch\n");
    abort();
  }

  if (rxBufLen < datalen-2) {
    ESP_LOGE(TAG, "Receive buffer too small for returned data: was %d but need %d\n", rxBufLen, datalen-2);
    abort();
  }

  memcpy(pRespBuf, &pRxBuf[TFI_idx + 2], datalen - 2);
  return datalen - 2;
}

// Return # bytes in the response
size_t pn532_tranceive(uint8_t commandCode, const uint8_t *pCmdDataBuf, size_t cmdDataBufLen, 
                    uint8_t *pRespBuf, size_t respBufLen, int retries, int timeout_ms)
{

  int retriesLeft = retries;
  int done = 0;
  int respLen = 0;
  do {

    if (retriesLeft != retries) {
      ESP_LOGI(TAG, "Retrying. Attempts left: %d\n", retriesLeft);
      send_pn532_ack(); // Reset the command
    }  

    retriesLeft--;

    ESP_ERROR_CHECK(send_pn532_command(commandCode, pCmdDataBuf, cmdDataBufLen));

    // Receive the ack
    static uint8_t buf[384];
    if (ESP_ERR_TIMEOUT == read_pn532_data(buf, sizeof(buf), timeout_ms)) {
      // Send an ack (resets the dialogue) and try again
      ESP_LOGW(TAG, "Timed out waiting for ACK.\n");
      continue;
    }

    // Make sure we recieved an ACK - not NACK or ERROR
    if (0 == check_nack((const uint8_t*)&buf, sizeof(buf))) {
      ESP_LOGE(TAG, "Received a NACK!!!\n");
      continue;
    }

    uint8_t ecode = 0;
    if (0 == check_error((const uint8_t*)&buf, sizeof(buf), &ecode)) {
      ESP_LOGE(TAG, "Received application level error code: %x\n", ecode);
      continue;
    }

    // Make sure we received an ack
    if (0 != check_ack(pRespBuf, respLen)) {
      ESP_LOGE(TAG, "Did not receive expected ACK. Not sure what happened here\n");
      continue;
    }

    ESP_LOGI(TAG, "*** Received ACK from pn532\n");

    // We got the ACK, so now retrieve the command response
    if (ESP_ERR_TIMEOUT == read_pn532_data((const uint8_t*)&buf, sizeof(buf), timeout_ms)) {
      ESP_LOGW(TAG, "Timed out waiting for command response.\n");
      continue;
    }

    respLen = pn532_extract_command_response((const uint8_t*)&buf, sizeof(buf), commandCode+1, pRespBuf, respBufLen);
    done = 1;
    
  } while (!done && retriesLeft > 0);

  return respLen;

}

void pn532_initialize() {
    
    ESP_LOGI(TAG, "Initializing the pn532");
    pn532_reset();

    // Wakeup the PN532
    pn532_wake();
  
    // Perform basic comms test
    pn532_comms_test();

    uint8_t IC, Ver, Rev, Support;
    pn532_get_firmware_version(&IC, &Ver, &Rev, &Support);

    pn532_set_parameters(SetParameters_AutomaticRATS_bit | SetParameters_AutomaticATR_RES_bit);

    // Config registers
    // 0x6302 = CIU_TxMode regsiter. 0x80 = enables the CRC generation during data transmission
    // 0x6303 = CIU_RxMode register. 0x80 = enables the CRC calculation during reception
    uint8_t regdata[] = {0x63, 0x02, 0x80, 0x63, 0x03, 0x80};
    pn532_tranceive(CMD_WriteRegister, regdata, sizeof(regdata), NULL, 0, 5, 2000);
    ESP_LOGI(TAG, "CRC generation configured");

    // RF Config
    // 0x01 = RF Field; 
    // 0x00. Bit 1 = 0 Auto RFCA off - do not take care of external field before switching on own field
    //       Bit 0 = 0 RF off - Do not generate RF field
    uint8_t rfdata[] = {0x01, 0x00};
    pn532_tranceive(CMD_RfConfiguration, rfdata, sizeof(rfdata), NULL, 0, 5, 2000);
    ESP_LOGI(TAG, "RF Configuration complete");

    // Collision avoidance
    // 0x6305 = CIU_TxAuto register. 0x04 = InitialRFOn - perform initial RF collision avoidance
    uint8_t regdata2[] = {0x63, 0x05, 0x04};
    pn532_tranceive(CMD_WriteRegister, regdata2, sizeof(regdata2), NULL, 0, 5, 2000);
    ESP_LOGI(TAG, "Collision avoidance configured");

    uint8_t tgtdata[] = {
      0x01,  // Mode:  PassiveOnly=1 (refuse active comm); DepOnly=0; PiccOnly=0
      // Mifare params. 6 bytes
      0x04, 0x00, // SENS_RES. 2 Bytes LSB first. per iso14443-3
      0x00, 0xb0, 0x0b, // NFCID1t 
      0x00, // SEL_RES - PICC not compliant with 14443-4
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Felica
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // NFCID3t
      0x00, // LenGt
      0x00 // LenTk
      };

    uint8_t commandbuf[256];
    size_t cmdlen = pn532_tranceive(CMD_TgInitAsTarget, tgtdata, sizeof(tgtdata), commandbuf, sizeof(commandbuf), 5, 0);
    ESP_LOGI(TAG, "GOT COMMAND");
    dumphex("Command from initiator", commandbuf, cmdlen);

    while (1) {
      processInitiatorCommand((const uint8_t*)commandbuf, cmdlen);

      cmdlen = pn532_tranceive(CMD_TgGetInitiatorCommand, NULL, 0, commandbuf, sizeof(commandbuf), 5, 0);

    }




}

void processInitiatorCommand(const uint8_t *pCmdBuf, size_t cmdLen) {
  if (cmdLen < 2) return;

  uint8_t mode = pCmdBuf[0];
  uint8_t baudbits = mode & 0x70;
  int baud = 0;
  if (baudbits == 0) baud = 106;
  if (baudbits == 1) baud = 212;
  if (baudbits == 2) baud = 424;
  ESP_LOGI(TAG, "Tag Baud: %d", baud);

  ESP_LOGI(TAG, "Supports 14443-4: %d", mode & 0x08);
  ESP_LOGI(TAG, "Supports DEP: %d", mode & 0x04);

  uint8_t framing = mode & 0x03;
  switch (framing) {
    case 0:
        ESP_LOGI(TAG, "Framing: Mifare");
        break;
      case 1:
        ESP_LOGI(TAG, "Framing: Active");
        break;
      case 2:
        ESP_LOGI(TAG, "Framing: Felica");
        break;
      default:
        ESP_LOGI(TAG, "Framing: **UNKNOWN**");
        break;
  }

  // Only support Mifare
  if (framing != 0) {
    ESP_LOGE(TAG, "Framing not supported");
    abort();
  }

  // Process the command
  uint8_t startAddr;
  uint8_t buf[16];

  switch (pCmdBuf[1]) {
    // READ
    case 0x30:
      // Read 16 bytes starting at the address requested
      startAddr = pCmdBuf[2] * 4; // Each address points to page of 4 bytes
      // if (startAddr + 16 > tag_data_len) {
      //   ESP_LOGE(TAG, "Tried to read beyond tag end");
      //   abort();
      // }

      ESP_LOGI(TAG, "Start addr: 0x%x", startAddr);

      memcpy(buf, tag_data + startAddr, 16);
      uint8_t stat = 0;
      pn532_tranceive(CMD_TgResponseToInitiator, buf, 16, &stat, 1, 5, 2000);
      uint8_t errcode = stat & 0x3F;
      if (errcode == 0) {
        ESP_LOGI(TAG, "Initiator response received successfully");
      } else {
        ESP_LOGE(TAG, "Initiator responded with error: %x", errcode);
      }
      break;
  // HALT
  case 0x50:
    ESP_LOGI(TAG, "Received HALT command");
    abort();

  default:
    ESP_LOGE(TAG, "Received unknown command: 0x%x", pCmdBuf[1]);
    abort();
    break;
  }
}