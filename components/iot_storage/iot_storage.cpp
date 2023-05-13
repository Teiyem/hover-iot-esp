#include "iot_storage.h"

/** The handle for the non-volatile storage namespace. */
nvs_handle iot_storage::_handle{};

/**
 * Initialises a new instance of the iot_storage class.
 * @param[in] name A pointer to a string containing the name of the non-volatile storage namespace to be opened.
 */
iot_storage::iot_storage(const char *name)
{
    ESP_LOGI(tag, "%s -> Opening nvs for namespace %s ->", __func__, name);

    if (iot_verify_string(name) != ESP_OK)
    {
        failed_to_open = true;
        return;
    }

    if (_handle == 0)
    {
        // esp_err_t err = nvs_open_from_partition(part_name, name_space, NVS_READONLY, &handle);
        esp_err_t ret = nvs_open(name, NVS_READWRITE, &_handle);

        if (ret != ESP_OK)
        {
            ESP_LOGE(tag, "%s ->  Failed to open nvs for namespace %s -> , reason %s", __func__, name, esp_err_to_name(ret));
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
 * Writes a data to the non-volatile storage.
 *
 * @param[in] key The key to use when writing the data.
 * @param[in] input A pointer to the data to write.
 * @param[in] len The length of the data to write.
 * @return Returns ESP_OK on success, otherwise an error code
 */
esp_err_t iot_storage::write(const char *key, const void *input, size_t len)
{
    if (failed_to_open)
        return ESP_ERR_INVALID_STATE;

    esp_err_t ret = nvs_set_blob(_handle, key, input, len);

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
 * Reads a data from non-volatile storage.
 *
 * @param[in] key The key to use when reading the data.
 * @param[out] buf A pointer to the input buffer where the data will be stored.
 * @param[in] len The length of the data to be read.
 * @return Returns ESP_OK on success, otherwise an error code
 */
esp_err_t iot_storage::read(const char *key, void *buf, size_t len)
{
    if (failed_to_open)
        return ESP_ERR_INVALID_STATE;

    return nvs_get_blob(_handle, key, buf, &len);
}

/**
 * Verifies the data stored in non-volatile storage with the input data.
 *
 * @param[in] key The key to use when verifying the data.
 * @param[in] input A pointer to the data to verify.
 * @param[in] len The maximum length of the data to be read.
 * @return Returns ESP_OK on success, otherwise an error code
 */
esp_err_t iot_storage::verify(const char *key, const void *input, size_t len)
{
    if (failed_to_open)
        return ESP_ERR_INVALID_STATE;

    void *buf = iot_allocate_mem(len);

    if (buf == nullptr)
        return ESP_ERR_NO_MEM;

    size_t buf_len = len;

    esp_err_t ret = read(key, buf, buf_len);

    if (ret != ESP_OK || buf_len != len)
    {
        free(buf);
        return ESP_FAIL;
    }

    if (memcmp(input, buf, len) != 0)
    {
        free(buf);
        return ESP_FAIL;
    }

    free(buf);
    return ESP_OK;
}

/**
 * Erases a key or all keys in the non-volatile storage.
 * @param[in] key The key to erase. Default is nullptr which will erase all keys.
 * @return Returns ESP_OK on success, otherwise an error code
 */
esp_err_t iot_storage::erase(const char *key)
{
    if (failed_to_open)
        return ESP_ERR_INVALID_STATE;

    esp_err_t ret = ESP_OK;

    if (key != nullptr)
        ret = nvs_erase_key(_handle, key);
    else
        ret = nvs_erase_all(_handle);

    if (ret != ESP_OK)
    {
        ESP_LOGE(tag, "%s -> Failed to erase key(s) from nvs, reason -> %s", __func__, esp_err_to_name(ret));
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
