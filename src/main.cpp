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
#include "utils/timing.hpp"

StorageManager storageManager;
WifiManager wifiManager;
SlimeVRClient slimeClient;

void telemetry(void *arg);
void run(void *arg);

extern "C" void app_main()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    xTaskCreatePinnedToCore(run, "Program", 4096, NULL, tskIDLE_PRIORITY, NULL, tskNO_AFFINITY);
    // xTaskCreatePinnedToCore(telemetry, "Telemetry", 4096, NULL, tskIDLE_PRIORITY, NULL, tskNO_AFFINITY + 1);
}

ulong current = 0;
ulong updateCurrent = 0;
bool infoSent = false;
int tps = 0;
void run(void *arg)
{
    storageManager.init();
    wifiManager.init();
    wifiManager.connect("Ellisium", "18315019");

    current = millis();
    for (;;)
    {
        ulong time = millis();
        if (time - current >= 5000)
        {
            current = time;
            ESP_LOGI("Telemetry", "WIFI state: %s", wifiManager.getStateName());
            if (wifiManager.state != WifiState::CONNECTED)
            {
                return;
            }
            if (!slimeClient.isRunning())
            {
                slimeClient.start();
            }
            ESP_LOGI("Telemetry", "WIFI state: %d", (tps / 5));
            tps = 0;
        }
        if (time - updateCurrent >= 7)
        {
            updateCurrent = time;
            if (!infoSent && slimeClient.isConnected())
            {
                slimeClient.sendSensorInfo(1);
                slimeClient.sendSensorInfo(2);
                slimeClient.sendSensorInfo(3);
                slimeClient.sendSensorInfo(4);
                slimeClient.sendSensorInfo(5);
                slimeClient.sendSensorInfo(6);
                infoSent = true;
            }
            if (infoSent && slimeClient.isConnected())
            {
                slimeClient.sendAcceleration(1);
                slimeClient.sendAcceleration(2);
                slimeClient.sendAcceleration(3);
                slimeClient.sendAcceleration(4);
                slimeClient.sendAcceleration(5);
                slimeClient.sendAcceleration(6);
                tps++;
            }
        }
    }
    vTaskDelete(NULL);
}

void telemetry(void *arg)
{
    vTaskDelete(NULL);
}
