#include <stdio.h>
#include "iot_security.h"
#include "iot_application.h"
#include "iot_device.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "iot_common.h"
#include <driver/gpio.h>


/* A constant used to identify the source of the log message of this file. */
static constexpr const char *MAIN_TAG = "Main";

/**
 * Callback for handling reads on the iot attributes.
 *
 * @param[in] read_data Pointer to the iot attribute/s that is/are being read.
 * @param[out] data A pointer to the data the has been read.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t iot_attribute_read_cb(iot_attribute_req_param_t *read_data)
{
    ESP_LOGI(MAIN_TAG, "%s: Received write for attribute", __func__);

    for (auto &attribute: read_data->attributes)
    {
        if (attribute.name.empty())
        {
            ESP_LOGI(MAIN_TAG, "%s: Failed read for attribute [name: null]", __func__);
            return ESP_FAIL;
        }

        ESP_LOGI(MAIN_TAG, "%s: Received read for attribute [name: %s ]", __func__, attribute.name.c_str());

        if(attribute.name == IOT_ATTR_NAME_POWER)
        {
            attribute.value = iot_val_bool(gpio_get_level(GPIO_NUM_2));
        }
        else if (attribute.name == IOT_ATTR_NAME_BRIGHTNESS)
        {
            attribute.value.i = esp_random() % 20;
        }
    }

    return ESP_OK;
}

/**
 * A Callback fn for handling writes on the device's attributes.
 * @param[in] param A pointer to the attribute data to write.
 * @return  ESP_OK on success, otherwise an error code.
 */
esp_err_t iot_attribute_write_cb(iot_attribute_req_param_t *param)
{
    uint8_t count = param->attributes.size();

    ESP_LOGI(MAIN_TAG, "%s: Received write for attributes [count:  %d ]", __func__, count);

    for (uint8_t i = 0; i < count; i++)
    {
        iot_attribute_req_data_t attribute = param->attributes[i];
        ESP_LOGI(MAIN_TAG, "%s: Received write for attribute [name: %s ]", __func__, attribute.name.c_str());

        if (attribute.name == IOT_ATTR_NAME_POWER)
        {
            if (gpio_get_level(GPIO_NUM_2) == attribute.value.b)
            {
                ESP_LOGI(MAIN_TAG, "%s: attribute is already set [to: %d ]", __func__, attribute.value.b);
            }
            else
            {
                gpio_set_level(GPIO_NUM_2, attribute.value.b);
                ESP_LOGI(MAIN_TAG, "%s: Toggling %s [to: %d ]", __func__, attribute.name.c_str(), attribute.value.b);
            }
        }
        else if (attribute.name == IOT_ATTR_NAME_BRIGHTNESS)
        {
            ESP_LOGI(MAIN_TAG, "%s: Toggling %s [to: %lu ]", __func__, attribute.name.c_str(), attribute.value.i);
        }
        else
        {
            ESP_LOGE(MAIN_TAG, "%s: Received write for a unknown attribute  [name: %s ]", attribute.name.c_str(), __func__);
            return ESP_ERR_INVALID_ARG;
        }
    }

    return ESP_OK;
}

/**
 * A simple test for encrypting and decrypting
 */
