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
