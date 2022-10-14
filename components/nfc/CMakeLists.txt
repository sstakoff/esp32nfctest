idf_component_register(SRCS "pn532.c" "i2c_wrap.c" "wifi.c"
                    INCLUDE_DIRS "include"
                    REQUIRES protocomm wifi_provisioning mdns)
