#include "system_task.hpp"

using namespace MotoHS::System;

namespace {
    static inline bool in_isr()
    {
        return xPortInIsrContext();
    }
} // namespace

SystemTask::SystemTask(Controllers::ButtonHandler& button) : button_handler(button)
{
}

void SystemTask::Start()
{
    msg_queue = xQueueCreate(10, sizeof(Msg));
    if (pdPASS !=
        xTaskCreate(SystemTask::Process, "SYS_TASK", 4096, this, 1, &task_handle)) {
    }
}

void SystemTask::PushMessage(MsgType type, MsgData data)
{
    Msg message;
    message.type = type;
    message.data = data;
    if (in_isr()) {
        BaseType_t xHigherPriorityTaskWoken;
        xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(msg_queue, &message, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    } else {
        xQueueSend(msg_queue, &message, portMAX_DELAY);
    }
}

void SystemTask::Process(void* instance)
{
    auto* app = static_cast<SystemTask*>(instance);
    app->Run();
}

void SystemTask::Run()
{
    button_handler.Init(this);

    while (true) {
        Msg msg;
        if (xQueueReceive(msg_queue, &msg, 100)) {
            if (msg.type == SystemTask::MsgType::ButtonEvent) {
                ProcessButtonEvent(msg.data);
            }
        }
    }
}

void SystemTask::ProcessButtonEvent(MsgData data)
{
    if (std::get<Controllers::ButtonActions>(data) ==
        Controllers::ButtonActions::ShortClick) {
        printf("Short click\n");
    } else if (std::get<Controllers::ButtonActions>(data) ==
               Controllers::ButtonActions::LongPress) {
        printf("Long press\n");
    } else if (std::get<Controllers::ButtonActions>(data) ==
               Controllers::ButtonActions::LongClick) {
        printf("Long Click\n");
    }
}
