#include "provisioning.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "PROVISION";

#define NVS_NAMESPACE "wifi_cfg"
#define NVS_SSID_KEY  "ssid"
#define NVS_PASS_KEY  "password"

static void remove_newline(char *str)
{
    size_t len = strlen(str);

    while (len > 0 &&
           (str[len - 1] == '\n' ||
            str[len - 1] == '\r'))
    {
        str[len - 1] = '\0';
        len--;
    }
}


static void read_line(
    const char *prompt,
    char *buffer,
    size_t buffer_size)
{
    printf("%s", prompt);
    fflush(stdout);

    if (fgets(buffer, buffer_size, stdin) != NULL)
    {
        remove_newline(buffer);
    }
}


esp_err_t provisioning_init(void)
{
    return ESP_OK;
}


esp_err_t provisioning_get_credentials(
    char *ssid,
    size_t ssid_size,
    char *password,
    size_t password_size)
{
    nvs_handle_t handle;

    esp_err_t ret =
        nvs_open(
            NVS_NAMESPACE,
            NVS_READWRITE,
            &handle
        );

    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "NVS open failed: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }

    size_t required_ssid = ssid_size;

    ret = nvs_get_str(
        handle,
        NVS_SSID_KEY,
        ssid,
        &required_ssid
    );

    if (ret == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGI(
            TAG,
            "Wi-Fi credentials not found"
        );

        read_line(
            "Enter Wi-Fi SSID: ",
            ssid,
            ssid_size
        );

        read_line(
            "Enter Wi-Fi password: ",
            password,
            password_size
        );

        if (strlen(ssid) == 0)
        {
            ESP_LOGE(
                TAG,
                "SSID cannot be empty"
            );

            nvs_close(handle);
            return ESP_ERR_INVALID_ARG;
        }

        ret = provisioning_save_credentials(
            ssid,
            password
        );

        nvs_close(handle);

        return ret;
    }

    if (ret != ESP_OK)
    {
        nvs_close(handle);
        return ret;
    }

    size_t required_password = password_size;

    ret = nvs_get_str(
        handle,
        NVS_PASS_KEY,
        password,
        &required_password
    );

    nvs_close(handle);

    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Password not found"
        );

        return ret;
    }

    ESP_LOGI(
        TAG,
        "Wi-Fi credentials loaded from NVS"
    );

    return ESP_OK;
}


esp_err_t provisioning_save_credentials(
    const char *ssid,
    const char *password)
{
    nvs_handle_t handle;

    esp_err_t ret =
        nvs_open(
            NVS_NAMESPACE,
            NVS_READWRITE,
            &handle
        );

    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = nvs_set_str(
        handle,
        NVS_SSID_KEY,
        ssid
    );

    if (ret != ESP_OK)
    {
        nvs_close(handle);
        return ret;
    }

    ret = nvs_set_str(
        handle,
        NVS_PASS_KEY,
        password
    );

    if (ret != ESP_OK)
    {
        nvs_close(handle);
        return ret;
    }

    ret = nvs_commit(handle);

    nvs_close(handle);

    if (ret == ESP_OK)
    {
        ESP_LOGI(
            TAG,
            "Wi-Fi credentials stored in NVS"
        );
    }

    return ret;
}


esp_err_t provisioning_clear_credentials(void)
{
    nvs_handle_t handle;

    esp_err_t ret =
        nvs_open(
            NVS_NAMESPACE,
            NVS_READWRITE,
            &handle
        );

    if (ret != ESP_OK)
    {
        return ret;
    }

    nvs_erase_key(
        handle,
        NVS_SSID_KEY
    );

    nvs_erase_key(
        handle,
        NVS_PASS_KEY
    );

    ret = nvs_commit(handle);

    nvs_close(handle);

    return ret;
}