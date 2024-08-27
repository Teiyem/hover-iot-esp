#include "iot_common.h"
#include "iot_device_defs.h"
#include "esp_err.h"
#include "iot_device_priv.h"

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
    return iot_attribute_t(name, is_primary,value, {});
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

    iot_device_service_t service = iot_device_service_t(name, enabled, core_service);

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
 * Convert a iot_val_t value to a protobuf _IotValue value.
 *
 * @param[in] iot_val The iot_val_t value to convert from.
 * @param[in] type The type of the (iot_val_t) value
 * @param[in,out] proto_val A pointer to the protobuf value to convert to.
 * @returns ESP_OK on success, otherwise an error code.
 */
esp_err_t iot_val_to_proto_val(iot_val_t iot_val, IotValue *proto_val)
{
    switch (iot_val.type)
    {
        case IOT_VAL_TYPE_BOOLEAN:
            proto_val->bool_value = iot_val.b;
            proto_val->type_case = IOT_VALUE__TYPE_BOOL_VALUE;
            ESP_LOGD(TAG, "%s: Added attribute param bool_value value [value: %d] to the device",  __func__,  iot_val.b);
            return ESP_OK;
        case IOT_VAL_TYPE_INTEGER:
            proto_val->int_value = iot_val.i;
            proto_val->type_case = IOT_VALUE__TYPE_INT_VALUE;
            ESP_LOGD(TAG, "%s: Added attribute param int_value value [value: %lu] to the device",  __func__,  iot_val.i);
            return ESP_OK;
        case IOT_VAL_TYPE_FLOAT:
            proto_val->float_value = iot_val.f;
            proto_val->type_case = IOT_VALUE__TYPE_FLOAT_VALUE;
            ESP_LOGD(TAG, "%s: Added attribute param float_value value [value: %f] to the device",  __func__,  iot_val.f);
            return ESP_OK;
        case IOT_VAL_TYPE_LONG:
            proto_val->long_value = iot_val.l;
            proto_val->type_case = IOT_VALUE__TYPE_LONG_VALUE;
            ESP_LOGD(TAG, "%s: Added attribute param long_value value [value: %llu] to the device",  __func__, iot_val.l);
            return ESP_OK;
        case IOT_VAL_TYPE_STRING:
            proto_val->string_value = iot_val.s;
            proto_val->type_case = IOT_VALUE__TYPE_STRING_VALUE;
            ESP_LOGD(TAG, "%s: Added attribute param string_value value [value: %s] to the device",  __func__, iot_val.s);
            return ESP_OK;
        default:
            return ESP_ERR_INVALID_ARG;
    }
}

/**
 * Serialize the attribute request data into a protobuf attribute response.
 *
 * @param[in] data A pointer to the data to convert from.
 * @param[out] res A pointer to the protobuf to serialize.
 * @return
 */
esp_err_t iot_attribute_res_proto_from_req_data(iot_attribute_req_data_t *data, IotAttributeResponse **res)
{
    static IotAttributeResponse proto = IOT_ATTRIBUTE_RESPONSE__INIT;
    IotAttributeData attribute = IOT_ATTRIBUTE_DATA__INIT;

    attribute.name = iot_char_s(data->name.c_str());

    IotValue value = IOT_VALUE__INIT;

    esp_err_t ret = iot_val_to_proto_val(data->value, &value);

    if (ret != ESP_OK)
        return ret;

    attribute.value = &value;

    proto.attributes = iot_allocate_mem<IotAttributeData *>(sizeof(IotAttributeData *));
    proto.attributes[0] = &attribute;
    proto.n_attributes = 1;

    *res = &proto;

    return ESP_OK;
}

/**
 * Frees the device info protobuf and the buffer.
 *
 * @param[in] info The device protobuf info to free.
 * @param[in] buf A pointer to the buffer to free.
 */
void iot_device_info_proto_free(uint8_t *buf, IotDeviceInfo &info)
{
    for (int i = 0; i < info.n_attributes; i++) {
        if (info.attributes[i]) {
            for (int j = 0; j < info.attributes[i]->n_params; j++) {
                iot_free_one(info.attributes[i]->params[j]->value);
                iot_free_one(info.attributes[i]->params[j]);
            }
            iot_free_one(info.attributes[i]->value);
            iot_free_one(info.attributes[i]);
        }
    }

    iot_free(info.attributes, buf, info.services);
}