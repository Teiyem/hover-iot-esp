#include "iot_device.h"

#ifdef CONFIG_IOT_HOVER_MQTT_ENABLED
#include "iot_mqtt.h"
#endif

/**
 * Instance to the http server component.
 */
IotServer *IotDevice::_iot_server{nullptr};

/**
 * The device configuration instance.
 */
iot_device_cfg_t *IotDevice::_iot_device_cfg{nullptr};

/**
 * Initialises a new instance of the IotDevice class.
 */
IotDevice::IotDevice(void)
{
    _iot_server = &IotFactory::create_component<IotServer>();
}

/**
 * Destroys the IotDevice class.
 */
IotDevice::~IotDevice(void)
{
    free(_iot_device_cfg->device_info);
}

/**
 * Initializes the iot device component.
 *
 * @param[in] device_cfg A pointer to the device configuration.
 * @return Returns ESP_OK on success, otherwise an error code
 */
esp_err_t IotDevice::init(iot_device_cfg_t *cfg)
{
    esp_err_t ret = validate_cfg(cfg);

    if (ret != ESP_OK)
        return ret;

    _iot_device_cfg = cfg;

    ret = _register_route("info", HTTP_GET, on_info);

    ret |= _register_route("attributes", HTTP_GET, on_read);

    if (ret != ESP_OK)
        return ret;

    switch (_iot_device_cfg->req_mode) {
        case IOT_ATTRIBUTE_CB_RW:
            ret = register_routes(HTTP_GET);
            ret |= register_routes(HTTP_POST);
            break;
        case IOT_ATTRIBUTE_CB_READ:
            ret = register_routes(HTTP_POST);
            break;
        case IOT_ATTRIBUTE_CB_WRITE:
            ret = register_routes(HTTP_POST);
            break;
        default:
            ESP_LOGE(TAG, "%s: Invalid cb mode received", __func__);
            return ESP_ERR_INVALID_ARG;
    }

    return ret;
}

/**
 * Validates the iot device configuration.
 *
 * @param[in] config The iot device configuration to validate.
 * @return Returns ESP_OK on success, otherwise an error code
 */
esp_err_t IotDevice::validate_cfg(const iot_device_cfg_t *cfg)
{
    esp_err_t ret = ESP_ERR_INVALID_ARG;

    if (cfg == nullptr) {
        ESP_LOGE(TAG, "%s: The device configuration is required.", __func__);
        return ret;
    }

    if (cfg->notify_cfg != nullptr) {
        if (cfg->notify_cfg->callback_handle == nullptr) {
            ESP_LOGE(TAG, "%s: The notify callback is required.", __func__);
            return ret;
        }

        if (cfg->notify_cfg->notify_cb == nullptr) {
            ESP_LOGE(TAG, "%s: The notify callback is required.", __func__);
            return ret;
        }
    }

    if (cfg->req_mode == IOT_ATTRIBUTE_CB_RW && (cfg->read_cb == nullptr || cfg->write_cb == nullptr)) {
        ESP_LOGE(TAG, "%s: Both read and write callbacks are required for IOT_CAP_CB_READ_WRITE", __func__);
        return ret;
    } else if (cfg->req_mode == IOT_ATTRIBUTE_CB_READ && (cfg->read_cb == nullptr || cfg->write_cb != nullptr)) {
        ESP_LOGE(TAG, "%s: Only the read callback is required for IOT_CAP_CB_READ", __func__);
        return ret;
    } else if (cfg->req_mode == IOT_ATTRIBUTE_CB_WRITE && (cfg->read_cb != nullptr || cfg->write_cb == nullptr)) {
        ESP_LOGE(TAG, "%s: Only the write callback is required for IOT_CAP_CB_WRITE", __func__);
        return ret;
    }

    return ESP_OK;
}

/**
 * Registers the HTTP route for the device using the group and given HTTP method.
 *
 * @param[in] method The http method for which the route needs to be registered.
 * @return Returns ESP_OK on success, otherwise an error code
 */
esp_err_t IotDevice::register_routes(httpd_method_t method)
{
    esp_err_t ret = ESP_FAIL;

    if (method == HTTP_GET)
        ret = _iot_server->register_route("attributes/*", method, on_read);
    else
        ret = _register_route("attributes", method, on_write);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to register device route for capabilities [http_method: %d]. Error [reason: %s]",
                 __func__, method, esp_err_to_name(ret));
    }

    return ret;
}

