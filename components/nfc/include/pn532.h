#ifndef _PN532_H_
#define _PN532_H_

#include "stddef.h"
#include "stdio.h"
#include "esp_err.h"

void pn532_wake();
void pn532_initialize();
void pn532_comms_test();
void pn532_get_firmware_version(uint8_t *IC, uint8_t *Ver, uint8_t *Rev, uint8_t *Support);
void pn532_set_parameters(uint8_t flags);

void pn532_set_reset_pin(int reset_pin_num);
void pn532_reset();

void send_pn532_ack();
void send_pn532_nack();
void dumphex(const char *msg, const uint8_t *buf, size_t cnt);
esp_err_t send_pn532_command(uint8_t commandCode, const uint8_t *pCmdDataBuf, size_t cmdDataBufLen);
esp_err_t read_pn532_data(const uint8_t *pRxBuf, size_t rxBufLen, int timeout_ms);
size_t pn532_tranceive(uint8_t commandCode, const uint8_t *pCmdDataBuf, size_t cmdDataBufLen, 
                    uint8_t *pRespBuf, size_t respBufLen, int retries, int timeout_ms);
int pn532_extract_command_response(const uint8_t *pRxBuf, size_t rxBufLen, uint8_t expectedCommandCode,
   uint8_t *pRespBuf, size_t respBufLen);
int check_ack(const uint8_t *buf, size_t buflen);
int check_nack(const uint8_t *buf, size_t buflen);
int check_error(const uint8_t *buf, size_t buflen, uint8_t *errorCode);


#define CMD_SamConfiguration 0x14
#define CMD_Diagnose 0x00
#define CMD_GetFirmwareVersion 0x02
#define CMD_SetParameters 0x12
#define SetParameters_AutomaticRATS_bit 0x10
#define SetParameters_AutomaticATR_RES_bit 0x04


#endif