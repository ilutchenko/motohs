#include "logger.h"

extern "C" {
void app_main();
}

void app_main(void)
{
    Logger::init();
    LOGI_TAGGED("Main", "Motoheadset started");
    /* Sheduler started automatically on boot,
        so we don't need to call vTaskStartScheduler()*/
}