/**
 * Registers the given HTTP path, HTTP method and handler.
 *
 * @param[in] path   The URL path to register.
 * @param[in] method The http method of the route to register.
 * @param[in] handler The function to handle requests to the path.
 * @return Returns ESP_OK on success, otherwise an error code
 */
esp_err_t IotDevice::_register_route(const char *path, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *))
{
    return _iot_server->register_route(path, method, handler);
}

/**
 * Handles a request to get the device information.
 *
 * @param[in] req The HTTP request object.
 * @return ESP_OK.
 */
esp_err_t IotDevice::on_info(httpd_req_t *req)
{
    ESP_LOGI(TAG, "%s: Received request to get device information.", __func__ );

    cJSON *root = cJSON_CreateObject();

    iot_device_info_t *info = _iot_device_cfg->device_info;

    cJSON_AddStringToObject(root, "uuid", _iot_device_cfg->device_info->uuid.data());
    cJSON_AddStringToObject(root, "name", info->device_name.data());
    cJSON_AddStringToObject(root, "type", iot_device_type_to_str(info->device_type).data());

    cJSON *attributes = cJSON_AddArrayToObject(root, "attributes");

    ESP_LOGI(TAG, "%s: Attributes size %d", __func__, info->attributes.size());

    for (int i = 0; i < info->attributes.size(); i++) {

        iot_attribute_t attribute = info->attributes[i];

        cJSON *j_attribute = cJSON_CreateObject();

        cJSON_AddStringToObject(j_attribute, "name", attribute.name.data());

        esp_err_t ret = iot_val_add_to_json(j_attribute, attribute.value);

        if (ret != ESP_OK) {
            continue;
        }

        cJSON_AddBoolToObject(j_attribute, "is_primary", attribute.is_primary);

        if (attribute.params.size() > 0) {
            cJSON *parameters = cJSON_AddArrayToObject(j_attribute, "parameters");

            for (int j = 0; j < attribute.params.size(); j++) {
                cJSON *j_param = cJSON_CreateObject();
                iot_param_t param = info->attributes[i].params[j];

                cJSON_AddStringToObject(j_param, "key", param.key.c_str());

                ret = iot_val_add_to_json(j_param, param.value);

                if (ret != ESP_OK) {
                    continue;
                }

                cJSON_AddItemToArray(parameters, j_param);
            }
        }

        cJSON_AddItemToArray(attributes, j_attribute);
    }

    cJSON *services = cJSON_AddArrayToObject(root,"services");

    for (int i = 0; i < info->services.size(); i++) {
        cJSON  *service = cJSON_CreateObject();

        cJSON_AddStringToObject(service, "name",info->services[i].name.data());
        cJSON_AddBoolToObject(service, "enabled",info->services[i].enabled);
        cJSON_AddBoolToObject(service, "core_service",info->services[i].core_service);

        ESP_LOGD(TAG, "%s: Adding service [name: %s ] to response", __func__, info->services[i].name.data());

        cJSON_AddItemToArray(services, service);
    }

    cJSON *metadata = cJSON_AddObjectToObject(root, "metadata");

    cJSON_AddStringToObject(metadata, "mac_address",info->metadata.mac_address.data());
    cJSON_AddStringToObject(metadata, "last_updated",info->metadata.last_updated.data());
    cJSON_AddStringToObject(metadata, "model",info->metadata.model.data());
    cJSON_AddStringToObject(metadata, "version",info->metadata.version.data());

    char *buf = cJSON_Print(root);

    if (buf == nullptr) {
        return _iot_server->send_err(req, IOT_HTTP_SERIALIZATION_ERR);
    }

    esp_err_t ret = _iot_server->send_res(req, buf);

    iot_free(buf);
    cJSON_Delete(root);

    if (ret != ESP_OK)
        return _iot_server->send_err(req,"Failed to send HTTP response");

    return ESP_OK;
}

/**
 * Handles a request to write a device's attribute.
 *
 * @param[in] req The HTTP request object.
 * @return ESP_OK.
 */
