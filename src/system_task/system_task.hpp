#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

namespace MotoHS {
    namespace System {
        class SystemTask
        {
        public:
            SystemTask();
            void start();

        private:
            TaskHandle_t m_task_handle;
            QueueHandle_t m_msg_queue;

            static void process(void* instance);
            void run();
        };
    } // namespace System
} // namespace MotoHS