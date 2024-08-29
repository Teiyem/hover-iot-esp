#pragma once

#define IOT_HTTP_SERIALIZATION_ERR "Failed to serialize response"       /**< Error message for when a serialization error occurred. */
#define IOT_HTTP_DESERIALIZATION_ERR "Failed to deserialize request"    /**< Error message for when a deserialization error occurred. */

/**
 * An enum of typical used http status codes.
 */
typedef enum iot_http_status {
    IOT_HTTP_STATUS_200_OK = 200,
    IOT_HTTP_STATUS_201_CREATED = 201,
    IOT_HTTP_STATUS_400_BAD_REQUEST = 400,
    IOT_HTTP_STATUS_401_UNAUTHORIZED = 401,
    IOT_HTTP_STATUS_403_FORBIDDEN = 403,
    IOT_HTTP_STATUS_404_NOT_FOUND = 404,
    IOT_HTTP_STATUS_405_METHOD_NOT_ALLOWED = 405,
    IOT_HTTP_STATUS_500_INT_SERVER_ERROR = 500,
} iot_http_status_e;
