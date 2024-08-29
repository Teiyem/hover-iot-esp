#include "iot_storage.h"

/**
 * Initialises a new instance of the IotStorage class.
 *
 * @param[in] partition A pointer to the name of partition to use.
 * @param[in] name_space A pointer of the namespace to yse.
 */
IotStorage::IotStorage(const char *partition, const char *name_space)
{
    ESP_LOGI(TAG, "%s: Opening [partition: %s, namespace: %s]", __func__, partition, name_space);

    if (!iot_valid_str(name_space) || !iot_valid_str(partition)) {
        failed_to_open = true;
        return;
    }

    esp_err_t ret = nvs_flash_init_partition(partition);

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    ret = nvs_open_from_partition(partition, name_space, NVS_READWRITE, &_handle);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to open [partition: %s ,namespace: %s, reason: %s]", __func__, partition, name_space,
                 esp_err_to_name(ret));
        failed_to_open = true;
        return;
    }

    if (CONFIG_IOT_HOVER_ENV_DEV)
        print_stats(partition, name_space);
}

/**
 * Destroys the IotStorage class.
 */
IotStorage::~IotStorage(void)
{
    ESP_LOGI(TAG, "%s: ", __func__);
    if (failed_to_open)
        return;

    nvs_close(_handle);
}

/**
 * Writes a data to the non-volatile storage.
 *
 * @param[in] A pointer to the write parameters.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotStorage::write(const iot_nvs_write_params_t *params)
{
    if (failed_to_open)
        return ESP_ERR_INVALID_STATE;

    esp_err_t ret = nvs_set_blob(_handle, params->key, params->data, params->len);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to set blob to nvs, [reason: %s]", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_commit(_handle);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to commit to nvs, [reason: %s]", __func__, esp_err_to_name(ret));
        return ret;
    }

    return ret;
}

/**
 * Reads data from the non-volatile storage.
 *
 * @param[in] key The key to use when reading the data.
 * @param[out] buf A pointer to a buffer to store the data.
 * @param[in] len The size of the buffer.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotStorage::read(const char *key, void *buf, size_t len)
{
    if (failed_to_open)
        return ESP_ERR_INVALID_STATE;

    if (len == 0) {
        ESP_LOGE(TAG, "%s: Cannot get blob, buf len is zero", __func__);
        return ESP_ERR_INVALID_ARG;
    }

    return nvs_get_blob(_handle, key, buf, &len);
}

/**
 * Reads data from non-volatile storage.
 *
 * @param[in] key The key to use when reading the data.
 * @param[out] buf A pointer to a buffer to store the data.
 * @param[out] len The size of the buffer and the size of the data read.
 * @param[in] type The data type to read.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotStorage::read(const char *key, void **buf, size_t &len, iot_nvs_val_type_e type)
{
    if (failed_to_open)
        return ESP_ERR_INVALID_STATE;

    esp_err_t ret = ESP_OK;

    // Get the size first
    if (type == IOT_TYPE_STR)
        ret = nvs_get_str(_handle, key, nullptr, &len);
    else if (type == IOT_TYPE_BLOB)
        ret = nvs_get_blob(_handle, key, nullptr, &len);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s:  Failed get the data size for [key: %s, reason: %s]", __func__, key, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "%s:  Got string with [size: %d]", __func__, len);

    *buf = iot_allocate_mem<uint8_t>(len);

    if (type == IOT_TYPE_STR)
        ret = nvs_get_str(_handle, key, reinterpret_cast<char *>(*buf), &len);
    else if (type == IOT_TYPE_BLOB)
        ret = nvs_get_blob(_handle, key, reinterpret_cast<char *>(*buf), &len);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s:  Failed get the data for [key: %s, reason: %s]", __func__, key, esp_err_to_name(ret));
        free(*buf);
        *buf = nullptr;
    }

    return ret;
}

/**
 * Erases a key or all keys in the non-volatile storage.
 *
 * @param[in] key The key to erase. Default is nullptr which will erase all keys.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotStorage::erase(const char *key)
{
    if (failed_to_open)
        return ESP_ERR_INVALID_STATE;

    esp_err_t ret = ESP_OK;

    if (key != nullptr)
        ret = nvs_erase_key(_handle, key);
    else
        ret = nvs_erase_all(_handle);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to erase key(s) from nvs, reason: %s]", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_commit(_handle);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to commit to nvs, [reason: %s]", __func__, esp_err_to_name(ret));
        return ret;
    }

    return ret;
}

/**
 * Prints the non-volatile storage stats.
 *
 * @param[in] partition  The partition name to print the stats for.
 * @param[in] name_space The namespace to print stats for.
 */
void IotStorage::print_stats(const char *partition, const char *name_space)
{
    nvs_stats_t nvs_stats;
    esp_err_t ret = nvs_get_stats(partition, &nvs_stats);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "%s: Partition [name: %s]", __func__, partition);
        ESP_LOGI(TAG, "%s: Used [entries: %d]", __func__, nvs_stats.used_entries);
        ESP_LOGI(TAG, "%s: Free [entries: %d]", __func__, nvs_stats.free_entries);
        ESP_LOGI(TAG, "%s: Total [entries: %d]", __func__, nvs_stats.total_entries);
        ESP_LOGI(TAG, "%s: Namespace [count: %d]", __func__, nvs_stats.namespace_count);
    } else {
        ESP_LOGE(TAG, "%s: Failed to get nvs stats for [partition: %s, reason: %s]", __func__, partition,
                 esp_err_to_name(ret));

    }

    ESP_LOGI(TAG, "%s: Listing all the key-value pairs for [partition: %s, namespace: %s]", __func__, partition,
             name_space);
    nvs_iterator_t it = nullptr;
    esp_err_t res = nvs_entry_find(partition, name_space, NVS_TYPE_ANY, &it);

    while (res == ESP_OK) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        ESP_LOGI(TAG, "%s: [key: %s, type: %d]", __func__, info.key, info.type);
        res = nvs_entry_next(&it);
    }

    nvs_release_iterator(it);
}