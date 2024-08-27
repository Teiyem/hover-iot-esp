#pragma once

#define IOT_MQTT_MAX_BUFFER 4096

/**
 * A struct for the mqtt configuration.
 */
typedef struct iot_mqtt_cfg {
    std::string mqtt_url;         /** The mqtt broker url. */
    std::string client_id;        /** The mqtt client id. */
    std::string username;         /** The mqtt username. */
    std::string password;         /** The mqtt password. */
}iot_mqtt_cfg_t;

typedef struct iot_mqtt_message {

} iot_mqtt_message_t;


typedef struct iot_mqtt_pub_message {
    std::string data;
    std::string topic;
    uint8_t qos;
} iot_mqtt_pub_message_t;


/**
 * A callback function to invoke when a message which is subscribed to is received.
 *
 * @param[in] topic Topic on which the message was received
 * @param[in] payload A pointer to the received data.
 * @param[in] payload_len The length of the received data.
 * @param[in] priv_data A pointer to the private data passed during subscribing.
 */
typedef void (*iot_mqtt_subscribe_cb_t)(std::string topic, void *data, size_t len, void *priv_data);

/**
 * A struct of an mqtt subscription.
 */
typedef struct iot_mqtt_subscribe {
    std::string topic;             /** The subscription's topic. */
    uint8_t qos;                   /** The quality of service for the subscription. */
    iot_mqtt_subscribe_cb_t cb;    /** The callback to invoke when a message is received on the sub topic. */
} iot_mqtt_subscribe_t;
