#include <sstream>
#include "iot_common.h"

static constexpr const char *TAG = "IotCommon"; /**< A constant used to identify the source of the log message of this. */

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
    char *result = iot_allocate_mem<char>(strlen(str1) + strlen(delimiter) + strlen(str2) + 1);

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
    size_t delimiter_len = strlen(delimiter);

    char *pos = strstr(str, delimiter);

    esp_err_t ret = ESP_FAIL;

    if (pos == nullptr)
    {
        ESP_LOGE(TAG, "%s: Failed to locate delimiter", __func__);
        return ret;
    }

    *str1 = iot_allocate_mem<char>(pos - str + 1);

    if (*str1 == nullptr)
    {
        return ret;
    }

    strncpy(*str1, str, pos - str);
    (*str1)[pos - str] = '\0';

    *str2 = iot_allocate_mem<char>(strlen(pos + delimiter_len) + 1);

    if (*str2 == nullptr)
    {
        free(*str1);
        return ret;
    }

    strcpy(*str2, pos + delimiter_len);

    return ESP_OK;
}

/**
 * Masks the entire string with an asterisks ('*').
 * @param[in] str The string to be masked.
 * @return The masked string or null if the string is invalid.
 * @note Ensure to free the masked string if it's not null.
 */
char *iot_mask_str(const char *str)
{
    if(!iot_valid_str(str))
        return nullptr;

    size_t length = strlen(str);
    char *mask = iot_allocate_mem<char>(length + 1);

    memset(mask, '*', length);
    mask[length] = '\0';

    return mask;
}

/**
 * Checks if a given string is valid.
 *
 * @param[in] str The string to check.
 * @return ESP_OK` if the string is valid, otherwise ESP_FAIL.
 */
bool iot_valid_str(const char *str)
{
    if (str == nullptr || str[0] == '\0' || strlen(str) == 0)
    {
        ESP_LOGE(TAG, "%s: String is invalid", __func__);
        return false;
    }

    return true;
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
    uint64_t result = 0;
    std::stringstream ss(time);
    std::string token;

    while (ss >> token) {
        size_t len = token.length();

        if (len > 1) {
            char unit = token[len - 1];
            int value = std::stoi(token.substr(0, len - 1));

            switch (unit) {
                case 'h':
                    result += static_cast<uint64_t>(value) * 60 * 60 * 1000;
                    break;
                case 'm':
                    result += static_cast<uint64_t>(value) * 60 * 1000;
                    break;
                case 's':
                    result += static_cast<uint64_t>(value) * 1000;
                    break;
                default:
                    ESP_LOGE(TAG, "%s: Invalid time [format: %s]", __func__, time);
                    return 0;
            }
        }
    }

    return result;
}

/**
 * Returns the milliseconds since the ESP was booted.
 */
unsigned long iot_millis(void)
{
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

void iot_hex_to_bytes(const char* hex_str, char* byte_array, size_t byte_array_size) {
    for (size_t i = 0; i < byte_array_size; i++) {
        sscanf(&hex_str[i * 2], "%2hhx", &byte_array[i]);
    }
}

/**
 * Casts a string literal(const char *) to char*
 *
 * @param literal A pointer to a constant character string.
 * @return char*  A pointer to the same memory location as literal.
 * @warning Don't modify the value a const is a const.
 */
char *iot_char_s(const char *literal)
{
    return const_cast<char *>(literal);
}