#pragma once

//region nvs constants

#define IOT_NVS_DEFAULT_PART_NAME "nvs"            /**< The default partition name.*/
#define IOT_NVS_DEFAULT_NAMESPACE "app"            /**< The default partition namespace.*/
#define IOT_NVS_FACTORY_PART_NAME "factory_nvs"    /**< The factory partition name.*/
#define IOT_NVS_FACTORY_NAMESPACE "iot_factory"    /**< The factory partition namespace.*/
#define IOT_NVS_DEVICE_DATA_KEY "iot_device_data"  /**< The device data nvs key .*/
#define IOT_NVS_WIFI_DATA_KEY "iot_wifi_data"      /**< The wifi data nvs key .*/

// endregion

//region maximum len constraints
#define IOT_MAX_SSID_LEN 32          /**< The IEEE standard maximum ssid length .*/
#define IOT_MAX_PASSWORD_LEN 64      /**< The IEEE standard maximum wifi password length .*/
#define IOT_MAX_ANY_NAME_LEN 20      /**< The maximum name length FOR ANY.*/
#define IOT_MAX_ANY_STRING_LEN 255   /**< The maximum long 's base url length.*/
// endregion

#define IOT_DEVICE_OTA_SERVICE "OTA"   /**< The identify used for the ota capability.*/

/**
 * A struct for the store wifi credentials.
 */
typedef struct __packed iot_wifi_data
{
    uint8_t ssid[IOT_MAX_SSID_LEN];            /**< The SSID of the wifi. */
    uint8_t password[IOT_MAX_PASSWORD_LEN];    /**< The password of the wifi. */
} iot_wifi_data_t;

/**
 * A struct for the stored device data.
 */
typedef struct __packed iot_device_data
{
    char server_url[IOT_MAX_ANY_STRING_LEN];         /**< The hover server's URL. */
    char name[IOT_MAX_ANY_NAME_LEN];                 /**< The device's friendly name. */
    char uuid[IOT_MAX_ANY_NAME_LEN];                 /**< The device's uuid and api key. */
    char timezone[IOT_MAX_ANY_NAME_LEN];             /**< The device's timezone. */
} iot_device_data_t;

/**
 * An enum of wifi operating mode types.
 */
typedef enum iot_wifi_op_mode
{
    IOT_WIFI_APSTA = 0, /**< Access Point and Station mode. */
    IOT_WIFI_STA,       /**< Station mode.*/
    IOT_WIFI_AP,        /**< Access Point mode. */
} iot_wifi_op_mode_e;

/**
* A type definition that represents an message.
*/
typedef uint32_t iot_base_message_e;

/**
 * A struct for message queues.
 */
typedef struct iot_queue_message
{
    iot_base_message_e id;  /**< The message's id.*/
    void *data;             /** The message's data */
} iot_queue_message_t;

#define IOT_APP_MSG_START 0 /**< The start of the iot application messages. Reserved 0 to 20. */

/**
 * An enum of application messages.
 */
typedef enum iot_app_message
{
    IOT_APP_MSG_PROV_START = IOT_APP_MSG_START, /**< The provisioning has. */
    IOT_APP_MSG_PROV_OK,                        /**< The provisioning was successful. */
    IOT_APP_MSG_PROV_FAIL,                      /**< The provisioning failed. */
    IOT_APP_MSG_WIFI_CONNECT_OK,                /**< The wifi is successful connected. */
    IOT_APP_MSG_WIFI_CONNECT_FAIL,              /**< The wifi connection failed. */
    IOT_APP_MSG_WIFI_RECONNECT,                 /**< The wifi is reconnecting. */
    IOT_APP_MSG_WIFI_RECONNECT_OK,              /**< The wifi reconnection was successful. */
    IOT_APP_MSG_WIFI_RECONNECT_FAIL,            /**< The wifi reconnection failed. */
    IOT_APP_MSG_WIFI_DISCONNECT,                /**< The wifi disconnected. */
    IOT_APP_MSG_OTA_UPDATE_OK,                  /**< The OTA update process was successful. */
    IOT_APP_MSG_OTA_UPDATE_FAIL,                /**< The OTA update failed. */
    IOT_APP_MSG_RESTART_REQUIRED = 20           /**< Message indicating that a restart is required. */
} iot_app_message_e;


typedef enum iot_core_events {
    IOT_EVENT_REBOOTING = 0
} iot_core_event_e;