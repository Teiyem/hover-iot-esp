#pragma once

/**
 * A struct of parameters for writing to nvs.
 */
typedef struct iot_nvs_write_params {
    const char *key;           /**< The nvs key to use. */
    const void *data;          /**< A pointer to data to write. */
    size_t len;                /**< The length of the data. */
} iot_nvs_write_params_t;

/**
 * An enum of supported nvs value types
 */
typedef enum iot_nvs_val_type {
    TYPE_STR = 0,      /**< A string nvs value type. */
    TYPE_BLOB = 1      /**< A binary blob nvs value type. */
} iot_nvs_val_type_e;
