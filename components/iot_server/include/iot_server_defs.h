#pragma once

#define IOT_HTTP_SERIALIZATION_ERR "Failed to serialize response"       /**< Error message for when a serialization error occurred. */
#define IOT_HTTP_DESERIALIZATION_ERR "Failed to deserialize request"    /**< Error message for when a deserialization error occurred. */

#define IOT_HTTP_DEFAULT_ERR_MSG "The request could not be processed"         /**< The default error message.*/

/**
 * An enum of common http status codes
 */
typedef enum iot_http_status {
    IOT_HTTP_STATUS_200_OK = 200,                   /**< Indicates the request was successful. */
    IOT_HTTP_STATUS_201_CREATED = 201,              /**< Indicates the request was successful and a new resource was created. */
    IOT_HTTP_STATUS_400_BAD_REQUEST = 400,          /**< Indicates the request could not be understood or was missing required parameters. */
    IOT_HTTP_STATUS_401_UNAUTHORIZED = 401,         /**< Indicates the authentication failed or user does not have permissions for the desired action. */
    IOT_HTTP_STATUS_403_FORBIDDEN = 403,            /**< Indicates the Authentication succeeded but authenticated user does not have access to the resource. */
    IOT_HTTP_STATUS_404_NOT_FOUND = 404,            /**< Indicates the requested resource could not be found. */
    IOT_HTTP_STATUS_405_METHOD_NOT_ALLOWED = 405,   /**< Indicates the HTTP method used is not allowed for the requested resource. */
    IOT_HTTP_STATUS_500_INT_SERVER_ERROR = 500,     /**< Indicates that An error occurred on the server side. */
} iot_http_status_e;