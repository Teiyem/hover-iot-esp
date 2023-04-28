#include <stdio.h>
#include "iot_util.h"

/**
 * Checks if a given string is valid.
 *
 * @param str The string to check.
 * @return `ESP_OK` if the string is valid, `ESP_FAIL` otherwise.
 */
esp_err_t check_string_validity(const char *str)
{
    if (str == nullptr || str[0] == '\0' || strlen(str) == 0)
    {
        ESP_LOGE(tag, "%s -> String is invalid", __func__);
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * Allocates a block of memory of the specified size using malloc.
 *
 * @param size The size of the memory block to allocate.
 * @return A pointer to the allocated memory block, or nullptr if the allocation fails.
 */
void *allocate_mem(size_t size)
{
    ESP_LOGI(tag, "%s -> Allocating memory for buffer with size -> %d", __func__, size);

    void *ptr = malloc(size);

    if (ptr == nullptr)
    {
        ESP_LOGE(tag, "%s -> Failed to allocate memory for buffer with size -> %d", __func__, size);
    }
    else
    {
        memset(ptr, 0, size);
    }

    return ptr;
}

/**
 * Concatenates two strings with a delimiter.
 *
 * @param str1 The first string to concatenate.
 * @param str2 The second string to concatenate.
 * @param delimiter The delimiter to use between the strings.
 * @return The concatenated string, or NULL if an error occurred.
 */
char *concat_with_delimiter(const char *str1, const char *str2, const char *delimiter)
{
    char *result = static_cast<char *>(allocate_mem(strlen(str1) + strlen(delimiter) + strlen(str2) + 1));

    if (result == nullptr)
    {
        return result;
    }

    strcpy(result, str1);
    strcat(result, delimiter);
    strcat(result, str2);

    return result;
}

/**
 * Splits a string into two parts based on a delimiter.
 *
 * @param str The string to split.
 * @param delimiter The delimiter to split the string on.
 * @param str1 The first part of the split string.
 * @param str2 The second part of the split string.
 * @return `ESP_OK` if the string is valid, `ESP_FAIL` otherwise.
 */
esp_err_t split_with_delimiter(const char *str, const char *delimiter, char **str1, char **str2)
{
    int delimiter_len = strlen(delimiter);

    char *pos = strstr(str, delimiter);

    esp_err_t err_ret = ESP_FAIL;

    if (pos == nullptr)
    {
        ESP_LOGE(tag, "%s -> Failed to locate delimiter", __func__);
        return err_ret;
    }

    *str1 = static_cast<char *>(allocate_mem(pos - str + 1));

    if (*str1 == nullptr)
    {
        return err_ret;
    }

    strncpy(*str1, str, pos - str);
    (*str1)[pos - str] = '\0';

    *str2 = static_cast<char *>(allocate_mem(strlen(pos + delimiter_len) + 1));

    if (*str2 == nullptr)
    {
        free(*str1);
        return err_ret;
    }

    strcpy(*str2, pos + delimiter_len);

    return 0;
}
