#include <mutex>
#include <cJSON.h>
#include "iot_mqtt.h"

/* A pointer to the start of the mqtt config. */
extern const uint8_t config_key_start[] asm("_binary_iot_mqtt_start");

/* A pointer to the end of the mqtt config. */
extern const uint8_t config_key_end[] asm("_binary_iot_mqtt_end");

/**
 * Starts the component
 *
 * @param[in] client_id The client id to use.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotMqtt::start(std::string client_id)
{
    const uint8_t *buf = const_cast<uint8_t *>(config_key_start);

    ESP_LOGI(TAG, "Loaded config [json: %s]", buf);

    cJSON *root = cJSON_Parse(reinterpret_cast<const char *>(buf));

    if (root == nullptr) {
        ESP_LOGI(TAG, "%s: Failed to start the component, [reason: %s]", __func__, cJSON_GetErrorPtr());
    }

    if (cJSON_GetObjectItem(root, "mqtt_url")) {
        char *url = cJSON_GetObjectItem(root,"mqtt_url")->valuestring;
        _mqtt_cfg.broker.address.uri = url;
        ESP_LOGI(TAG, "%s: Configuring Mqtt [url: %s]", __func__, _mqtt_cfg.broker.address.uri);
    }

    if (cJSON_GetObjectItem(root, "username")) {
        char *username = cJSON_GetObjectItem(root,"username")->valuestring;
        _mqtt_cfg.credentials.username = username;
        ESP_LOGI(TAG, "%s: Configuring Mqtt [url: %s]", __func__, username);
    }

    if (cJSON_GetObjectItem(root, "password")) {
        char *password = cJSON_GetObjectItem(root,"password")->valuestring;
        _mqtt_cfg.credentials.authentication.password = password;
        ESP_LOGI(TAG, "%s: Configuring Mqtt [password: %s]", __func__, iot_mask_str(password));
    }

    _mqtt_cfg.network.disable_auto_reconnect = true;
    _client = esp_mqtt_client_init(&_mqtt_cfg);

    esp_mqtt_client_register_event(_client, MQTT_EVENT_ANY, on_event, this);
    esp_err_t ret = esp_mqtt_client_start(_client);

    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "%s: Failed to start the component, [reason: %s]", __func__, esp_err_to_name(ret));
        return ret;
    }

    _initialized = true;

    ESP_LOGI(TAG, "%s: Component started successfully", __func__);

    return ESP_OK;
}

/**
 * Reconnects to the mqtt client,
 *
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotMqtt::reconnect()
{
    esp_err_t ret = esp_mqtt_client_reconnect(_client);

    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "%s: Failed to reconnect the mqtt client, [reason: %s]", __func__, esp_err_to_name(ret));
    }
    return ret;
}

/**
 * Subscribes to a mqtt topic.
 *
 * @param[in] subscribe The the subscription data.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotMqtt::subscribe(iot_mqtt_subscribe_t subscribe)
{
    std::lock_guard<std::mutex> lock(_subscribe_mutex);

    std::string topic = subscribe.topic;

    if (!iot_valid_str(topic.c_str()))
        return ESP_FAIL;

    int ret = esp_mqtt_client_subscribe(_client, topic.c_str(), subscribe.qos);

    if (ret < 0) {
        ESP_LOGI(TAG, "%s: Failed to subscribe to [topic: %s]", __func__, topic.c_str());
        return ESP_FAIL;
    }

    _subscriptions[topic] = subscribe.cb;

    ESP_LOGI(TAG, "%s: Successfully subscribed successful [msg_id: %d]", __func__, ret);

    return ESP_OK;
}

/**
 * Publishes data to a mqtt topic.
 *
 * @param[in] topic The topic to publish to.
 * @param[in] data The data to publish.
 * @param[in] data_len The length of the data.
 * @param[in] qos The quality of service for the message.
 * @param[out] msg_id A pointer to the message id to fill.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotMqtt::publish(std::string topic, std::string data, size_t data_len, uint8_t qos, int *msg_id)
{
    *msg_id = esp_mqtt_client_publish(_client, topic.data(), data.data(), data_len, qos, 0);

    if (*msg_id < 0) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * Handles MQTT related events
 *
 * @param[in] args A pointer to the user data.
 * @param[in] base The event base for the handler.
 * @param[in] id The id of the received event.
 * @param[in] data A pointer to the event data.
 */
void IotMqtt::on_event(void *args, esp_event_base_t base, int32_t id, void *data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, id);

    auto *self =  static_cast<IotMqtt *>(args);

    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(data);

    switch ((esp_mqtt_event_id_t)id) {
        case MQTT_EVENT_CONNECTED:
            self->_connected = true;
            ESP_LOGI(TAG, "%s: Received event [id: MQTT_EVENT_CONNECTED].", __func__);
            break;
        case MQTT_EVENT_DISCONNECTED:
            self->_connected = false;
            ESP_LOGI(TAG, "%s: Received event [id: MQTT_EVENT_DISCONNECTED].", __func__);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "%s: Received event [id: MQTT_EVENT_SUBSCRIBED, msg_id: %d].", __func__, event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "%s: Received event [id: MQTT_EVENT_UNSUBSCRIBED, msg_id: %d].", __func__, event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "%s: Received event [id: MQTT_EVENT_PUBLISHED, msg_id: %d].", __func__, event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "%s: Received event [id: MQTT_EVENT_DATA, topic: %s].", __func__, event->topic);
            self->on_data(event);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "%s: Received event [id: MQTT_EVENT_ERROR].", __func__);
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "%s: MQTT_EVENT_ERROR - Last reported errno from esp-tls [errno: : 0x%x]", __func__, event->error_handle->esp_tls_last_esp_err);
                ESP_LOGE(TAG, "%s: MQTT_EVENT_ERROR - Last reported error from tls stack [error: : 0x%x]", __func__, event->error_handle->esp_tls_stack_err);
                ESP_LOGE(TAG, "%s: MQTT_EVENT_ERROR - Last captured transport socket [errno: %s]", __func__,  strerror(event->error_handle->esp_transport_sock_errno));
            }
            else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED)
                ESP_LOGE(TAG, "%s: MQTT_EVENT_ERROR - Connection refused [error: 0x%x]", __func__, event->error_handle->connect_return_code);
            else
                ESP_LOGE(TAG, "%s: MQTT_EVENT_ERROR - Unknown error [type: 0x%x]", __func__, event->error_handle->error_type);
            break;
        default:
            ESP_LOGI(TAG, "%s: Received other event [id: %d]", __func__, event->event_id);
            break;
    }
}

/**
 * Handles incoming message data and invoking the subscribers.
 *
 * @param[in] evt The event data.
 */
void IotMqtt::on_data(esp_mqtt_event_handle_t evt)
{
    ESP_LOGI(TAG, "%s: Received [data: %s, size: %d].", __func__, evt->data, evt->data_len);

    std::string topic = evt->topic;

    if (topic.empty())
        return;

    auto it = _subscriptions.find(evt->topic);

    if (it != _subscriptions.end()) {
        it->second(evt->topic, evt->data, evt->data_len, nullptr);
    }
}

/**
 * Check if the mqtt client is currently connected.
 *
 * @return true if connected, false otherwise.
 */
bool IotMqtt::is_connected() const
{
    return _connected;
}
