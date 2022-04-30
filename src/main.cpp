#include "esp_log.h"

extern "C" {
void app_main();
}

constexpr uint32_t main_task_delay_ms = 10;

void app_main(void)
{
    ESP_LOGI("Main", "Motoheadset started");
    /* Sheduler started automatically on boot,
        so we don't need to call vTaskStartScheduler()*/
}
