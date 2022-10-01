#include <string>
#include <system_task.hpp>
#include "button.hpp"
#include <driver/timer.h>

using namespace MotoHS::Controllers;

#define TIMER_DIVIDER (16)
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER)

static std::string print("TIMER!");

void ButtonHandler::ButtonPinInterrupt(void* arg)
{
    auto* button_handler = static_cast<ButtonHandler*>(arg);
    Events event = (gpio_get_level(button_handler->button_gpio_num()) == false)
                       ? Events::Press
                       : Events::Release;
    button_handler->HandleInterrupt(event);
}

bool ButtonHandler::ButtonTimerCallback(void* args)
{
    auto* button_handler = static_cast<ButtonHandler*>(args);
    button_handler->HandleInterrupt(ButtonHandler::Events::Timer);
    return false; // return whether we need to yield at the end of ISR
}

bool ButtonHandler::ButtonEmergencyTimerCallback(void* args)
{
    auto* button_handler = static_cast<ButtonHandler*>(args);
    button_handler->HandleInterrupt(ButtonHandler::Events::EmergencyTimer);
    return false; // return whether we need to yield at the end of ISR
}

ButtonHandler::ButtonHandler(gpio_num_t gpio) : button_gpio(gpio)
{
}

void ButtonHandler::Init(MotoHS::System::SystemTask* systemTask)
{
    system_task = systemTask;
    gpio_config_t io_conf{};
    io_conf.pin_bit_mask = static_cast<uint64_t>(1) << static_cast<uint32_t>(button_gpio);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    gpio_config(&io_conf);
    gpio_set_intr_type(button_gpio, GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(button_gpio, &ButtonHandler::ButtonPinInterrupt, this);
    InitHwTimers();
}

void ButtonHandler::HandleInterrupt(Events event)
{
    if (event == Events::Press) {
        StartTimer();
        StartEmergencyTimer();
    } else if (event == Events::Release) {
        StopTimer();
        StopEmergencyTimer();
        double value{};
        timer_get_counter_time_sec(TIMER_GROUP_0, TIMER_0, &value);
        if (value < 2) {
            system_task->PushMessage(MotoHS::System::SystemTask::MsgType::ButtonEvent,
                                     ButtonActions::ShortClick);
        } else if (value > 2) {
            system_task->PushMessage(MotoHS::System::SystemTask::MsgType::ButtonEvent,
                                     ButtonActions::LongClick);
        }
    } else if (event == Events::Timer) {
        system_task->PushMessage(MotoHS::System::SystemTask::MsgType::ButtonEvent,
                                 ButtonActions::LongPress);
    }
}

void ButtonHandler::InitHwTimers()
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config{};
    config.divider = TIMER_DIVIDER;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_EN;
    config.auto_reload = TIMER_AUTORELOAD_DIS; // default clock source is APB
    timer_init(TIMER_GROUP_0, TIMER_0, &config);
    timer_init(TIMER_GROUP_0, TIMER_1, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 2 * TIMER_SCALE);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_1, 10 * TIMER_SCALE);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_enable_intr(TIMER_GROUP_0, TIMER_1);

    timer_isr_callback_add(TIMER_GROUP_0, TIMER_0, &ButtonHandler::ButtonTimerCallback,
                           this, 0);
    timer_isr_callback_add(TIMER_GROUP_0, TIMER_1,
                           &ButtonHandler::ButtonEmergencyTimerCallback, this, 0);
}

void ButtonHandler::StartTimer()
{
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_set_alarm(TIMER_GROUP_0, TIMER_0, TIMER_ALARM_EN);
    timer_start(TIMER_GROUP_0, TIMER_0);
}

void ButtonHandler::StartEmergencyTimer()
{
    timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);
    timer_set_alarm(TIMER_GROUP_0, TIMER_1, TIMER_ALARM_EN);
    timer_start(TIMER_GROUP_0, TIMER_1);
}

void ButtonHandler::StopTimer()
{
    timer_pause(TIMER_GROUP_0, TIMER_0);
}

void ButtonHandler::StopEmergencyTimer()
{
    timer_pause(TIMER_GROUP_0, TIMER_1);
}
