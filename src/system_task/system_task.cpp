#include "system_task.hpp"

using namespace MotoHS::System;

SystemTask::SystemTask()
{
}

void SystemTask::start()
{
    m_msg_queue = xQueueCreate(10, 1);
    if (pdPASS !=
        xTaskCreate(SystemTask::process, "SYS_TASK", 4096, this, 1, &m_task_handle)) {
    }
}

void SystemTask::process(void* instance)
{
    auto* app = static_cast<SystemTask*>(instance);
    app->run();
}

void SystemTask::run()
{
    while (true) {
        uint8_t msg;
        if (xQueueReceive(m_msg_queue, &msg, 100)) {
        }
    }
}
