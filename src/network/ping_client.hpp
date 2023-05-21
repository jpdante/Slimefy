#pragma once

#include <netdb.h>
#include <ping/ping_sock.h>

class PingClient
{
private:
    esp_ping_config_t config;
    esp_ping_callbacks_t callbacks;
    esp_ping_handle_t handle;
    bool running;

public:
    PingClient(int count = ESP_PING_COUNT_INFINITE, int interval = 1000, int taskPriority = 5);

    esp_err_t start(uint32_t interval, uint32_t taskPriority, const char *target_host);
    esp_err_t start(const char *target_host);
    esp_err_t stop();
    bool isRunning();

    void internal_ping_success(esp_ping_handle_t hdl);
    void internal_ping_timeout(esp_ping_handle_t hdl);
    void internal_ping_end(esp_ping_handle_t hdl);

private:
    static void on_ping_success(esp_ping_handle_t hdl, void *args);
    static void on_ping_timeout(esp_ping_handle_t hdl, void *args);
    static void on_ping_end(esp_ping_handle_t hdl, void *args);
};