esp_err_t IotDevice::on_write(httpd_req_t *req)
{
    size_t buf_len = req->content_len + 1;

    char *buf = iot_allocate_mem<char>(buf_len);

    if (buf == nullptr)
        return _iot_server->send_err(req, "Failed to create object");

    esp_err_t ret = _iot_server->get_body(req, buf, buf_len);

    if (ret != ESP_OK) {
        iot_free(buf);
        return _iot_server->send_err(req, "Failed to get request body");
    }

    iot_attribute_req_param_t data;

    ret = iot_attribute_req_from_json(buf, &data);

    iot_free(buf);

    if (ret == ESP_FAIL || ret == ESP_ERR_INVALID_ARG)
        return _iot_server->send_err(req, IOT_HTTP_DESERIALIZATION_ERR, ret == ESP_FAIL ?
                                                                        IOT_HTTP_STATUS_500_INT_SERVER_ERROR
                                                                                                     : IOT_HTTP_STATUS_400_BAD_REQUEST);

    ret = _iot_device_cfg->write_cb(&data);

    if (ret != ESP_OK)
        return _iot_server->send_err(req, "Failed to write attributes");

    ret = _iot_server->send_res(req, "Write Successful", true);

    if (ret != ESP_OK)
        return _iot_server->send_err(req, nullptr);

    return ESP_OK;
}

/**
 * Handles a request to read a device's attribute.
 *
 * @param[in] req The HTTP request object.
 * @return ESP_OK.
 */
esp_err_t IotDevice::on_read(httpd_req_t *req)
{
    ESP_LOGI(TAG, "%s: Received request to read attribute", __func__ );

    std::string name = _iot_server->get_path_param(req, "attributes/");

    iot_attribute_req_data_t data = {};
    iot_attribute_req_param_t param = {};

    if (name.empty()) {
        if (_iot_device_cfg != nullptr) {
            for (const auto &item: _iot_device_cfg->device_info->attributes)
                param.attributes.push_back(iot_attribute_create_read_req_data(item.name));
        }
    } else
        param.attributes.push_back(iot_attribute_create_read_req_data(name));

    esp_err_t ret = _iot_device_cfg->read_cb(&param);

    if (ret != ESP_OK) {
        return _iot_server->send_err(req,"Failed to read attributes");
    }

    cJSON *response = cJSON_CreateArray();

    for (const auto &item: param.attributes) {
        cJSON *attribute = cJSON_CreateObject();

        cJSON_AddStringToObject(attribute, "name", item.name.c_str());

        ret = iot_val_add_to_json(attribute, item.value);

        if (ret != ESP_OK)
            return _iot_server->send_err(req,IOT_HTTP_SERIALIZATION_ERR);

        cJSON_AddItemToArray(response, attribute);
    }

    char *buf = cJSON_Print(response);

    if (buf == nullptr)
        return _iot_server->send_err(req, "Failed to create json");

    ret = _iot_server->send_res(req, buf, 0);

    iot_free(buf);
    cJSON_Delete(response);

    if (ret != ESP_OK)
        return _iot_server->send_err(req, "Failed to send http response");

    return ESP_OK;
}

