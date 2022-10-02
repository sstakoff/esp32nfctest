#ifndef _PN532_H_
#define _PN532_H_

#include "stddef.h"
#include "stdio.h"
#include "esp_err.h"


void pn532_wake();
void pn532_set_reset_pin(int reset_pin_num);
void pn532_reset();

void build_frame(uint8_t *pCmdBuf, size_t cmdLen, uint8_t *pFrameBuf, size_t *pFrameBufLen);
esp_err_t send_pn532_command(const uint8_t *pCmdBuf, size_t cmdBufLen);
esp_err_t read_pn532_data(const uint8_t *pRxBuf, size_t rxBufLen, int timeout_ms);
int check_ack(const uint8_t *buf, size_t buflen);
int check_nack(const uint8_t *buf, size_t buflen);


#endif