#pragma once

#define IOT_OTA_MAX_BUFFER_SIZE 1024 /** The maximum buffer/ chunk size to hold the firmware update file. */

/**
 * An enum of the different ota update statuses.
 */
typedef enum iot_ota_state
{
    IOT_OTA_STATE_IDLE = 0,        /**< The update process has started. */
    IOT_OTA_STATE_STARTED,         /**< The update process has started. */
    IOT_OTA_STATE_SUCCESS,         /**< The update process completed successfully. */
    IOT_OTA_STATE_FAILED,          /**< The update process failed. */
    IOT_OTA_STATE_REJECTED,        /**< The update data is invalid and the update request is rejected. */
    IOT_OTA_STATE_DELAYED,         /**< The update process delayed. */
} iot_ota_state_e;
