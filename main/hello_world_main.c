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


void app_main(void)
{
    printf("Hello world!\n");

    vTaskDelay(2000/ portTICK_PERIOD_MS);


    // Reset PN532
    // gpio_config_t conf = {
    //   .pin_bit_mask = 1 << GPIO_NUM_0,
    //   .mode = GPIO_MODE_OUTPUT,
    //   .pull_up_en = GPIO_PULLDOWN_DISABLE,
    // };
    // ESP_ERROR_CHECK( gpio_config(&conf));
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_19, GPIO_MODE_OUTPUT));
    gpio_set_level(GPIO_NUM_19, 0);
    vTaskDelay(2000/ portTICK_PERIOD_MS);
    gpio_set_level(GPIO_NUM_19, 1);
    vTaskDelay(2000/ portTICK_PERIOD_MS);


    i2c_init(0, 21, 22, 400000, 0x48);
    printf("I2C initialized\n");
    int t=0;
    i2c_set_timeout(0, 132000);
    i2c_get_timeout(0,&t);
    printf("I2C Timeout: %d\n", t);
    vTaskDelay(2000/ portTICK_PERIOD_MS);


  // const uint8_t wakeupCommand[] = { 0x00, 0x00, 's', 't', 'o', 'o'}; // Comms test

  uint8_t wakeupCommand[] = {0x14, 0x01};
  uint8_t frame[256];
  size_t frameLen = sizeof(frame);

  build_frame(wakeupCommand, sizeof(wakeupCommand), frame, &frameLen);
  printf("Frame: ");
  for (int i=0; i < frameLen; ++i) {
    printf("%x ", frame[i]);
  }
  printf("\n");

  i2c_write(frame, frameLen, 500);
  printf("Wrote command\n");

  vTaskDelay(5000 / portTICK_PERIOD_MS);

  uint8_t buf[256];
  i2c_read(&buf, sizeof(buf), 500);
  printf("RX: ");
  for (int i=0; i < 7; ++i) {
    printf("%x ", buf[i]);
  }
  printf("\n");

  vTaskDelay(1000 / portTICK_PERIOD_MS);

  i2c_read(&buf, sizeof(buf), 500);
  printf("RX: ");
  for (int i=0; i < 17; ++i) {
    printf("%x ", buf[i]);
  }
  printf("\n");








    // uint8_t buf[8];
    // i2c_read(buf, 8, 500);
    // for (int i=0; i < 8; ++i) {
    //   printf("%d ", buf[i]);
    // }
    // printf("\n");
  


    // printf("%d tick rate MS\n", portTICK_RATE_MS);
    // printf("%d 1 msec to ticks\n", pdMS_TO_TICKS(1000));
    // fflush(stdout);


    // /* Print chip information */
    // esp_chip_info_t chip_info;
    // esp_chip_info(&chip_info);
    // printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
    //         CONFIG_IDF_TARGET,
    //         chip_info.cores,
    //         (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
    //         (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    // printf("silicon revision %d, ", chip_info.revision);

    // printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
    //         (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    // printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

    // for (int i = 10; i >= 0; i--) {
    //     printf("Restarting in %d seconds...\n", i);
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
    // printf("Restarting now.\n");
    // fflush(stdout);
    // esp_restart();

        fflush(stdout);

}
