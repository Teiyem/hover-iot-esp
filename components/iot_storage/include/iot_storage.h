#pragma once

#include "nvs_flash.h"
#include "iot_common.h"

/**
 * A class that provides methods for reading and writing data to non-volatile storage.
 */
class iot_storage
{
public:
    iot_storage(const char *name = "nvs");
    ~iot_storage(void);
    esp_err_t write(const char *key, const void *input, size_t len);
    esp_err_t read(const char *key, void *buf, size_t len);
    esp_err_t verify(const char *key, const void *input, size_t len);
    esp_err_t erase(const char *key = nullptr);

private:
    static constexpr const char *tag = "iot_storage"; /**< A constant used to identify the source of the log message of this class. */
    static nvs_handle _handle;                        /**< The handle for the non-volatile storage namespace. */
    bool failed_to_open = false;                      /**< Indicates whether we managed to open the namespace or not. */
};
