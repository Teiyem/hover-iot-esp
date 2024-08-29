#include <cJSON.h>
#include "iot_common.h"
#include "iot_device_defs.h"
#include "esp_err.h"

static constexpr const char *TAG = "IotDevice"; /* A constant used to identify the source of the log message of this. */

/**
 * Creates a float iot_val_t value type.
 *
 * @param[in] num The float to encapsulate.
 * @return iot_val_t The value containing the float.
 */
iot_val_t iot_val_float(float num)
{
    return { .is_null= false, .f = num, .type = IOT_VAL_TYPE_FLOAT };
}

/**
 * Creates an integer iot_val_t value type.
 *
 * @param[in] num The integer to encapsulate.
 * @return iot_val_t The value containing the integer.
 */
iot_val_t iot_val_int(uint32_t num)
{
    return { .is_null= false, .i = num, .type = IOT_VAL_TYPE_INTEGER };
}

/**
 * Creates a boolean iot_val_t value type.
 *
 * @param[in] num The boolean to encapsulate.
 * @return iot_val_t The value containing the boolean.
 */
iot_val_t iot_val_bool(bool boolean)
{
    return { .is_null= false, .b = boolean , .type = IOT_VAL_TYPE_BOOLEAN };
}

/**
 * Creates a string iot_val_t value type.
 *
 * @param[in] str A pointer to the string to encapsulate.
 * @return iot_val_t The iot value containing the string.
 */
iot_val_t iot_val_str(char *str)
{
    return {.is_null= false, .s = str, .type = IOT_VAL_TYPE_STRING };
}

/**
 * Creates a device.
 *
 * @param[in] name The device's name.
 * @param[in] type The device's type.
 * @return iot_device_info_t A pointer to the created device.
 * @note free the memory once done.
 */
iot_device_info_t *iot_device_create(std::string name, iot_device_type_t type)
{
    iot_device_info_t *info = iot_allocate_mem<iot_device_info_t>((sizeof(iot_device_info_t)));

    info->device_name = name;
    info->device_type = type;
    info->attributes = {};
    info->services = {};

    return info;
}

/**
 * Creates an attribute.
 *
 * @param[in] name The attribute's name.
 * @param[in] value The attribute's value.
 * @param[in] params The device' attribute parameters.
 * @param[in] is_primary Whether the attribute is a primary one.
 * @return iot_attribute_t The created attribute.
 */
iot_attribute_t iot_attribute_create(std::string name, iot_val_t value, bool is_primary)
{
    return  {.name = name, .is_primary = is_primary, .value = value, .params = {}};
}

/**
 *  Adds a parameter to an attribute, if it doesn't already exist.
 *
 * @param[in] attribute A pointer to the attribute to add the parameter to.
 * @param[in] key The parameter's key.
 * @param[in] value The parameter's value.
 * @returns ESP_OK on success, otherwise ESP_ERR_INVALID_ARG for duplicate values.
 */
esp_err_t iot_attribute_add_param(iot_attribute_t *attribute, std::string key, iot_val_t value)
{
    for (const auto& param : attribute->params) {
        if (param.key == key) {
            ESP_LOGE(TAG, "%s: Param with key -> %s has already added.", __func__ ,key.c_str());
            return ESP_ERR_INVALID_ARG;
        }
    }

    attribute->params.push_back(iot_param_t(key, value));

    ESP_LOGD(TAG, "%s: Added parameter with name ->  %s to the device",  __func__,  key.c_str());

    return ESP_OK;
}

/**
 * Adds a service to a device, if it doesn't already exist.
 *
 * @param[in] device A pointer to the device to add the service to.
 * @param[in] name The service's name.
 * @param[in] enabled Whether the service is enabled or not.
 * @param[in] core_service Whether the service is a core services.
 * @returns ESP_OK on success, otherwise ESP_ERR_INVALID_ARG for duplicate values.
 */
esp_err_t iot_device_add_service(iot_device_info_t *device, std::string name, bool enabled, bool core_service)
{
    for (const auto& service : device->services) {
        if (service.name == name) {
            ESP_LOGE(TAG, "%s: Service with name -> %s has already added.", __func__ ,name.c_str());
            return ESP_ERR_INVALID_ARG;
        }
    }

    iot_device_service_t service = {.name = name, .enabled = enabled, .core_service = core_service};

    device->services.push_back(service);

    ESP_LOGD(TAG, "%s: Added service with name ->  %s to the device",  __func__, name.c_str());

    return ESP_OK;
}

/**
 * Adds an attribute to the device.
 *
 * @param[in] device  The device to add the attribute to.
 * @param[in] attribute The attribute to add.
 * @returns ESP_OK on success, otherwise ESP_ERR_INVALID_ARG for duplicate values.
 */
esp_err_t iot_device_add_attribute(iot_device_info_t *device, iot_attribute_t attribute)
{
    for (const auto& _attribute : device->attributes) {
        if (_attribute.name == attribute.name) {
            ESP_LOGE(TAG, "%s: Attribute [name: %s] has already added.", __func__, attribute.name.c_str());
            return ESP_ERR_INVALID_ARG;
        }
    }

    device->attributes.push_back(attribute);

    ESP_LOGD(TAG, "%s: Added attribute [name: %s] to the device",  __func__,  attribute.name.c_str());

    return ESP_OK;
}

/**
 * Creates an attribute request data.
 *
 * @param[in] name The name of the attribute to create the request data for.
 * @return iot_attribute_req_data_t The attribute request data.
 */
iot_attribute_req_data_t iot_attribute_create_read_req_data(std::string name)
{
    return {
        .name = name,
        .value {}
    };
}

/**
 * Adds an iot_val_t value to a json object.
 *
 * @param json A pointer to the object to add the value to.
 * @param val The value to add.
 * @returns ESP_OK on success, otherwise an error code.
 */
 esp_err_t iot_val_add_to_json(cJSON *json, iot_val_t val)
{
    std::string value;
    std::string type;

    switch (val.type)
    {
        case IOT_VAL_TYPE_BOOLEAN:
            value = std::to_string(val.b).c_str();
            type = IOT_VAL_TYPE_BOOLEAN_STR;
            ESP_LOGD(TAG, "%s: Added bool [value: %d] to json object",  __func__,  val.b);
            break;
        case IOT_VAL_TYPE_INTEGER:
            value = std::to_string( val.i).c_str();
            type = IOT_VAL_TYPE_INTEGER_STR;
            ESP_LOGD(TAG, "%s: Added int [value: %lu] to json object",  __func__,  val.i);
            break;
        case IOT_VAL_TYPE_FLOAT:
            value = std::to_string(val.f).c_str();
            type = IOT_VAL_TYPE_FLOAT_STR;
            ESP_LOGD(TAG, "%s: Added float [value: %f] to json object",  __func__,  val.f);
            break;
        case IOT_VAL_TYPE_LONG:
            value = std::to_string(val.l).c_str();
            type = IOT_VAL_TYPE_LONG_STR;
            ESP_LOGD(TAG, "%s: Added long [value: %llu] to json object",  __func__,  val.l);
            break;
        case IOT_VAL_TYPE_STRING:
            value = val.s;
            type = IOT_VAL_TYPE_STRING_STR;
            ESP_LOGD(TAG, "%s: Added bool [value: %s] to json object",  __func__,  val.s);
            break;
        default:
            ESP_LOGE(TAG, "%s: Invalid value [type: %d", __func__, val.type);
            return ESP_ERR_INVALID_ARG;
    }

    cJSON_AddStringToObject(json, "value", value.c_str());
    cJSON_AddStringToObject(json, "type", type.c_str());

    return ESP_OK;
}
