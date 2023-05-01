#pragma once

#include "nvs.h"
#include "iot_util.h"

/**
 * A class that provides methods for reading and writing data to non-volatile storage
 * using the Non-Volatile Storage (NVS) API.
 */
class iot_storage
{
public:
    iot_storage(const char *namespace_name);
    ~iot_storage(void);
    esp_err_t write(const char *key, const void *data, size_t len);
    esp_err_t read(const char *key, void *buffer, size_t len);

private:
    static constexpr const char *tag = "iot_storage"; /**< A constant used to identify the source of the log message of this class. */
    static nvs_handle _handle;                        /**< The handle for the non-volatile storage namespace. */
    bool failed_to_open = false;                      /**< Indicates whether we managed to open the namespace or not. */
};
