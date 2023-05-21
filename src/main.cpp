#include <stdio.h>
#include <esp_timer.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <soc/soc.h>
#include <soc/rtc_cntl_reg.h>

#include "storage/storage_manager.hpp"
#include "network/wifi_manager.hpp"
#include "network/ping_client.hpp"
#include "network/slimevr_client.hpp"

StorageManager storageManager;
WifiManager wifiManager;
SlimeVRClient slimeClient;

void telemetry(void *arg);
void run(void *arg);

extern "C" void app_main()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    xTaskCreate(run, "Program", 4096, NULL, tskIDLE_PRIORITY, NULL);
}

void run(void *arg)
{
    storageManager.init();
    wifiManager.init();
    wifiManager.connect("Ellisium", "18315019");
    xTaskCreate(telemetry, "Telemetry", 4096, NULL, tskIDLE_PRIORITY, NULL);
    vTaskDelete(NULL);
}

ulong millis()
{
    return (ulong)(esp_timer_get_time() / 1000ULL);
}

ulong current = 0;
void telemetry(void *arg)
{
    current = millis();
    for (;;)
    {
        if (millis() - current >= 5000)
        {
            current = millis();
            ESP_LOGI("Telemetry", "WIFI state: %s", wifiManager.getStateName());
            if (wifiManager.state != WifiState::CONNECTED)
            {
                return;
            }
            if (!slimeClient.isRunning())
            {
                slimeClient.start();
            }
        }
    }
}
