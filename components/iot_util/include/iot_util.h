#pragma once

#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_err.h"

static constexpr const char *tag = "iot_util";

void *allocate_mem(size_t size);
char *concat_with_delimiter(const char *str1, const char *str2, const char *delimiter);
esp_err_t split_with_delimiter(const char *str, const char *delimiter, char **str1, char **str2);
esp_err_t check_string_validity(const char *str);

/**
 * A struct containing an input string and its length for encryption or decryption.
 *
 * @param input Pointer to the input string.
 * @param len Length of the input string.
 */
typedef struct
{
    const char *input; ///< Pointer to the input string.
    const size_t len;  ///< Length of the input string.
} encryption_params_t;