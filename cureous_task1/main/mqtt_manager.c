#include "mqtt_manager.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_timer.h"

#include "mqtt_client.h"

#include "cJSON.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "wifi_manager.h"

static const char *TAG = "MQTT";

/*
 * Public MQTT broker.
 *
 * For an assessment/demo, plain MQTT is simple.
 * Production firmware should use MQTT over TLS.
 */
#define MQTT_BROKER_URI \
    "mqtt://test.mosquitto.org:1883"

static esp_mqtt_client_handle_t mqtt_client;

static bool mqtt_connected = false;

static char mqtt_topic[128];


static void get_device_topic(void)
{
    uint8_t mac[6];

    ESP_ERROR_CHECK(
        esp_read_mac(
            mac,
            ESP_MAC_WIFI_STA
        )
    );

    snprintf(
        mqtt_topic,
        sizeof(mqtt_topic),
        "cureous/esp32c6/%02X%02X%02X%02X%02X%02X/heartbeat",
        mac[0],
        mac[1],
        mac[2],
        mac[3],
        mac[4],
        mac[5]
    );
}


static void mqtt_event_handler(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data)
{
    esp_mqtt_event_handle_t event =
        event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED:

            mqtt_connected = true;

            ESP_LOGI(
                TAG,
                "MQTT connected"
            );

            ESP_LOGI(
                TAG,
                "Heartbeat topic: %s",
                mqtt_topic
            );

            break;


        case MQTT_EVENT_DISCONNECTED:

            mqtt_connected = false;

            ESP_LOGW(
                TAG,
                "MQTT disconnected"
            );

            break;


        case MQTT_EVENT_ERROR:

            ESP_LOGE(
                TAG,
                "MQTT error"
            );

            break;


        default:
            break;
    }
}


static void heartbeat_task(void *arg)
{
    while (1)
    {
        vTaskDelay(
            pdMS_TO_TICKS(5000)
        );

        if (!wifi_manager_is_connected())
        {
            ESP_LOGW(
                TAG,
                "Wi-Fi not connected; heartbeat skipped"
            );

            continue;
        }

        if (!mqtt_connected)
        {
            ESP_LOGW(
                TAG,
                "MQTT not connected; heartbeat skipped"
            );

            continue;
        }

        /*
         * Uptime in seconds.
         */
        int64_t uptime_us =
            esp_timer_get_time();

        uint64_t uptime_sec =
            uptime_us / 1000000ULL;


        /*
         * Get current time.
         */
        time_t now;

        struct tm timeinfo;

        time(&now);

        localtime_r(
            &now,
            &timeinfo
        );

        char timestamp[64];

        strftime(
            timestamp,
            sizeof(timestamp),
            "%Y-%m-%dT%H:%M:%SZ",
            &timeinfo
        );


        /*
         * Get Wi-Fi RSSI.
         */
        wifi_ap_record_t ap_info;

        int rssi = 0;

        if (esp_wifi_sta_get_ap_info(&ap_info)
            == ESP_OK)
        {
            rssi = ap_info.rssi;
        }


        /*
         * Create JSON payload.
         */
        cJSON *root =
            cJSON_CreateObject();

        if (root == NULL)
        {
            ESP_LOGE(
                TAG,
                "Failed to create JSON"
            );

            continue;
        }

        cJSON_AddStringToObject(
            root,
            "device",
            "ESP32-C6"
        );

        cJSON_AddNumberToObject(
            root,
            "uptime_sec",
            (double)uptime_sec
        );

        cJSON_AddStringToObject(
            root,
            "status",
            "online"
        );

        cJSON_AddNumberToObject(
            root,
            "wifi_rssi",
            rssi
        );

        cJSON_AddStringToObject(
            root,
            "timestamp",
            timestamp
        );

        cJSON_AddStringToObject(
            root,
            "firmware",
            "1.0.0"
        );


        char *payload =
            cJSON_PrintUnformatted(root);

        if (payload != NULL)
        {
            int msg_id =
                esp_mqtt_client_publish(
                    mqtt_client,
                    mqtt_topic,
                    payload,
                    0,
                    1,
                    0
                );

            ESP_LOGI(
                TAG,
                "Heartbeat published, msg_id=%d",
                msg_id
            );

            ESP_LOGI(
                TAG,
                "Payload: %s",
                payload
            );

            free(payload);
        }

        cJSON_Delete(root);
    }
}


esp_err_t mqtt_manager_start(void)
{
    get_device_topic();

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri =
            MQTT_BROKER_URI,
    };

    mqtt_client =
        esp_mqtt_client_init(
            &mqtt_cfg
        );

    if (mqtt_client == NULL)
    {
        ESP_LOGE(
            TAG,
            "MQTT client initialization failed"
        );

        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(
        esp_mqtt_client_register_event(
            mqtt_client,
            ESP_EVENT_ANY_ID,
            mqtt_event_handler,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_mqtt_client_start(
            mqtt_client
        )
    );

    xTaskCreate(
        heartbeat_task,
        "heartbeat_task",
        4096,
        NULL,
        5,
        NULL
    );

    return ESP_OK;
}