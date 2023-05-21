#pragma once

#include <esp_wifi.h>
#include <freertos/event_groups.h>

enum WifiState
{
    ACCESSPOINT,
    CONNECTED,
    DISCONNECTED,
    CONNECTING,
    DISCONNECTING,
    INITIALIZED,
    UNKNOWN
};

class WifiManager
{
private:
    EventGroupHandle_t wifi_event_group;

public:
    WifiState state;
    int maxConnectionRetries;
    int currentRetry;
    char *ssid;
    char *ip;

public:
    WifiManager();

    void init();
    WifiState startAccessPoint(const char *ssid, const char *password);
    WifiState connect(const char *ssid, const char *password);
    WifiState disconnect();
    const char *getStateName();

private:
    static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
};