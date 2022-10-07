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

    // Configure PN532 reset pin and reset the card
    pn532_set_reset_pin(GPIO_NUM_19);
    pn532_reset();

    // Wakeup the PN532
    pn532_wake();
  
    // Perform basic comms test
    pn532_comms_test();


    fflush(stdout);

}



