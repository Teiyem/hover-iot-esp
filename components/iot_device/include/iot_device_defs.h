#pragma once

#include <vector>
#include "esp_err.h"
#include "esp_timer.h"
#include <string>
#include <cJSON.h>

// region STANDARD PARAMETERS
#define IOT_VAL_TYPE_INTEGER_STR  "integer"    /**< A string representation of an integer value type. */
#define IOT_VAL_TYPE_FLOAT_STR    "float"      /**< A string representation of float value type. */
#define IOT_VAL_TYPE_LONG_STR     "long"       /**< A string representation of a long value type */
#define IOT_VAL_TYPE_STRING_STR   "string"     /**< A string representation of a string value type. */
#define IOT_VAL_TYPE_BOOLEAN_STR  "bool"    /**< A string representation of a boolean value type. */
// endregion

// region STANDARD PARAMETERS
#define IOT_ATTR_PARAM_MAX "Max"        /**< The name of a max param. */
#define IOT_ATTR_PARAM_MIN "Min"        /**< The name of a min param. */
#define IOT_ATTR_PARAM_R   "Read"       /**< The name of a min param. */
#define IOT_ATTR_PARAM_W   "Write"      /**< The name of a min param. */
#define IOT_ATTR_PARAM_UNIT "Unit"      /**< The name of a unit param. */
// endregion

// region STANDARD ATTRIBUTE NAMES
#define IOT_ATTR_NAME_POWER          "Power"        /**< The name of a power attribute. */
#define IOT_ATTR_NAME_BRIGHTNESS     "Brightness"   /**< The name of a power attribute. */
#define IOT_ATTR_NAME_COLOR          "Color"        /**< The name of a color attribute. */
#define IOT_ATTR_NAME_HUE            "Hue"          /**< The name of a hue attribute. */
#define IOT_ATTR_NAME_TEMPERATURE    "Temperature"  /**< The name of a temperature attribute. */
#define IOT_ATTR_NAME_HUMIDITY       "Humidity"     /**< The name of a humidity attribute. */
// endregion

/**
 * An enum of the different types of value types.
 */
typedef enum iot_val_type
{
    IOT_VAL_TYPE_BOOLEAN = 0, /**< A boolean value type. */
    IOT_VAL_TYPE_INTEGER,     /**< An Integer value type. */
    IOT_VAL_TYPE_FLOAT,       /**< A Float value type. */
    IOT_VAL_TYPE_LONG,        /**< A long value type. */
    IOT_VAL_TYPE_STRING,      /**< A String value type. */
    IOT_VAL_TYPE_INVALID,     /**< An invalid value type. */
} iot_val_type_e;

/**
 * A struct of a value type.
 */
typedef struct iot_val
{
    bool is_null;
    union {
        bool b;           /**< A boolean value. */
        uint32_t i;       /**< An unsigned 32-bit integer value. */
        uint64_t l;       /**< An unsigned 64-bit integer value. */
        float f;          /**< A floating point value. */
        char *s;          /**< A string value. */
    };
    iot_val_type_e type;  /**< The value type */
} iot_val_t;

/**
 * A struct of an attribute's parameter.
 */
typedef struct iot_param
{
    std::string key;         /**< The parameter's key. */
    iot_val_t value;         /**< The parameter's value. */
} iot_param_t;

/**
 * A struct of a device's attribute.
 */
typedef struct iot_attribute
{
    std::string name;                    /**< The attribute's name. */
    bool is_primary = false;             /**< Indicates whether the attribute is a primary attribute. */
    iot_val_t value;                     /**< The attribute's value. */
    std::vector<iot_param_t> params{};   /**< The attribute's parameters. */
} iot_attribute_t;

/**
 * An enum of the different support device types.
 */
typedef enum iot_device_type
{
    IOT_DEVICE_TYPE_SWITCH = 0,    /**< A device that is a switch. */
    IOT_DEVICE_TYPE_TEMPERATURE,   /**< A device that is a temperature sensor. */
    IOT_DEVICE_TYPE_HUMIDITY,      /**< A device that is a humidity sensor. */
    IOT_DEVICE_TYPE_LIGHT,         /**< A device that is a light. */
    IOT_DEVICE_TYPE_FAN,           /**< A device that is a fan. */
    IOT_DEVICE_TYPE_MOTION,        /**< A device that is a motion sensor. */
    IOT_DEVICE_TYPE_CONTACT,       /**< A device that is a contact sensor. */
    IOT_DEVICE_TYPE_OUTLET,        /**< A device that is an outlet. */
    IOT_DEVICE_TYPE_PLUG,          /**< A device that is a plug. */
    IOT_DEVICE_TYPE_LOCK,          /**< A device that is a lock. */
    IOT_DEVICE_TYPE_BLINDS,        /**< A device that is binds. */
    IOT_DEVICE_TYPE_THERMOSTAT,    /**< A device that is a thermostat. */
    IOT_DEVICE_TYPE_ALARM,         /**< A device that is an alarm. */
    IOT_DEVICE_TYPE_OTHER          /**< A device type which is not a standard. */
} iot_device_type_t;

