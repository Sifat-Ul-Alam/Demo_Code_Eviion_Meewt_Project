#include "esp_system.h"
#include "nvs_flash.h"
#include "stdio.h"
#include "Flash.h"


void Save_Uint64_To_Flash(uint64_t value, const char* key){
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Open NVS
    nvs_handle_t nvs_handle;
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    ESP_ERROR_CHECK(ret);

    // Write uint64 value to NVS
    ret = nvs_set_u64(nvs_handle, key, value);
    ESP_ERROR_CHECK(ret);

    // Commit changes to NVS
    ret = nvs_commit(nvs_handle);
    ESP_ERROR_CHECK(ret);

    // Close NVS
    nvs_close(nvs_handle);
}

uint64_t Read_Uint64_From_Flash(const char* key){
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Open NVS
    nvs_handle_t nvs_handle;
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    ESP_ERROR_CHECK(ret);

    // Read uint64 value from NVS
    uint64_t value;
    ret = nvs_get_u64(nvs_handle, key, &value);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        // Key not found, return default value
        value = 0;
    } else {
        ESP_ERROR_CHECK(ret);
    }
    // Close NVS
    nvs_close(nvs_handle);
    return value;
}

static void Flash_Nvs_Init(void){
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void Save_Blob_To_Flash(const char* key, const void* data, size_t len){
    Flash_Nvs_Init();
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle));
    ESP_ERROR_CHECK(nvs_set_blob(nvs_handle, key, data, len));
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    nvs_close(nvs_handle);
}

int Read_Blob_From_Flash(const char* key, void* data, size_t len){
    Flash_Nvs_Init();
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle));
    size_t size = len;
    esp_err_t ret = nvs_get_blob(nvs_handle, key, data, &size);
    nvs_close(nvs_handle);
    return (ret == ESP_OK && size == len) ? 1 : 0;
}
