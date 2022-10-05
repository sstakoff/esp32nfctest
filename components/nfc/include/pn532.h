#ifndef _PN532_H_
#define _PN532_H_

#include "stddef.h"
#include "stdio.h"
#include "esp_err.h"

typedef enum {
  ACK,
  NAK,
  ERR,
  DATA
} Pn532ResponseTypeEnum;

void pn532_wake();
void pn532_set_reset_pin(int reset_pin_num);
void pn532_reset();

void send_pn532_ack();
void send_pn532_nack();
esp_err_t send_pn532_command(const uint8_t *pCmdBuf, size_t cmdBufLen);
esp_err_t read_pn532_data(const uint8_t *pRxBuf, size_t rxBufLen, int timeout_ms);
void pn532_extract_command_response(const uint8_t *pRxBuf, size_t rxBufLen, uint8_t expectedCommandCode,
   uint8_t *pRespBuf, size_t respBufLen);
int check_ack(const uint8_t *buf, size_t buflen);
int check_nack(const uint8_t *buf, size_t buflen);
int check_error(const uint8_t *buf, size_t buflen);




#endif