/**
 * An enum of the different supported attribute request mode types.
 */
typedef enum iot_attribute_req_mode
{
    IOT_ATTRIBUTE_CB_RW = 0,  /**< Indicates that both read and write requests for attributes are supported. */
    IOT_ATTRIBUTE_CB_READ,    /**< Indicates that only read requests for attributes is supported. */
    IOT_ATTRIBUTE_CB_WRITE,   /**< Indicates that only write requests for attributes is supported. */
} iot_attribute_req_mode_e;

/**
 * A struct of a device service.
 */
typedef struct iot_device_service
{
    std::string name;       /**< The service's name. */
    bool enabled;           /**< Indicates whether the service is enabled. */
    bool core_service;      /**< Inndicates whether the service is a core service. */
} iot_device_service_t;

/**
 * A struct of a device's metadata.
 */
typedef struct iot_device_meta
{
    std::string mac_address;         /**< The device's mac address. */
    std::string model;               /**< The device's model. */
    std::string version;             /**< The device's firmware version. */
    std::string last_updated;        /**< The device's last firmware update timestamp. */
} iot_device_meta_t;

/**
 * A struct of a device's information.
 */
typedef struct iot_device_info
{
    std::string device_name;                       /**< The device's name. */
    iot_device_type_t device_type;                 /**< The device's type. */
    iot_device_meta_t metadata;                    /**< The device's metadata. */
    std::string uuid{};                            /**< The device's uuid. */
    std::vector<iot_attribute_t> attributes;       /**< The device's attributes list. */
    std::vector<iot_device_service_t> services;    /**< The device's services list. */
} iot_device_info_t;

/**
 * A struct of an attribute request data.
 */
typedef struct iot_attribute_req_data
{
    std::string name;             /**< The attribute's name. */
    iot_val_t value;              /**< The attribute's value. */
} iot_attribute_req_data_t;

/**
 * An attribute request parameter struct.
 */
typedef struct iot_attribute_req_param
{
    std::vector<iot_attribute_req_data_t> attributes; /**< The list of attribute data. **/
} iot_attribute_req_param_t;

/**
 * A callback function for attribute write requests.
 *
 * @param[in] param A pointer to the attribute write parameter.
 * @return ESP_OK on success, otherwise an error code.
 */
typedef esp_err_t (*iot_attribute_write_cb_t)(iot_attribute_req_param_t *param);

/**
 * A callback function for attribute read requests.
 *
 * @param[in,out] param A pointer to the attribute read parameter.
 * @return  ESP_OK on success, otherwise an error code.
 */
typedef esp_err_t (*iot_attribute_read_cb_t)(iot_attribute_req_param_t *param);

/**
 * A callback function to notify client with the iot device's attributes values.
 *
 * @param[in,out] param A pointer to the attribute notify parameter.
 * @return ESP_OK on success, otherwise an error code.
 */
typedef esp_err_t (*iot_attribute_notify_cb_t)(iot_attribute_req_param_t *param);

/**
 * A struct of an attribute notification configuration.
 */
typedef struct iot_notify_attribute_cfg
{
    TimerHandle_t callback_handle;           /**<  Handle for the timer used for notifications. */
    int period = 0;                          /**<  Period (in milliseconds) for the notification timer. */
    iot_attribute_notify_cb_t notify_cb;     /**<  Callback function to be called on notification events. */
} iot_notify_attribute_cfg_t;

/**
 * A struct of a device configuration.
 */
typedef struct iot_device_cfg
{
    iot_device_info_t *device_info;                           /**< A pointer to the device's info. */
    iot_attribute_req_mode_e req_mode = IOT_ATTRIBUTE_CB_RW;  /**< The device's supported attribute request mode. */
    iot_attribute_read_cb_t read_cb;                          /**< The device's callback function for attribute read requests. */
    iot_attribute_write_cb_t write_cb;                        /**< The device's callback function for attribute write requests. */
    iot_notify_attribute_cfg_t *notify_cfg;                   /**< A pointer to the device's attribute notify configuration. */
} iot_device_cfg_t;

// region UTILITY FUNCTIONS
iot_val_t iot_val_float(float num);
iot_val_t iot_val_int(uint32_t num);
iot_val_t iot_val_bool(bool boolean);
iot_val_t iot_val_str(char * str);

iot_attribute_t iot_attribute_create(const char *name, iot_val_t value, bool is_primary = false);
iot_device_info_t *iot_device_create(const char *name, iot_device_type_t type);
iot_attribute_req_data_t iot_attribute_create_ctl_data(iot_val_t value);

esp_err_t iot_attribute_add_param(iot_attribute_t *attribute,const char *key, iot_val_t value);
esp_err_t iot_device_add_service(iot_device_info_t *device, const char *name, bool enabled, bool core_service);
esp_err_t iot_device_add_attribute(iot_device_info_t *device, iot_attribute_t attribute);

iot_attribute_req_data_t iot_attribute_create_read_req_data(std::string name);
esp_err_t iot_val_add_to_json(cJSON *p_json, iot_val_t val);
// endregion