void test_encrypt_decrypt(void)
{
    auto _iot_security = IotFactory::create_scoped<IotSecurity>();

    const char *plaintext = "{\"employees\": [{\"firstName\":\"John\",\"lastName\":\"Doe\"},{\"firstName\":\"Anna\",\"lastName\":\"Smith\"},{\"firstName\":\"Peter\",\"lastName\":\"Jones\"}],\"company\":{\"name\":\"ABC Inc\",\"address\":{\"street\":\"123 Main St\",\"city\":\"New York\",\"state\":\"NY\",\"zip\":\"10001\"}},\"projects\":[{\"name\":\"Project A\",\"description\":\"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed aliquet sapien at sem euismod, quis bibendum est vestibulum. Aliquam mollis vel neque eget facilisis. Integer non consequat arcu. Sed vestibulum tempor massa, id dignissim libero laoreet quis. Sed sit amet orci et sapien eleifend pharetra. Curabitur efficitur purus dolor, et pellentesque turpis congue sed. Ut id metus interdum, blandit justo ut, tincidunt enim.\"},{\"name\":\"Project B\",\"description\":\"Pellentesque ut turpis ligula. Nam nec nulla nisi. Morbi posuere, metus vel fermentum auctor, velit velit consequat enim, quis congue metus nisl in sem. Sed in mauris enim. Donec eget commodo ipsum. Sed eget libero fermentum, auctor mauris eu, gravida justo. Sed ac feugiat nisl. Proin pellentesque vestibulum odio, nec tincidunt odio suscipit a. In hac habitasse platea dictumst. Ut pellentesque velit nec tellus lobortis faucibus. Curabitur porttitor posuere dolor quis blandit. Donec convallis ante in sapien pharetra dictum. Vivamus euismod convallis dui et tincidunt. Nulla facilisi. Vivamus euismod eget velit vel egestas. \"},{\"name\":\"Project C\",\"description\":\"Nullam fringilla risus eu orci volutpat luctus. Etiam consectetur libero sapien, at dictum lorem laoreet vel. Donec ac nunc quam. Duis sit amet velit id tellus lobortis bibendum non in neque. In hac habitasse platea dictumst. Sed nec lobortis dolor. Nam eu lobortis nulla. Cras vel eleifend ex. Donec at tellus dolor. Aliquam erat volutpat. Proin ullamcorper enim risus, in bibendum sapien ultricies id. Vestibulum vel ultrices mi, vitae fringilla nisi. Nulla facilisi. Nam efficitur dolor in finibus convallis. \"}]}";

    enc_dec_crypt_params_t encrypt_params = {
        .input = plaintext,
        .len = strlen(plaintext)
    };

    uint64_t encrypt_start = esp_timer_get_time();

    char *encrypted = _iot_security->encrypt(&encrypt_params);

    uint64_t encrypt_end = esp_timer_get_time();

    ESP_LOGI(MAIN_TAG, "%s: Encryption took [microseconds: %llu ]", __func__, encrypt_start - encrypt_end);

    if (encrypted != nullptr)
    {
        ESP_LOGI(MAIN_TAG, "%s: Encryption [plaintext: %s ]", __func__, plaintext);
        ESP_LOGI(MAIN_TAG, "%s: Encryption [result: %s ]", __func__, encrypted);

        enc_dec_crypt_params_t decrypt_params = {
            .input = encrypted,
            .len = strlen(encrypted)
        };

        uint64_t decrypt_start = esp_timer_get_time();

        char *decrypted = _iot_security->decrypt(&decrypt_params);

        uint64_t decrypt_end = esp_timer_get_time();

        ESP_LOGI(MAIN_TAG, "%s: Decryption took [microseconds: %llu ]", __func__, decrypt_start - decrypt_end);

        free(encrypted);

        if (decrypted != nullptr)
        {
            ESP_LOGI(MAIN_TAG, "%s: Decryption [result: %s ]", __func__, decrypted);
            free(decrypted);
        }

    }
}

/**
 * A simple test for the time conversion.
 */
void test_time_conversion()
{
    ESP_LOGI(MAIN_TAG, "%s: [in:%s, out: %llu ms]", __func__ , "1s", iot_convert_time_to_ms("1s"));
    ESP_LOGI(MAIN_TAG, "%s: [in:%s, out: %llu ms]", __func__ , "1m", iot_convert_time_to_ms("1m"));
    ESP_LOGI(MAIN_TAG, "%s: [in:%s, out: %llu ms]", __func__ , "1h", iot_convert_time_to_ms("1h"));
    ESP_LOGI(MAIN_TAG, "%s: [in:%s, out: %llu ms]", __func__ , "1m 30s", iot_convert_time_to_ms("1m 30s"));
}

/**
 * Application entry point.
 */
extern "C" void app_main(void)
{
    ESP_LOGI(MAIN_TAG, "%s: Application starting %s", __func__, iot_now_str());

    iot_device_info_t *device = iot_device_create("Light", IOT_DEVICE_TYPE_LIGHT);

    iot_attribute_t pwr = iot_attribute_create(IOT_ATTR_NAME_POWER, iot_val_bool(false), true);
    iot_attribute_t bri = iot_attribute_create(IOT_ATTR_NAME_BRIGHTNESS, iot_val_int(100));

    iot_attribute_add_param(&bri, IOT_ATTR_PARAM_MIN, iot_val_int(0));
    iot_attribute_add_param(&bri, IOT_ATTR_PARAM_MAX, iot_val_int(100));

    iot_device_add_attribute(device, pwr);
    iot_device_add_attribute(device, bri);

    iot_device_add_service(device, IOT_DEVICE_OTA_SERVICE, true, true);

    iot_device_cfg_t cfg = iot_device_cfg_t{device, IOT_ATTRIBUTE_CB_RW, &iot_attribute_read_cb, &iot_attribute_write_cb,
                                            nullptr};

    iot_app_cfg_t app_cfg = iot_app_cfg_t(IOT_WIFI_APSTA, &cfg, "IOT_LIGHT_543210XV6");

    IotApp.start(app_cfg);

    test_encrypt_decrypt();

    test_time_conversion();

    while (true)
    {
        ESP_LOGI(MAIN_TAG, "%s: running...............", __func__);
        vTaskDelay(50000 / portTICK_PERIOD_MS);
    }
}

