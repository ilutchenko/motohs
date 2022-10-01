#pragma once

#include <variant>
#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "button.hpp"
#include "button_actions.hpp"

// typedef std::variant<char, int> MsgData;
using namespace MotoHS;

namespace MotoHS {
    namespace Controllers {
        class ButtonHandler;
    }
    namespace System {
        class SystemTask
        {
        public:
            enum class MsgType : uint8_t
            {
                ButtonEvent = 0
            };
            using MsgData = std::variant<Controllers::ButtonActions>;

            struct Msg {
                MsgType type;
                MsgData data;
            };

            SystemTask(Controllers::ButtonHandler& button);
            void Start();
            void PushMessage(MsgType type, MsgData data);

        private:
            TaskHandle_t task_handle;
            QueueHandle_t msg_queue;
            Controllers::ButtonHandler& button_handler;

            static void Process(void* instance);
            void Run();
            void ProcessButtonEvent(MsgData data);
        };
    } // namespace System
} // namespace MotoHS