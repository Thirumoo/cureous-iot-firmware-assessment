#ifndef PROVISIONING_H
#define PROVISIONING_H

#include "esp_err.h"

esp_err_t provisioning_init(void);

esp_err_t provisioning_get_credentials(
    char *ssid,
    size_t ssid_size,
    char *password,
    size_t password_size
);

esp_err_t provisioning_save_credentials(
    const char *ssid,
    const char *password
);

esp_err_t provisioning_clear_credentials(void);

#endif