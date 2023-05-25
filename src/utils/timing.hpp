#pragma once

#include <esp_timer.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

inline unsigned long IRAM_ATTR micros()
{
    return (unsigned long)(esp_timer_get_time());
}

inline void IRAM_ATTR delayMicroseconds(uint32_t us)
{
    uint32_t m = micros();
    if (us)
    {
        uint32_t e = (m + us);
        if (m > e)
        { //overflow
            while (micros() > e)
            {
                __asm__ volatile ("nop");
            }
        }
        while (micros() < e)
        {
            __asm__ volatile ("nop");
        }
    }
}

inline void IRAM_ATTR delay(int ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

inline int64_t IRAM_ATTR millis()
{
    return (int64_t)(esp_timer_get_time() / 1000ULL);
}