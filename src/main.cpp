#include <logging/logger.h>
#include <cli/console.h>
#include <system_task/system_task.hpp>
#include <button/button.hpp>

MotoHS::Controllers::ButtonHandler button_handler(GPIO_NUM_0);
MotoHS::System::SystemTask system_task(button_handler);

extern "C" {
void app_main();
}

void app_main(void)
{
    Logger::init();
    cli_start();
    system_task.Start();
    LOGI_TAGGED("Main", "Motoheadset started");
    /* Sheduler started automatically on boot,
        so we don't need to call vTaskStartScheduler()*/
}
