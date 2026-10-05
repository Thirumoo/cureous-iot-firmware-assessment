#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define WIFI_CONNECTED_BIT BIT0

esp_err_t wifi_manager_init(void);

esp_err_t wifi_manager_start(void);

bool wifi_manager_is_connected(void);

EventGroupHandle_t wifi_manager_get_event_group(void);

#endif