/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "i2c_wrap.h"
#include <string.h>
#include "pn532.h"
#include "esp_log.h"

#define I2C_PORT 0

static const char *TAG = "main";


void app_main()
{
    ESP_LOGI(TAG, "Starting up!!!\n");


    // Initialize I2C @ 400 KHz
    i2c_init(I2C_PORT, 21, 22, 400000, 0x48);
    printf("I2C initialized\n");

    save_i2c_timeout();
    set_i2c_timeout(2); // 2ms


    // Configure PN532 reset pin and reset the card
    pn532_set_reset_pin(GPIO_NUM_19);

    pn532_initialize();



    fflush(stdout);

}


// static uint8_t __nfcforum_tag2_memory_area[] = {
//   0x00, 0x00, 0x00, 0x00,  // Block 0
//   0x00, 0x00, 0x00, 0x00,
//   0x00, 0x00, 0xFF, 0xFF,  // Block 2 (Static lock bytes: CC area and data area are read-only locked)
//   0xE1, 0x10, 0x06, 0x0F,  // Block 3 (CC - NFC-Forum Tag Type 2 version 1.0, Data area (from block 4 to the end) is 48 bytes, Read-only mode)

//   0x03, 33,   0xd1, 0x02,  // Block 4 (NDEF)
//   0x1c, 0x53, 0x70, 0x91,
//   0x01, 0x09, 0x54, 0x02,
//   0x65, 0x6e, 0x4c, 0x69,

//   0x62, 0x6e, 0x66, 0x63,
//   0x51, 0x01, 0x0b, 0x55,
//   0x03, 0x6c, 0x69, 0x62,
//   0x6e, 0x66, 0x63, 0x2e,

//   0x6f, 0x72, 0x67, 0x00,
//   0x00, 0x00, 0x00, 0x00,
//   0x00, 0x00, 0x00, 0x00,
//   0x00, 0x00, 0x00, 0x00,
// };





