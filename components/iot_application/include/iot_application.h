#pragma once

#include "esp_sntp.h"
#include "esp_app_desc.h"
#include "iot_common.h"
#include "iot_factory.h"
#include "iot_device.h"
#include "iot_application_defs.h"
#include "iot_wifi.h"
#include "iot_provision.h"
#include "iot_server.h"

#include "iot_status_led.h"

#ifdef CONFIG_IOT_HOVER_MQTT_ENABLED
#include "iot_mqtt.h"
#endif

#include "iot_ota.h"
#include "iot_component.h"

/**
 * A class that handles the core functionality of the application.
 */
class IotApplication final
{
public:
    IotApplication(void);
    ~IotApplication(void);
    IotApplication(const IotApplication&) = delete;
    IotApplication(IotApplication&&) = delete;
    IotApplication& operator=(const IotApplication&) = delete;
    IotApplication& operator=(IotApplication&&) = delete;

    void start(iot_app_cfg_t config);

private:
    static constexpr const char *TAG = "IotApplication";         /**< A constant used to identify the source of the log message of this class. */
    static constexpr const uint32_t _clock_sync_time = 500000;   /**< The wait time in milliseconds before attempting to sync the clock. */

    const esp_app_desc_t* _app_desc;
    IotWifi *_iot_wifi;
    IotOta *_iot_ota;
    IotStatusLed *_iot_status_led;
    #ifdef CONFIG_IOT_HOVER_MQTT_ENABLED
    IotMqtt *_iot_mqtt;
    #endif
    static IotProvision *_iot_provision;
    static IotDevice *_iot_device;
    static iot_app_state_e _app_state;
    static QueueHandle_t _queue_handle;
    static std::mutex _reboot_mutex;
    static uint64_t _restart_delay;
    static bool _first_connection;
    static char *_timezone;

    void stop();
    void on_connected();
    iot_device_data_t _device_data{};

    void set_default_log_levels(void);
    void init_sntp(char *timezone);
    void _init(iot_app_cfg_t config);

    static void reboot(uint64_t delay);
    static void on_event([[maybe_unused]] [[maybe_unused]] void *args, [[maybe_unused]] esp_event_base_t base, int32_t id, void *data);
    static void on_sntp_update(timeval *tv);
    static BaseType_t send_to_queue(iot_event_queue_t msg);
    [[noreturn]] static void task(void *param);
    void process_event(iot_event_queue_t event);

    std::vector<IotComponent *> _components{};
};

/** Global instance to the app do not create another.*/
extern IotApplication IotApp;