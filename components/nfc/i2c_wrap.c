#include "i2c_wrap.h"
#include "esp_log.h"
#include "driver/i2c.h"

static int i2c_portnum = 0;
static uint8_t i2c_dev_addr_w = 0x00;
static uint8_t i2c_dev_addr_r = 0x00;
static uint8_t i2c_dev_7bit_addr = 0x00;

static const char *TAG="i2c_wrap";

static int saved_i2c_timeout = 0;


esp_err_t i2c_init(int portnum, uint8_t sda_pin_num, uint8_t scl_pin_num, uint32_t spd_hz, uint8_t dev_addr_w) {
  i2c_config_t conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = sda_pin_num,
    .sda_pullup_en = GPIO_PULLUP_DISABLE,
    .scl_io_num = scl_pin_num,        
    .scl_pullup_en = GPIO_PULLUP_DISABLE,
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

esp_err_t i2c_read(uint8_t *pbtRx, const size_t szRx, TickType_t ticks_to_wait) {
  // Perform a read as master
  esp_err_t res = i2c_master_read_from_device(i2c_portnum, i2c_dev_7bit_addr, pbtRx, szRx, ticks_to_wait);
  return res;
}

esp_err_t i2c_write(const uint8_t *pbtTx, const size_t szTx, TickType_t ticks_to_wait) {
  return i2c_master_write_to_device(i2c_portnum, i2c_dev_7bit_addr, pbtTx, szTx, ticks_to_wait);
}

void save_i2c_timeout() {
  i2c_get_timeout(i2c_portnum, &saved_i2c_timeout);
}

void restore_i2c_timeout() {
  ESP_LOGI(TAG, "Restoring i2c timeout to: %d\n", saved_i2c_timeout);
  i2c_set_timeout(i2c_portnum, saved_i2c_timeout);
}

void set_i2c_timeout(int timeout_ms) {
  // Clock is based on the APB Frequency (80MHz)
  // double t = timeout_ms / 1000.0 * I2C_APB_CLK_FREQ;
  // int t2 = t;

  int t2 = I2C_APB_CLK_FREQ / 1000 * timeout_ms;

  ESP_LOGI(TAG, "Setting i2c timeout to: %d\n", t2);
  i2c_set_timeout(i2c_portnum, t2);
}

