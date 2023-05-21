#include "wifi_manager.hpp"

#include <esp_event.h>
#include <arpa/inet.h>
#include <string.h>
#include <esp_log.h>

static const char *TAG = "WifiManager";

WifiManager::WifiManager()
{
    this->state = WifiState::UNKNOWN;
    this->ip = nullptr;
    this->ssid = nullptr;
    this->maxConnectionRetries = 5;
    this->currentRetry = 0;
}

void WifiManager::init()
{
    this->state = WifiState::UNKNOWN;

    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        this,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        this,
                                                        &instance_got_ip));
    this->state = WifiState::INITIALIZED;
}

void WifiManager::event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    WifiManager *wifiManager = (WifiManager *)arg;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Connecting to AP...");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (wifiManager != nullptr)
        {
            wifiManager->state = WifiState::DISCONNECTED;
            if (wifiManager->currentRetry < wifiManager->maxConnectionRetries)
            {
                esp_wifi_connect();
                wifiManager->currentRetry++;
                ESP_LOGI(TAG, "Retrying to connect...");
                return;
            }
            else
            {
                ESP_LOGI(TAG, "Failed to connect to AP: %s", wifiManager->ssid);
            }
        }
        ESP_LOGI(TAG, "Disconnected");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        if (wifiManager != nullptr)
        {
            wifiManager->state = WifiState::CONNECTED;
            wifiManager->ip = inet_ntoa(((ip_event_got_ip_t *)event_data)->ip_info.ip);
            ESP_LOGI(TAG, "Connected to AP: %s", wifiManager->ssid);
            ESP_LOGI(TAG, "IP address: %s", wifiManager->ip);
        }
    }
}

WifiState WifiManager::startAccessPoint(const char *ssid, const char *password)
{
    return this->state;
}

WifiState WifiManager::connect(const char *ssid, const char *password)
{
    wifi_config_t wifiConfig;

    strncpy(reinterpret_cast<char*>(wifiConfig.sta.ssid), ssid, sizeof(wifiConfig.sta.ssid));
    strncpy(reinterpret_cast<char*>(wifiConfig.sta.password), password, sizeof(wifiConfig.sta.password));

    wifiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_LOGI(TAG, "Requesting connection to AP: %s", (char *)ssid);

    this->currentRetry = 0;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig));
    ESP_ERROR_CHECK(esp_wifi_start());
    this->ssid = (char *)ssid;
    this->state = WifiState::CONNECTING;
    return this->state;
}

WifiState WifiManager::disconnect()
{
    ESP_LOGI(TAG, "Requesting disconnection");
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    this->state = WifiState::DISCONNECTING;
    return this->state;
}

const char *WifiManager::getStateName()
{
    switch (this->state)
    {
    case WifiState::ACCESSPOINT:
        return "ACCESSPOINT";
    case WifiState::CONNECTED:
        return "CONNECTED";
    case WifiState::DISCONNECTED:
        return "DISCONNECTED";
    case WifiState::CONNECTING:
        return "CONNECTING";
    case WifiState::DISCONNECTING:
        return "DISCONNECTING";
    case WifiState::INITIALIZED:
        return "INITIALIZED";
    default:
        return "UNKNOWN";
    }
}