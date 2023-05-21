#include "storage_manager.hpp"

#include <nvs_flash.h>

StorageManager::StorageManager() {
    this->initialized = false;
}

void StorageManager::init()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    this->initialized = true;
}