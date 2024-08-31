#pragma once

#include <stdlib.h>
#include <string>
#include <string.h>
#include <time.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_attr.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "iot_defs.h"

void iot_zero_mem(void *ptr, size_t size);

/**
 * Allocates memory of the specified size using malloc.
 *
 * @param[in] size The size of the memory to allocate.
 * @return A pointer to the allocated memory block, or nullptr if the allocation fails.
 * @note Ensure to free the allocated memory after usage.
 */
template <typename T>
T *iot_allocate_mem(size_t size)
{
    ESP_LOGI("IotCommon", "%s: Allocating memory for buffer [size: %d]", __func__, size);

    T *ptr = static_cast<T *>(malloc(size));

    if (ptr == nullptr)
        ESP_LOGE("IotCommon", "%s: Failed to allocate memory for buffer [size: %d]", __func__, size);
    else
        iot_zero_mem(ptr, size);

    return ptr;
}

char *iot_cat_with_delimiter(const char *str1, const char *str2, const char *delimiter);
esp_err_t iot_split_with_delimiter(const char *str, const char *delimiter, char **str1, char **str2);
char *iot_mask_str(const char *str);
bool iot_valid_str(const char *str);

template <typename T>
void iot_not_null(T *value)
{
    if (value == nullptr)
        esp_system_abort("Value is null" );
}

/**
 * Frees a single allocated block of memory. Null pointer safe.
 * @tparam T The type of the resource to free.
 * @param resource A pointer to the block of memory to free.
 * @note Null pointer safe.
 */
template<typename T>
void iot_free_one(T* resource) {
    if (resource != nullptr)
        free(resource);
    else
        ESP_LOGW("IotCommon", "%s: Attempted to free a pointer which is null", __func__);
}

/**
 * Frees an array of dynamically allocated block of memory.
 *
 * @tparam T The type of the resource array to delete.
 * @param resource A pointer to the block of memory array to free.
 * @note Null pointer safe.
 */
template<typename... Args>
void iot_free(Args ...args)
{
    (iot_free_one(args), ...);
}

void iot_delete_task_queue(TaskHandle_t &task_handle, QueueHandle_t &queue_handle);

const time_t *iot_now(void);
std::string iot_now_str(void);
uint64_t iot_convert_time_to_ms(const char *time);
unsigned long IRAM_ATTR iot_millis();
void iot_hex_to_bytes(const char* hex_str, char* byte_array, size_t byte_array_size);
char *iot_char_s(const char *literal);