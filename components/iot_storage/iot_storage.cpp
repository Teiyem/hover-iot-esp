#include "iot_storage.h"

/** The handle for the non-volatile storage namespace. */
nvs_handle iot_storage::_nvs_handle = 0;

/**
 * Initialises a new instance of the iot_storage class.
 * @param namespace_name A pointer to a string containing the name of the non-volatile storage namespace to be opened.
 */
iot_storage::iot_storage(const char *namespace_name)
{
    if (_nvs_handle == 0)
    {
        nvs_open(namespace_name, NVS_READWRITE, &_nvs_handle);
    }
}

/**
 * Reads a blob of data from non-volatile storage.
 *
 * @param key A pointer to a string containing the key for the data to be read.
 * @param buffer A pointer to the buffer where the data will be stored.
 * @param len A pointer to a variable containing the maximum length of the data to be read.
 * @return An esp_err_t value indicating the success or failure of the read operation.
 */
esp_err_t iot_storage::write(const char *key, const void *data, size_t len)
{
    return nvs_set_blob(_nvs_handle, key, data, len);
}

/**
 * Reads a blob of data from non-volatile storage.
 *
 * @param key A pointer to a string containing the key for the data to be read.
 * @param buffer A pointer to the buffer where the data will be stored.
 * @param len A pointer to a variable containing the maximum length of the data to be read.
 * @return An esp_err_t value indicating the success or failure of the read operation.
 */
esp_err_t iot_storage::read(const char *key, void *buffer, size_t *len)
{
    return nvs_get_blob(_nvs_handle, key, buffer, len);
}