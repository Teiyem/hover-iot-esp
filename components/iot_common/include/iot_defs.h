#pragma once

#define IOT_MAX_SSID_LENGTH 32     /**< The IEEE standard maximum ssid length .*/
#define IOT_MAX_PASSWORD_LENGTH 64 /**< The IEEE standard maximum wifi password length .*/
#define IOT_MAX_DEVICE_NAME 20     /**< The maximum allowed characters for a device name.*/
#define IOT_MAX_SERVER_BASE_URL 40 /**< The maximum allowed characters for a server's base url.*/

/**
 * A struct that represents application WiFi credentials.
 */
typedef struct __packed iot_app_wifi_cred
{
    uint8_t ssid[IOT_MAX_SSID_LENGTH];         /**< The SSID of the WiFi. */
    uint8_t password[IOT_MAX_PASSWORD_LENGTH]; /**< The password of the WiFi. */
    bool recently_set = false;                 /**< Indicates whether the wifi credentials have been set recently or not. */
} iot_app_wifi_cred_t;

/**
 * A struct represent device app data.
 */
typedef struct __packed iot_app_data
{
    char server_url[IOT_MAX_SERVER_BASE_URL]; /**< A pointer to the server's URL.*/
    char device_name[IOT_MAX_DEVICE_NAME];    /**< A pointer to the device's given name.*/
    uint8_t room_id;                          /**< The room id of where the device is located.*/
} iot_app_data_t;

/**
 * An enum of wifi operating modes.
 */
typedef enum iot_wifi_op_mode
{
    IOT_WIFI_APSTA = 0, /**< Access Point and Station mode. */
    IOT_WIFI_STA,       /**< Station mode.*/
    IOT_WIFI_AP,        /**< Access Point mode. */
} iot_wifi_op_mode_e;

/**
 * An enum of application messages.
 */
typedef enum iot_app_message
{
    IOT_APP_MSG_SETUP_OK = 0,        /**< Success message from iot_setup. */
    IOT_APP_MSG_SETUP_FAIL,          /**< Failure message from iot_setup. */
    IOT_APP_MSG_WIFI_CONNECT_OK,     /**< Successful wifi connection message. */
    IOT_APP_MSG_WIFI_CONNECT_FAIL,   /**< Failed wifi connection message. */
    IOT_APP_MSG_WIFI_RECONNECT,      /**< Wifi reconnection message. */
    IOT_APP_MSG_WIFI_RECONNECT_OK,   /**< Successful wifi reconnection message. */
    IOT_APP_MSG_WIFI_RECONNECT_FAIL, /**< Failed wifi reconnection message. */
    IOT_APP_MSG_WIFI_DISCONNECT,     /**< Wifi disconnection message. */
    IOT_APP_MSG_OTA_UPDATE_OK,       /**< Successful OTA update message. */
    IOT_APP_MSG_OTA_UPDATE_FAIL,     /**< Failed OTA update message. */
    IOT_APP_MSG_RESTART_REQUIRED     /**< Message indicating that a restart is required. */
} iot_app_message_e;