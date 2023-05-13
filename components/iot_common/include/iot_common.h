#pragma once

#include <stdlib.h>
#include <string.h>
#include <chrono>
#include <time.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "iot_defs.h"

static constexpr const char *TAG = "iot_common"; /* A constant used to identify the source of the log message of this. */

void *iot_allocate_mem(size_t size);
void iot_zero_mem(void *ptr, size_t size);
char *iot_cat_with_delimiter(const char *str1, const char *str2, const char *delimiter);
esp_err_t iot_split_with_delimiter(const char *str, const char *delimiter, char **str1, char **str2);
esp_err_t iot_verify_string(const char *str);
std::chrono::_V2::system_clock::time_point iot_now(void);
const char *iot_now_str(void);
uint64_t iot_convert_time_to_ms(const char *time);
unsigned long IRAM_ATTR iot_millis();