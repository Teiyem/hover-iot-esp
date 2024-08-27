#pragma once

#include "iot_common.h"
#include "iot_device_defs.h"
#include "iot_device.pb-c.h"

esp_err_t iot_val_to_proto_val(iot_val_t iot_val, IotValue *proto_val);
esp_err_t iot_attribute_res_proto_from_req_data(iot_attribute_req_data_t *data, IotAttributeResponse **res);
void iot_device_info_proto_free(uint8_t *buf, IotDeviceInfo &info);