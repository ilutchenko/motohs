#include "logger.h"
#include "console.h"
#include "system_task/system_task.hpp"

MotoHS::System::SystemTask system_task;

extern "C" {
void app_main();
}

void app_main(void)
{
    Logger::init();
    cli_start();
    system_task.start();
    LOGI_TAGGED("Main", "Motoheadset started");
    /* Sheduler started automatically on boot,
        so we don't need to call vTaskStartScheduler()*/
}
