#include "iot_device.h"
#include "priv/iot_device_priv.h"

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
 * Destructor for IotDevice class.
 */
IotDevice::~IotDevice(void)
{
    delete _iot_server;
    free(_iot_device_cfg->device_info);
    free(_iot_device_cfg);
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
    IotDeviceInfo info_pb = IOT_DEVICE_INFO__INIT;
    iot_device_info_t *info = _iot_device_cfg->device_info;

    info_pb.uuid = iot_char_s("777YH7HUHUHUHUHUHUHUH");
    info_pb.name = info->device_name.data();
    info_pb.type = iot_device_type_to_str(info->device_type).data();

    info_pb.n_attributes = info->attributes.size();
    info_pb.attributes = iot_allocate_mem<IotAttribute *>(sizeof(IotAttribute *) * info_pb.n_attributes);

    ESP_LOGI(TAG, "%s: Attributes size %d", __func__, info_pb.n_attributes);

    for (int i = 0; i < info_pb.n_attributes; i++) {
        IotAttribute *attribute_pb = iot_allocate_mem<IotAttribute>(sizeof(IotAttribute));
        *attribute_pb = IOT_ATTRIBUTE__INIT;

        IotValue *value = iot_allocate_mem<IotValue>(sizeof(IotValue));
        *value = IOT_VALUE__INIT;

        iot_attribute_t attribute = info->attributes[i];

        attribute_pb->name = attribute.name.data();
        attribute_pb->is_primary = attribute.is_primary;

        esp_err_t ret = iot_val_to_proto_val(attribute.value, value);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "%s: Attribute [name: %s value: %d] has invalid value", __func__, attribute_pb->name,
                     attribute.value.type);
            continue;
        }

        attribute_pb->value = value;

        ESP_LOGD(TAG, "%s: Adding attribute [name: %s type: %d] to response", __func__, attribute_pb->name, attribute_pb->value->type_case);

        attribute_pb->n_params = attribute.params.size();

        if (attribute_pb->n_params > 0) {
            attribute_pb->params = iot_allocate_mem<IotAttribute__ParamsEntry *>(
                    sizeof(IotAttribute__ParamsEntry *) * attribute_pb->n_params);

            for (int j = 0; j < attribute_pb->n_params; j++) {
                IotAttribute__ParamsEntry *entry = iot_allocate_mem<IotAttribute__ParamsEntry>(sizeof(IotAttribute__ParamsEntry));
                *entry = IOT_ATTRIBUTE__PARAMS_ENTRY__INIT;

                IotValue *param_value = iot_allocate_mem<IotValue>(sizeof(IotValue));
                *param_value = IOT_VALUE__INIT;

                iot_param_t param = attribute.params[j];
                entry->key = strdup(param.key.c_str());
                entry->value = param_value;

                ret = iot_val_to_proto_val(param.value, entry->value);

                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "%s: Attribute param [key: %s value: %d] has invalid value", __func__, param.key.c_str(),
                             param.value.type);
                    continue;
                }

                ESP_LOGD(TAG, "%s: Adding attribute param [key: %s , type: %d] to response", __func__, entry->key, entry->value->type_case);

                attribute_pb->params[j] = entry;

            }
        } else {
            attribute_pb->n_params = 0;
            attribute_pb->params = nullptr;
        }

        info_pb.attributes[i] = attribute_pb;
    }

    info_pb.n_services = info->services.size();
    info_pb.services = iot_allocate_mem<IotService *>(sizeof(IotService *) * info_pb.n_services);

    for (int i = 0; i < info_pb.n_services; i++) {
        static IotService service = IOT_SERVICE__INIT;

        service.name = info->services[i].name.data();
        service.enabled = info->services[i].enabled;
        service.is_core_service = info->services[i].core_service;

        ESP_LOGD(TAG, "%s: Adding service [name: %s ] to response", __func__, service.name);

        info_pb.services[i] = &service;
    }

    IotMetadata metadata = IOT_METADATA__INIT;

    metadata.mac_address = info->metadata.mac_address.data();
    metadata.last_updated = info->metadata.last_updated.data();
    metadata.model = info->metadata.model.data();
    metadata.firmware_version = info->metadata.version.data();

    ESP_LOGD(TAG, "%s: Adding metadata [mac_address: %s ] to response", __func__, metadata.mac_address);
    ESP_LOGD(TAG, "%s: Adding metadata [last_updated: %s ] to response", __func__, metadata.last_updated);
    ESP_LOGD(TAG, "%s: Adding metadata [model: %s ] to response", __func__, metadata.model);
    ESP_LOGD(TAG, "%s: Adding metadata [version: %s ] to response", __func__, metadata.firmware_version);

    info_pb.metadata = &metadata;

    size_t proto_size = iot_device_info__get_packed_size(&info_pb);

    uint8_t *buf = iot_allocate_mem<uint8_t>(proto_size);

    if (buf == nullptr) {
        iot_device_info_proto_free(buf, info_pb);
        return ESP_ERR_NO_MEM;
    }

    iot_device_info__pack(&info_pb, buf);

    esp_err_t ret = _iot_server->send_res(req, reinterpret_cast<const char *>(buf), proto_size);

    iot_device_info_proto_free(buf, info_pb);

    if (ret != ESP_OK)
        return _iot_server->send_err(req, iot_error_response_create_proto(
                HTTPD_500_INTERNAL_SERVER_ERROR, iot_char_s("Failed to send HTTP response")
        ));

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
    size_t buf_len = req->content_len;

    char *buf = iot_allocate_mem<char>(buf_len);

    if (buf == nullptr)
        return _iot_server->send_err(req, iot_error_response_create_proto(
                HTTPD_500_INTERNAL_SERVER_ERROR, iot_char_s("Failed to create object")));

    esp_err_t ret = _iot_server->get_payload(req, buf, buf_len);

    if (ret != ESP_OK) {
        free(buf);
        return _iot_server->send_err(req, iot_error_response_create_proto(
                HTTPD_500_INTERNAL_SERVER_ERROR, iot_char_s("Failed to get payload")));
    }

    iot_attribute_req_param_t data;

    ret = iot_ctl_write_from_proto(buf, buf_len, &data);

    free(buf);

    if (ret == ESP_FAIL || ret == ESP_ERR_INVALID_ARG)
        return _iot_server->send_err(req, iot_error_response_create_proto(ret == ESP_ERR_INVALID_ARG ?
                                                                          HTTPD_500_INTERNAL_SERVER_ERROR
                                                                                                     : HTTPD_400_BAD_REQUEST,
                                                                          iot_char_s("Failed to write attributes")));

    ret = _iot_device_cfg->write_cb(&data);

    if (ret != ESP_OK)
        return _iot_server->send_err(req, iot_error_response_create_proto(
                HTTPD_500_INTERNAL_SERVER_ERROR, iot_char_s("Failed to write attributes")));

    ret = _iot_server->send_res(req, nullptr, 0);

    if (ret != ESP_OK)
        return _iot_server->send_err(req, iot_error_response_create_proto(HTTPD_500_INTERNAL_SERVER_ERROR, nullptr));

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
    std::string attribute = _iot_server->get_path_param(req, "attributes/");

    iot_attribute_req_data_t data = {};
    iot_attribute_req_param_t param = {};

    if (attribute.empty()) {
        if (_iot_device_cfg != nullptr) {
            for (const auto &item: _iot_device_cfg->device_info->attributes)
                param.attributes.push_back(iot_attribute_create_read_req_data(item.name));
        }
    } else
        param.attributes.push_back(iot_attribute_create_read_req_data(attribute));

    esp_err_t ret = _iot_device_cfg->read_cb(&param);

    if (ret != ESP_OK) {
        return _iot_server->send_err(req, iot_error_response_create_proto(
                HTTPD_500_INTERNAL_SERVER_ERROR, iot_char_s("Failed to read attributes")));
    }

    IotAttributeResponse *res = nullptr;

    ret = iot_attribute_res_proto_from_req_data(&data, &res);

    if (ret != ESP_OK)
        return _iot_server->send_err(req, iot_error_response_create_proto(
                HTTPD_500_INTERNAL_SERVER_ERROR, iot_char_s("Failed to proto message")));

    size_t proto_size = iot_attribute_response__get_packed_size(res);

    char *buf = iot_allocate_mem<char>(proto_size);

    if (buf == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    iot_attribute_response__pack(res, reinterpret_cast<uint8_t *>(*buf));

    ret = _iot_server->send_res(req, buf, proto_size);

    free(buf);

    if (ret != ESP_OK)
        return _iot_server->send_err(req, iot_error_response_create_proto(
                HTTPD_500_INTERNAL_SERVER_ERROR, iot_char_s("Failed to send HTTP response")));

    return ESP_OK;
}

