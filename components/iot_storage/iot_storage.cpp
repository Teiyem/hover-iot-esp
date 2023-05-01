#include "iot_storage.h"

/** The handle for the non-volatile storage namespace. */
nvs_handle iot_storage::_handle = 0;

/**
 * Initialises a new instance of the iot_storage class.
 * @param[in] namespace_name A pointer to a string containing the name of the non-volatile storage namespace to be opened.
 */
iot_storage::iot_storage(const char *namespace_name)
{
    if (check_string_validity(namespace_name) != ESP_OK)
    {
        failed_to_open = true;
        return;
    }

    if (_handle == 0)
    {
        esp_err_t ret = nvs_open(namespace_name, NVS_READWRITE, &_handle);

        if (ret != ESP_OK)
        {
            ESP_LOGE(tag, "%s ->  Failed to open nvs for namespace %s -> , reason %s", __func__, namespace_name, esp_err_to_name(ret));
            failed_to_open = true;
            return;
        }
    }
}

/**
 * Destructor for iot_storage class.
 */
iot_storage::~iot_storage(void)
{
    if (failed_to_open)
        return;

    nvs_close(_handle);
}

/**
 * Reads a blob of data from non-volatile storage.
 *
 * @param[in] key A pointer to a string containing the key for the data to be read.
 * @param[in] buffer A pointer to the buffer where the data will be stored.
 * @param[in] len The maximum length of the data to be written.
 * @return An esp_err_t value indicating the success or failure of the read operation.
 */
esp_err_t iot_storage::write(const char *key, const void *data, size_t len)
{
    if (failed_to_open)
        return ESP_ERR_INVALID_STATE;

    esp_err_t ret = nvs_set_blob(_handle, key, data, len);

    if (ret != ESP_OK)
    {
        ESP_LOGE(tag, "%s -> Failed to set blob to nvs, reason -> %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_commit(_handle);

    if (ret != ESP_OK)
    {
        ESP_LOGE(tag, "%s -> Failed to commit to nvs, reason -> %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    return ret;
}

/**
 * Reads a blob of data from non-volatile storage.
 *
 * @param[in] key A pointer to a string containing the key for the data to be read.
 * @param[out] buffer A pointer to the buffer where the data will be stored.
 * @param[in] len The maximum length of the data to be read.
 * @return An esp_err_t value indicating the success or failure of the read operation.
 */
esp_err_t iot_storage::read(const char *key, void *buffer, size_t len)
{
    if (failed_to_open)
        return ESP_ERR_INVALID_STATE;

    return nvs_get_blob(_handle, key, buffer, &len);
}