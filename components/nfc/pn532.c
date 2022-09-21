#include "pn532.h"
#include <string.h>

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