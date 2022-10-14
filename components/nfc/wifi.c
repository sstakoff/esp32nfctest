
#include "wifi.h"
#include <protocomm.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>
#include <wifi_provisioning/scheme_softap.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <mdns.h>

#include "esp_log.h"

void configure_wifi()
{
  esp_event_loop_create_default();
  wifi_init_config_t wconfig = WIFI_INIT_CONFIG_DEFAULT();
  nvs_flash_init();

  uint8_t mac[8];

  ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac));
  ESP_ERROR_CHECK(esp_base_mac_addr_set(mac));
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(mdns_init());

    ESP_ERROR_CHECK(esp_wifi_init(&wconfig));


  wifi_prov_mgr_config_t config = {
      .scheme = wifi_prov_scheme_ble,
      .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM};
  ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

  const char *service_name = "my_device";
  const char *service_key = "password";
  wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
  const char *pop = "abcd1234";
    ESP_LOGI("XXXXXX", "WIFI PROV STARTING");

  ESP_ERROR_CHECK( wifi_prov_mgr_start_provisioning(security, pop, service_name, service_key) );
    ESP_LOGI("XXXXXX", "About to call wifi_prov_mgr_wait");

  wifi_prov_mgr_wait();
      ESP_LOGI("XXXXXX", "About to call wifi_prov_mgr_deinit");

  wifi_prov_mgr_deinit();

  ESP_LOGI("XXXXXX", "WIFI PROV RETURNED");
}