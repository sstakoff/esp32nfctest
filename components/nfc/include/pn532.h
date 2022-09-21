#ifndef _PN532_H_
#define _PN532_H_

#include "stddef.h"
#include "stdio.h"

void build_frame(uint8_t *pCmdBuf, size_t cmdLen, uint8_t *pFrameBuf, size_t *pFrameBufLen);

#endif