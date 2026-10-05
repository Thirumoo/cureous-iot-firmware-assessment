#include "wifi_manager.h"

#include <string.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "provisioning.h"

static const char *TAG = "WIFI";

static EventGroupHandle_t wifi_event_group;

static int retry_count = 0;

#define MAX_RETRY_COUNT 10

static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "Wi-Fi STA started");

        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupClearBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );

        ESP_LOGW(TAG, "Wi-Fi disconnected");

        if (retry_count < MAX_RETRY_COUNT)
        {
            retry_count++;

            ESP_LOGI(
                TAG,
                "Reconnecting... attempt %d/%d",
                retry_count,
                MAX_RETRY_COUNT
            );

            esp_wifi_connect();
        }
        else
        {
            ESP_LOGW(
                TAG,
                "Maximum retry count reached"
            );

            /*
             * Keep retrying autonomously.
             * Reset retry counter and continue.
             */
            retry_count = 0;

            esp_wifi_connect();
        }
    }

    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        ESP_LOGI(
            TAG,
            "Got IP: " IPSTR,
            IP2STR(&event->ip_info.ip)
        );

        retry_count = 0;

        xEventGroupSetBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );
    }
}


esp_err_t wifi_manager_init(void)
{
    wifi_event_group =
        xEventGroupCreate();

    if (wifi_event_group == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    ESP_ERROR_CHECK(
        esp_netif_init()
    );

    ESP_ERROR_CHECK(
        esp_event_loop_create_default()
    );

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg =
        WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_wifi_init(&cfg)
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL
        )
    );

    return ESP_OK;
}


esp_err_t wifi_manager_start(void)
{
    char ssid[64] = {0};
    char password[64] = {0};

    esp_err_t ret =
        provisioning_get_credentials(
            ssid,
            sizeof(ssid),
            password,
            sizeof(password)
        );

    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Could not obtain Wi-Fi credentials"
        );

        return ret;
    }

    wifi_config_t wifi_config = {0};

    strncpy(
        (char *)wifi_config.sta.ssid,
        ssid,
        sizeof(wifi_config.sta.ssid) - 1
    );

    strncpy(
        (char *)wifi_config.sta.password,
        password,
        sizeof(wifi_config.sta.password) - 1
    );

    wifi_config.sta.threshold.authmode =
        WIFI_AUTH_WPA2_PSK;

    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_STA)
    );

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        )
    );

    ESP_ERROR_CHECK(
        esp_wifi_start()
    );

    ESP_LOGI(
        TAG,
        "Wi-Fi started"
    );

    return ESP_OK;
}


bool wifi_manager_is_connected(void)
{
    if (wifi_event_group == NULL)
    {
        return false;
    }

    EventBits_t bits =
        xEventGroupGetBits(
            wifi_event_group
        );

    return (
        bits & WIFI_CONNECTED_BIT
    ) != 0;
}


EventGroupHandle_t
wifi_manager_get_event_group(void)
{
    return wifi_event_group;
}