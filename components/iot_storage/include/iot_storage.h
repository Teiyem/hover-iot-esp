#pragma once

#include "nvs_flash.h"
#include "iot_common.h"
#include "iot_storage_defs.h"

/**
 * A class that provides methods for reading and writing data to non-volatile storage.
 */
class IotStorage final
{
public:
    IotStorage(const char *partition, const char *name_space);
    ~IotStorage(void);

    esp_err_t write(const iot_nvs_write_params_t *write_params);
    esp_err_t read(const char *key, void *buf, size_t len);
    esp_err_t read(const char *key, void **buf, size_t &len, iot_nvs_val_type_e type);
    esp_err_t erase(const char *key = nullptr);

private:
    static constexpr const char *TAG = "IotStorage"; /**< A constant used to identify the source of the log message of this class. */

    nvs_handle _handle;                              /**< The handle for the non-volatile storage namespace. */
    bool failed_to_open = false;                     /**< Indicates whether we managed to open the namespace or not. */
    void print_stats(const char *partition, const char *name_space);
};
