#pragma once

#include <optional>

#define IOT_HTTPD_TYPE_JSON "application/ json"         /**< A JSON content type header value. */
#define IOT_HTTPD_TYPE_PROTO "application/x-protobuf"   /**< A protobuf content type header value. */

/**
 * An enum for the different types of response.
 */
typedef enum iot_response_type {
    IOT_RESPONSE_TYPE_JSON = 0,   /**< A JSON response type. */
    IOT_RESPONSE_TYPE_DEFAULT,    /**< A plain text response type. */
    IOT_RESPONSE_TYPE_PROTO,      /**< A common protocol buffer response type. */
} iot_response_type_e;

/**
 * A struct for http responses.
 */
typedef struct iot_response {
    iot_response_type_e type;    /**< The response type. */
    char *data;                  /**< The response data or message. */
    bool free_data;              /**< Indicates whether the data should be freed */
    const char *status;          /**< The status of the response. */
    size_t len;                  /**< The data length. */
} iot_response_t;

/**
 * A struct for http responses.
 */
typedef struct iot_server_cfg {
    bool use_https;
}iot_server_cfg_t;

/**
 * A struct for http error responses.
 */
typedef struct iot_error_response {
    iot_response_type_e type;    /**< The response type. */
    char *message;               /**< The response data or message. */
    bool free_msg;               /**< Indicates whether the data should be freed. */
    httpd_err_code_t error;      /**< The error code associated with the HTTP response. */
} iot_error_response_t;

iot_response_t iot_response_create_text(char *data, const char *status = "200", bool free_data = false);
iot_response_t iot_response_create_json(char *data, size_t len, const char *status = "200", bool free_data = true);
iot_response_t iot_response_create_proto(char *data, size_t len, const char *status = "200", bool free_data = true);

iot_error_response_t iot_error_create_text(httpd_err_code_t error, const char *message);
iot_error_response_t iot_error_response_create_proto(httpd_err_code_t error, char *message, bool free_data = true);
iot_error_response_t iot_error_response_create_json(httpd_err_code_t error, char *message, bool free_data = true);
