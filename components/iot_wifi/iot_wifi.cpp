#include <stdio.h>
#include "iot_wifi.h"

/* Determines whether the device should attempt to reconnect to the Wi-Fi network. */
bool iot_wifi::_reconnect{false};

/* The number of Wi-Fi reconnect attempts. */
uint8_t iot_wifi::_retries{0};

/* Indicates whether the device is connected to wifi. */
bool iot_wifi::_connected{false};

/**
 * Wi-Fi events callback handler that is called when a Wi-Fi event occurs.
 * @param arg Pointer to the user passed to esp_event_loop_create when the event loop was created.
 * @param event_base The event base that the event is associated with.
 * @param event_id The event ID.
 * @param event_data The data associated with the event.
 */
void iot_wifi::wifi_evt_handler(void *arg, esp_event_base_t event_base, const int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(tag, "%s -> Connecting to the wifi", __func__);
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        _connected = false;
        _reconnect = true;
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        _connected = true;
        const auto event = static_cast<ip_event_got_ip_t *>(event_data);
        ESP_LOGI(tag, "Wifi connected, device assigned IP Address is:" IPSTR, IP2STR(&event->ip_info.ip));

        if (_reconnect)
        {
            _retries = 0;
            _reconnect = false;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP)
    {
        ESP_LOGI(tag, "%s -> Lost IP address", __func__);
    }
}

/**
 * Configures the device's Wi-Fi connection in station mode, and registers the Wi-Fi event handlers.
 */
void iot_wifi::configure()
{
    ESP_ERROR_CHECK(esp_netif_init());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_evt_handler, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_evt_handler, nullptr));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    while (!_connected)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @return A boolean indicating whether the device is connected to wifi or not.
 */
bool iot_wifi::is_connected() const
{
    return _connected;
}

/**
 * Attempts to reconnect to the Wi-Fi.
 */
void iot_wifi::reconnect_wifi(void)
{
    ESP_LOGI(tag, "%s -> Attempting to reconnect to the wifi", __func__);

    if (_retries < _max_retries)
    {
        esp_wifi_connect();
        _retries++;
    }
    else
    {
        ESP_LOGI(tag, "%s -> Failed to reconnect to the wifi", __func__);

        if (connect_interval == 50000)
        {
            _reconnect = false;
            return;
        }

        connect_interval += 1000;
        _retries = 0;
    }

    vTaskDelay(connect_interval / portTICK_PERIOD_MS);
}
