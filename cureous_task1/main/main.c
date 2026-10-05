#include <stdio.h>
#include <time.h>

#include "esp_log.h"
#include "esp_err.h"

#include "nvs_flash.h"

#include "esp_netif_sntp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "wifi_manager.h"
#include "provisioning.h"
#include "mqtt_manager.h"

static const char *TAG = "MAIN";


static void initialize_sntp(void)
{
    ESP_LOGI(
        TAG,
        "Initializing SNTP..."
    );

    esp_sntp_config_t config =
        ESP_NETIF_SNTP_DEFAULT_CONFIG(
            "pool.ntp.org"
        );

    config.start = true;

    esp_netif_sntp_init(
        &config
    );

    ESP_LOGI(
        TAG,
        "SNTP started"
    );
}


static void wait_for_time_sync(void)
{
    time_t now = 0;

    struct tm timeinfo = {0};

    int retry = 0;

    const int retry_count = 10;

    while (timeinfo.tm_year < (2020 - 1900) &&
           retry < retry_count)
    {
        ESP_LOGI(
            TAG,
            "Waiting for time synchronization..."
        );

        vTaskDelay(
            pdMS_TO_TICKS(2000)
        );

        time(&now);

        localtime_r(
            &now,
            &timeinfo
        );

        retry++;
    }

    if (timeinfo.tm_year >= (2020 - 1900))
    {
        ESP_LOGI(
            TAG,
            "Time synchronized"
        );

        char buffer[64];

        strftime(
            buffer,
            sizeof(buffer),
            "%Y-%m-%d %H:%M:%S",
            &timeinfo
        );

        ESP_LOGI(
            TAG,
            "Current time: %s",
            buffer
        );
    }
    else
    {
        ESP_LOGW(
            TAG,
            "SNTP synchronization timeout"
        );
    }
}


void app_main(void)
{
    ESP_LOGI(
        TAG,
        "================================="
    );

    ESP_LOGI(
        TAG,
        "Cureous Labs - Task 1 Option A"
    );

    ESP_LOGI(
        TAG,
        "ESP32-C6 Native IoT Firmware"
    );

    ESP_LOGI(
        TAG,
        "================================="
    );


    /*
     * Initialize NVS.
     */
    esp_err_t ret =
        nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(
            nvs_flash_erase()
        );

        ESP_ERROR_CHECK(
            nvs_flash_init()
        );
    }
    else
    {
        ESP_ERROR_CHECK(ret);
    }


    /*
     * Initialize provisioning.
     */
    ESP_ERROR_CHECK(
        provisioning_init()
    );


    /*
     * Initialize Wi-Fi.
     */
    ESP_ERROR_CHECK(
        wifi_manager_init()
    );


    /*
     * Start Wi-Fi.
     *
     * If credentials do not exist,
     * UART provisioning will be started.
     */
    ESP_ERROR_CHECK(
        wifi_manager_start()
    );


    /*
     * Wait until Wi-Fi gets an IP address.
     */
    ESP_LOGI(
        TAG,
        "Waiting for Wi-Fi connection..."
    );

    EventGroupHandle_t wifi_event_group =
        wifi_manager_get_event_group();

    xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdTRUE,
        portMAX_DELAY
    );


    ESP_LOGI(
        TAG,
        "Wi-Fi connection established"
    );


    /*
     * Start SNTP.
     */
    initialize_sntp();

    wait_for_time_sync();


    /*
     * Start MQTT.
     */
    ESP_ERROR_CHECK(
        mqtt_manager_start()
    );


    ESP_LOGI(
        TAG,
        "System initialization complete"
    );


    /*
     * app_main can return because all
     * continuous work is handled by
     * FreeRTOS tasks/event handlers.
     */
}