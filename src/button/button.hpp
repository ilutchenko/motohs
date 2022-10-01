#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <driver/gpio.h>
#include "button_actions.hpp"
#include "system_task.hpp"

namespace MotoHS {
    namespace System {
        class SystemTask;
    }
    namespace Controllers {
        class ButtonHandler
        {
        public:
            enum class Events : uint8_t
            {
                Press,
                Release,
                Timer,
                EmergencyTimer
            };
            ButtonHandler(gpio_num_t button_gpio);
            void Init(MotoHS::System::SystemTask* systemTask);
            void HandleInterrupt(Events event);
            static void ButtonPinInterrupt(void* arg);
            void StartTimer();
            void StartEmergencyTimer();
            void StopTimer();
            void StopEmergencyTimer();
            static bool ButtonTimerCallback(void* args);
            static bool ButtonEmergencyTimerCallback(void* args);
            inline gpio_num_t button_gpio_num()
            {
                return button_gpio;
            }

        private:
            TimerHandle_t buttonTimer;
            gpio_num_t button_gpio;
            bool buttonPressed = false;

            MotoHS::System::SystemTask* system_task;

            void InitHwTimers();
        };
    } // namespace Controllers
} // namespace MotoHS
