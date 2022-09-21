#include "i2c_wrap.h"
#include "esp_log.h"
#include "driver/i2c.h"

static int i2c_portnum = 0;
static uint8_t i2c_dev_addr_w = 0x00;
static uint8_t i2c_dev_addr_r = 0x00;
static uint8_t i2c_dev_7bit_addr = 0x00;


esp_err_t i2c_init(int portnum, uint8_t sda_pin_num, uint8_t scl_pin_num, uint32_t spd_hz, uint8_t dev_addr_w) {
  i2c_config_t conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = sda_pin_num,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_io_num = scl_pin_num,        
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = spd_hz,  
    // .clk_flags = 0,   /*!< Optional, you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here. */
  };

  i2c_portnum = portnum;
  i2c_dev_addr_w = dev_addr_w;
  i2c_dev_addr_r = dev_addr_w | 0x01;
  i2c_dev_7bit_addr = dev_addr_w >> 1;

  ESP_ERROR_CHECK(i2c_param_config(i2c_portnum, &conf));
  ESP_ERROR_CHECK(i2c_driver_install(i2c_portnum, I2C_MODE_MASTER, 0, 0, 0));
  return ESP_OK;

}

int i2c_read(uint8_t *pbtRx, const size_t szRx, TickType_t ticks_to_wait) {
  // Perform a read as master
  esp_err_t res = i2c_master_read_from_device(i2c_portnum, i2c_dev_7bit_addr, pbtRx, szRx, ticks_to_wait);
  ESP_ERROR_CHECK(res);
  return ESP_OK;
}

int i2c_write(const uint8_t *pbtTx, const size_t szTx, TickType_t ticks_to_wait) {
  ESP_ERROR_CHECK(i2c_master_write_to_device(i2c_portnum, i2c_dev_7bit_addr, pbtTx, szTx, ticks_to_wait));
  return 0;
}


// /**
//  * @brief Write a frame to I2C device containing \a pbtTx content
//  *
//  * @param id I2C device.
//  * @param pbtTx pointer on buffer containing data
//  * @param szTx length of the buffer
//  * @return NFC_SUCCESS on success, otherwise driver error code
//  */
// int
// i2c_write(i2c_device id, const uint8_t *pbtTx, const size_t szTx)
// {
//   LOG_HEX(LOG_GROUP, "TX", pbtTx, szTx);

//   ssize_t writeCount;
//   writeCount = write(I2C_DATA(id) ->fd, pbtTx, szTx);

//   if ((const ssize_t) szTx == writeCount) {
//     log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG,
//             "wrote %d bytes successfully.", (int)szTx);
//     return NFC_SUCCESS;
//   } else {
//     log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR,
//             "Error: wrote only %d bytes (%d expected) (%s).", (int)writeCount, (int) szTx, strerror(errno));
//     return NFC_EIO;
//   }
// }