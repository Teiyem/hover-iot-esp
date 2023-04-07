#pragma once

#include <nvs.h>

/**
 * A class that provides methods for reading and writing data to non-volatile storage
 * using the Non-Volatile Storage (NVS) API.
 */
class iot_storage
{
public:
    iot_storage(const char *namespace_name);
    esp_err_t write(const char *key, const void *data, size_t len);
    esp_err_t read(const char *key, void *buffer, size_t *len);

private:
    static nvs_handle _nvs_handle; /** The handle for the non-volatile storage namespace. */
};