/**
 * Deserializes the attribute write request json to an attribute ctl data struct.
 *
 * @param[in] buf A point ot the json to deserialize.
 * @param[out] data A pointer to the iot attribute ctl data.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotDevice::iot_attribute_req_from_json(char *buf, iot_attribute_req_param_t *param)
{
    ESP_LOGI(TAG, "%s: Parsing attribute write data", __func__);

    cJSON *root = cJSON_Parse(buf);

    if (root == nullptr) {
        ESP_LOGE(TAG, "%s: Failed to deserialize request data, [reason: %s]", __func__, cJSON_GetErrorPtr());
        return ESP_FAIL;
    }

    std::vector<iot_attribute_req_data_t> attributes;

    int count = cJSON_GetArraySize(root);

    if (count == 0) {
        ESP_LOGE(TAG, "%s: Request contains zero attribute write data", __func__);
        cJSON_free(root);
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "%s: Found attributes [count: %d]", __func__, count);

    iot_zero_mem(param, sizeof(iot_attribute_req_param_t));

    for (size_t i = 0; i < count; ++i) {
        cJSON *item = cJSON_GetArrayItem(root, i);

        if (item == nullptr) {
            ESP_LOGE(TAG, "%s: Failed to read attribute from array[reason: %s]", __func__, cJSON_GetErrorPtr());
            cJSON_free(root);
            return ESP_FAIL;
        }

        cJSON *item_name = cJSON_GetObjectItem(item, "name");
        cJSON *item_value = cJSON_GetObjectItem(item, "value");
        cJSON *item_type = cJSON_GetObjectItem(item, "type");

        if (item_name == nullptr || item_value == nullptr || item_type == nullptr) {
            ESP_LOGE(TAG, "%s: Attribute data is missing, [name: %d, value: %d, type: %d]", __func__,
                     item_value != nullptr, item_value != nullptr, item_type != nullptr);
            cJSON_free(root);
            return ESP_ERR_INVALID_ARG;
        }

        std::string name = item_name->string;
        std::string type = item_type->valuestring;

        ESP_LOGI(TAG, "%s: Reading attribute [name: %s, type: %s]", __func__, name.data(), type.data());

        iot_attribute_req_data_t attribute = {.name = item_name->valuestring, .value = {}};

        iot_val_t val;
        val.is_null = false;

        if (type == IOT_VAL_TYPE_BOOLEAN_STR) {
            val.type = IOT_VAL_TYPE_BOOLEAN;
            val.b =  (bool)item_value->valueint;
        } else if (type == IOT_VAL_TYPE_INTEGER_STR) {
            val.type = IOT_VAL_TYPE_INTEGER;
            val.i = item_value->valueint;
        } else if (type == IOT_VAL_TYPE_LONG_STR) {
            val.type = IOT_VAL_TYPE_LONG;
            val.l = item_value->valueint;
        } else if (type == IOT_VAL_TYPE_FLOAT_STR) {
            val.type = IOT_VAL_TYPE_FLOAT;
            val.i = (float)item_value->valuedouble;
        } else if (type == IOT_VAL_TYPE_STRING_STR) {
            val.type = IOT_VAL_TYPE_STRING;
            val.s = strdup(item_value->valuestring);
        } else {
            ESP_LOGE(TAG, "%s: Received invalid value type", __func__);
            val.type = IOT_VAL_TYPE_INVALID;
        }

        if (val.type == IOT_VAL_TYPE_INVALID) {
            cJSON_free(root);
            return ESP_ERR_INVALID_ARG;
        }

        attributes.push_back(attribute);
    }

    cJSON_free(root);

    param->attributes = attributes;

    return ESP_OK;
}

/**
 * Gets the string representation of the device type enum.
 *
 * @param type The type to get the string for.
 * @return std::string The string device type.
 */
std::string IotDevice::iot_device_type_to_str(iot_device_type_t type)
{
    switch (type) {
        case IOT_DEVICE_TYPE_SWITCH:
            return "Switch";
        case IOT_DEVICE_TYPE_TEMPERATURE:
            return "Temperature";
        case IOT_DEVICE_TYPE_HUMIDITY:
            return "Humidity";
        case IOT_DEVICE_TYPE_LIGHT:
            return "Light";
        case IOT_DEVICE_TYPE_FAN:
            return "Fan";
        case IOT_DEVICE_TYPE_MOTION:
            return "Motion";
        case IOT_DEVICE_TYPE_CONTACT:
            return "Contact";
        case IOT_DEVICE_TYPE_OUTLET:
            return "Outlet";
        case IOT_DEVICE_TYPE_PLUG:
            return "Plug";
        case IOT_DEVICE_TYPE_LOCK:
            return "Lock";
        case IOT_DEVICE_TYPE_BLINDS:
            return "Blinds";
        case IOT_DEVICE_TYPE_THERMOSTAT:
            return "Thermostat";
        case IOT_DEVICE_TYPE_ALARM:
            return "Alarm";
        default:
            return "Other";
    }
}

#ifdef CONFIG_IOT_HOVER_MQTT_ENABLED

/**
 * Gets whether the device is subscribed to an mqtt topic.
 * @return true if subscribed otherwise false.
 */
bool IotDevice::subscribed_to_mqtt() const
{
    return _mqtt_subscribed;
}

/**
 * Subscribes to mqtt.
 */
void IotDevice::subscribe_to_mqtt()
{
    auto *mqtt = &IotFactory::create_component<IotMqtt>();

    iot_mqtt_subscribe_t subscribe = {
        .topic =  "hover/iot/device/" + _iot_device_cfg->device_info->metadata.mac_address  + "/attribute/",
        .cb = on_data
    };

    esp_err_t ret = mqtt->subscribe(subscribe);

    _mqtt_subscribed = ret == ESP_OK;
}

void IotDevice::on_data(std::string topic, std::string data, size_t len, void *priv_data)
{
    ESP_LOGI(TAG, "%s: Received mqtt message [topic: %s, data: %s, len: %d]", __func__, topic.data(), data.data(), len);
}

#endif
