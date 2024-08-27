#include <esp_http_server.h>
#include "iot_server_defs.h"
#include "iot_common.h"

/**
 * Creates a text response.
 *
 * @param data The response data.
 * @param status The status of the response. Default is 200.
 * @param free_data Whether the message should be freed or not. Default is true.
 * @return
 */
iot_response_t iot_response_create_text(char *data, const char *status, bool free_data)
{
    return {
            .type = IOT_RESPONSE_TYPE_DEFAULT,
            .data = data,
            .free_data = free_data,
            .status = status,
            .len = 0
    };
}

/**
 * Creates a proto response.
 *
 * @param data      The response data.
 * @param len       The length of the response data.
 * @param status The status of the response. Default is 200.
 * @param free_data Whether the message should be freed or not. Default is true.
 * @return
 */
iot_response_t iot_response_create_proto(char *data, size_t len, const char *status, bool free_data)
{
    return {
        .type = IOT_RESPONSE_TYPE_PROTO,
        .data = data,
        .free_data = free_data,
        .status = status,
        .len = len
    };
}

/**
 * Creates a json response.
 *
 * @param data      The response data.
 * @param len       The length of the response data.
 * @param status    The status of the response. Default is 200.
 * @param free_data Whether the message should be freed or not. Default is true.
 * @return
 */
iot_response_t iot_response_create_json(char *data, size_t len, const char *status, bool free_data)
{
    return {
            .type = IOT_RESPONSE_TYPE_JSON,
            .data = data,
            .free_data = free_data,
            .status = status,
            .len = len
    };
}

/**
 * Creates a text error response.
 *
 * @param error The error code.
 * @param message The detailed error message.
 * @return iot_error_response_t The default error response.
 */
iot_error_response_t iot_error_create_text(httpd_err_code_t error, const char *message)
{
    return {
            .type = IOT_RESPONSE_TYPE_DEFAULT,
            .message= iot_char_s(message),
            .free_msg = false,
            .error = error,
    };
}

/**
 * Creates a json error response.
 *
 * @param error The error code.
 * @param message The detailed error message.
 * @param free_data Whether the message should be freed or not. Default is true.
 * @return
 */
iot_error_response_t iot_error_response_create_proto(httpd_err_code_t error, char *message, bool free_data)
{
    return {
            .type = IOT_RESPONSE_TYPE_PROTO,
            .message= message,
            .free_msg = free_data,
            .error = error
    };
}

/**
 * Creates a json error response.
 *
 * @param error The error code.
 * @param message The json error message object.
 * @param free_data Whether the message should be freed or not. Default is true.
 * @return iot_error_response_t The default error response.
 */
iot_error_response_t iot_error_response_create_json(httpd_err_code_t error, char *message, bool free_data)
{
    return {
            .type = IOT_RESPONSE_TYPE_JSON,
            .message= message,
            .free_msg = free_data,
            .error = error
    };
}