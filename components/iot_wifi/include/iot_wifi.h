#pragma once

#include <algorithm>
#include <functional>
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/netdb.h"
#include "nvs_flash.h"
#include "iot_wifi_defs.h"
#include "iot_storage.h"
#include "lwip/apps/netbiosns.h"
#include "iot_component.h"
/**
 * A class for handling network related tasks.
 */
class IotWifi final : public IotComponent
{
public:
    IotWifi();
    ~IotWifi();

    IotWifi(const IotWifi&) = delete;
    IotWifi(IotWifi&&) = delete;
    IotWifi& operator=(const IotWifi&) = delete;
    IotWifi& operator=(IotWifi&&) = delete;

    void start() override;
    void stop() override;
    esp_err_t init_mdns(std::string device_name);
    void set_callback(std::function<void(iot_wifi_message_e)> evt_cb);
    [[maybe_unused]] bool connected();
    bool configured();
    [[nodiscard]] char *get_mac();

private:
    static constexpr const char *TAG = "IotWifi";  /**< A constant used to identify the source of the log message of this class. */
    const uint8_t _max_retries = 10;               /**< The maximum number of times to attempt to reconnect to the Wi-Fi before giving up. */
    uint16_t connect_interval = 1000;              /**< The amount of time to wait before attempting to reconnect to the Wi-Fi. */

    static char _mac[13];
    static bool _configuring;
    static bool _connected;
    static bool _reconnect;
    static bool _reconnecting;
    static uint8_t _retries;
    static wifi_init_config_t _wifi_config;
    static std::function<void(iot_wifi_message_e)> _evt_cb;
    static QueueHandle_t _queue_handle;
    static TaskHandle_t _task_handle;

    esp_err_t check_configuration();
    esp_err_t read_mac(void);
    void connect();
    void reconnect();

    IotStorage *_iot_storage{};
    static void init_default_config();
    static void on_event(void *args, esp_event_base_t base, int32_t id, void *data);
    static BaseType_t send_to_queue(iot_wifi_message_e msg);
    [[noreturn]] static void runner(void *param);

    void process_message(iot_base_message_e msg_id);
    void on_state_changed(bool connected);
};
