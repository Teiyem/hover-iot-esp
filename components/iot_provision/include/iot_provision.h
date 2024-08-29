#pragma once

#include "esp_wifi.h"
#include "iot_common.h"
#include "iot_provision_defs.h"
#include "iot_storage.h"
#include "iot_factory.h"

/**
 * A class for handling device setup with smart config.
 */
class IotProvision final
{
public:
    IotProvision(void);
    ~IotProvision(void);
    void start(void);

private:
    static constexpr const char *TAG = "IotProvision"; /* A constant used to identify the source of the log message of this class. */
    static IotStorage *_iot_storage;
    static QueueHandle_t _queue_handle;
    static TaskHandle_t _task_handle;
    static iot_wifi_data_t _wifi_data;

    void init(void);
    esp_err_t get_data(IotStorage *storage, const char *key, char **data, size_t &len, iot_nvs_val_type type = IOT_TYPE_STR);
    static void on_event([[maybe_unused]] void *args, const esp_event_base_t base, int32_t id, void *data);
    static esp_err_t on_data(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                             uint8_t **outbuf, ssize_t *outlen, [[maybe_unused]] void *priv_data);
    static esp_err_t save(iot_device_data_t device_data);
    static void create_service_name(char *service_name);
    static void print_qrcode(const char *name, const char *username, const char *pop, const char *key);
    static BaseType_t send_to_queue(iot_prov_message_e msg);
    [[noreturn]] static void task(void *param);
    void process_message(iot_prov_message_e msg);
};