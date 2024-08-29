#pragma once

#include <unordered_map>
#include "iot_common.h"
#include "iot_mqtt_defs.h"
#include "mqtt_client.h"

/**
 * A class for handling mqtt related functionalities.
 */
class IotMqtt final {
public:
    esp_err_t start(std::string client_id);
    esp_err_t reconnect();
    esp_err_t subscribe(iot_mqtt_subscribe_t subscribe);
    esp_err_t publish(std::string topic, std::string data, size_t data_len, uint8_t qos, int *msg_id);
    bool connected() const;
    bool subscribed(std::string topic) const;

private:
    static constexpr const char *TAG = "IotMqtt";  /**< A constant used to identify the source of the log message of this class. */
    bool _connected = false;
    bool _initialized = false;

    esp_mqtt_client_config_t _mqtt_cfg{};                                       /**< The mqtt client config. */
    esp_mqtt_client_handle_t _client;                                           /**< The mqtt client handle. */
    std::mutex _subscribe_mutex;                                                /**< The mute for protecting access to the subscription map. */
    std::mutex _cb_mutex;                                                       /**< Mutex for protecting access to the event callbacks map. */
    std::unordered_map<std::string, iot_mqtt_subscribe_cb_t> _subscriptions{};  /**< The map of mqtt subscribers. */

    static std::function<void(iot_app_message_e)> _evt_cb;
    static void on_event(void *args, esp_event_base_t base, int32_t id, void *data);
    void on_data(esp_mqtt_event_handle_t evt);
};