#include "iot_common.h"

/**
 * Allocates a block of memory of the specified size using malloc.
 *
 * @param[in] size The size of the memory block to allocate.
 * @return A pointer to the allocated memory block, or nullptr if the allocation fails.
 * @note Ensure to free the allocated memory after usage.
 */
void *iot_allocate_mem(size_t size)
{
    ESP_LOGI(TAG, "%s -> Allocating memory for buffer with size -> %d", __func__, size);

    void *ptr = malloc(size);

    if (ptr == nullptr)
        ESP_LOGE(TAG, "%s -> Failed to allocate memory for buffer with size -> %d", __func__, size);
    else
        iot_zero_mem(ptr, size);

    return ptr;
}

/**
 * Zeros out a block of memory.
 *
 * @param[in] ptr A pointer to the memory block.
 * @param[in] size The size of the memory block in bytes.
 */
void iot_zero_mem(void *ptr, size_t size)
{
    memset(ptr, 0x00, size);
}

/**
 * Concatenates two strings with a delimiter.
 *
 * @param[in] str1 The first string to concatenate.
 * @param[in] str2 The second string to concatenate.
 * @param[in] delimiter The delimiter to use between the strings.
 * @return The concatenated string, or NULL if an error occurred.
 * @note Ensure to free the concatenated string.
 */
char *iot_cat_with_delimiter(const char *str1, const char *str2, const char *delimiter)
{
    char *result = static_cast<char *>(iot_allocate_mem(strlen(str1) + strlen(delimiter) + strlen(str2) + 1));

    if (result == nullptr)
        return result;

    strcpy(result, str1);
    strcat(result, delimiter);
    strcat(result, str2);

    return result;
}

/**
 * Splits a string into two parts based on a delimiter.
 *
 * @param[in] str The string to split.
 * @param[in] delimiter The delimiter to split the string on.
 * @param[out] str1 The first part of the split string.
 * @param[out] str2 The second part of the split string.
 * @return ESP_OK if the string is valid, otherwise ESP_FAIL.
 * @note Ensure to free str1 and str2 after usage.
 */
esp_err_t iot_split_with_delimiter(const char *str, const char *delimiter, char **str1, char **str2)
{
    int delimiter_len = strlen(delimiter);

    char *pos = strstr(str, delimiter);

    esp_err_t ret = ESP_FAIL;

    if (pos == nullptr)
    {
        ESP_LOGE(TAG, "%s -> Failed to locate delimiter", __func__);
        return ret;
    }

    *str1 = static_cast<char *>(iot_allocate_mem(pos - str + 1));

    if (*str1 == nullptr)
    {
        return ret;
    }

    strncpy(*str1, str, pos - str);
    (*str1)[pos - str] = '\0';

    *str2 = static_cast<char *>(iot_allocate_mem(strlen(pos + delimiter_len) + 1));

    if (*str2 == nullptr)
    {
        free(*str1);
        return ret;
    }

    strcpy(*str2, pos + delimiter_len);

    return ESP_OK;
}

/**
 * Checks if a given string is valid.
 *
 * @param str The string to check.
 * @return ESP_OK` if the string is valid, otherwise ESP_FAIL.
 */
esp_err_t iot_verify_string(const char *str)
{
    if (str == nullptr || str[0] == '\0' || strlen(str) == 0)
    {
        ESP_LOGE(TAG, "%s -> String is invalid", __func__);
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * Gets the current time as a time_point.
 * @return A time_point object.
 */
std::chrono::_V2::system_clock::time_point iot_now(void)
{
    return std::chrono::system_clock::now();
}

/**
 * Gets a string representation of the current time, as determined by the device's internal clock.
 * @return A pointer to the time string.
 */
const char *iot_now_str(void)
{
    const std::time_t now{std::chrono::system_clock::to_time_t(iot_now())};
    return std::asctime(std::localtime(&now));
}

/**
 * Converts a string representation of time in hours, minutes, and seconds to microseconds.
 *
 * @param[in] time A string representing time in the format "h hours m minutes s seconds".
 * @note Only hours, minutes, and seconds can be specified.
 * @return The time in microseconds, or 0 if the input is invalid.
 */
uint64_t iot_convert_time_to_ms(const char *time)
{
    uint64_t ret = 0;

    char *buf;
    char *ptr = strdup(time);
    char *token = strtok_r(ptr, " ", &buf);
    while (token != nullptr)
    {
        int len = strlen(token);

        if (len > 1)
        {
            char unit = token[len - 1];
            int value = atoi(token);
            switch (unit)
            {
            case 'h':
                ret += value * 60 * 60 * 1000000;
                break;
            case 'm':
                ret += value * 60 * 1000000;
                break;
            case 's':
                ret += value * 1000000;
                break;
            default:
                ESP_LOGE(TAG, "%s -> Invalid time format: %s", __func__, time);
                return 0;
            }
        }
        token = strtok_r(nullptr, " ", &buf);
    }
    free(ptr);

    return ret;
}

/**
 * Returns the milliseconds since the ESP was booted.
 */
unsigned long iot_millis(void)
{
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}