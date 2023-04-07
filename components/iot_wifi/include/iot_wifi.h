#pragma once

#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "lwip/err.h"
#include "lwip/sys.h"

/**
 * A class for handling wifi-related tasks.
 */
class iot_wifi
{
public:
    void configure();
    bool is_connected() const;

private:
    static bool _reconnect;                        /* Indicates whether the device should attempt to reconnect to the Wi-Fi network. */
    static bool _connected;                        /* Indicates whether the device is connected to wifi. */
    static uint8_t _retries;                       /* The number of Wi-Fi reconnect attempts. */
    const uint8_t _max_retries = 10;               /* The maximum number of times the device will attempt to connect to Wi-Fi before giving up. */
    uint16_t connect_interval = 1000;              /* The amount of time the device will wait before attempting to reconnect to Wi-Fi. */
    static constexpr const char *tag = "iot_wifi"; /* A constant used to identify the source of the log message of this class. */
    static void wifi_evt_handler(void *arg, esp_event_base_t event_base, const int32_t event_id, void *event_data);
    void reconnect_wifi(void);
};