/**
 * Converts a protobuf message to an attribute ctl data struct.
 *
 * @param[in] buf A point ot the protobuf message buffer to convert.
 * @param[out] data A pointer to the iot attribute ctl data to convert to.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotDevice::iot_ctl_write_from_proto(char *buf, size_t buf_len, iot_attribute_req_param_t *param)
{
    ESP_LOGI(TAG, "%s: Parsing attribute write data", __func__);

    IotAttributeWriteRequest *request = iot_attribute_write_request__unpack(
            nullptr, buf_len, reinterpret_cast<const uint8_t *>(buf));

    if (request == nullptr) {
        ESP_LOGE(TAG, "%s: Failed to parse request data", __func__);
        return ESP_FAIL;
    }

    std::vector<iot_attribute_req_data_t> attributes;

    for (size_t i = 0; i < request->n_attributes; ++i) {
        IotAttributeData *data = request->attributes[i];

        iot_val_type val_type;

        if (data->value->type_case == IOT_VALUE__TYPE_BOOL_VALUE) { val_type = IOT_VAL_TYPE_BOOLEAN; }
        else if (data->value->type_case == IOT_VALUE__TYPE_INT_VALUE) { val_type = IOT_VAL_TYPE_INTEGER; }
        else if (data->value->type_case == IOT_VALUE__TYPE_FLOAT_VALUE) { val_type = IOT_VAL_TYPE_FLOAT; }
        else if (data->value->type_case == IOT_VALUE__TYPE_STRING_VALUE) { val_type = IOT_VAL_TYPE_STRING; }
        else { val_type = IOT_VAL_TYPE_INVALID; }

        if (val_type == IOT_VAL_TYPE_INVALID) {
            iot_attribute_write_request__free_unpacked(request, nullptr);
            return ESP_ERR_INVALID_ARG;
        }

        iot_attribute_req_data_t attribute = iot_attribute_req_data_t(data->name, {});

        iot_val_t val;

        switch (val_type) {
            case IOT_VAL_TYPE_BOOLEAN:
                val.b = data->value->bool_value;
                val.type = val_type;
                val.is_null = false;
                break;
            case IOT_VAL_TYPE_INTEGER:
                val.i = data->value->int_value;
                val.type = val_type;
                val.is_null = false;
                break;
            case IOT_VAL_TYPE_FLOAT:
                val.f = data->value->float_value;
                val.type = val_type;
                val.is_null = false;
                break;
            case IOT_VAL_TYPE_LONG:
                val.l = data->value->long_value;
                val.type = val_type;
                val.is_null = false;
                break;
            case IOT_VAL_TYPE_STRING:
                val.s = data->value->string_value;
                val.type = val_type;
                val.is_null = false;
                break;
            default:
                val.is_null = true;
                break;
        }

        if (attribute.value.is_null) {
            ESP_LOGE(TAG, "%s: Attribute value is invalid", __func__);
            iot_attribute_write_request__free_unpacked(request, nullptr);
            return ESP_FAIL;
        }

        attribute.value = val;
        attributes.push_back(attribute);
    }

    iot_attribute_write_request__free_unpacked(request, nullptr);

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