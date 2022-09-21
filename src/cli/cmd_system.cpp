#include <stdio.h>
#include <cstring>
#include <ctype.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>

#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "console.h"
#include "sdkconfig.h"
#include "filter_rnnoise.h"

#ifdef CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
#define WITH_TASKS_INFO 1
#endif

#define TAG "CMD SYS"

static void register_free(void);
static void register_restart(void);
static void register_filter(void);

#if WITH_TASKS_INFO
static void register_tasks(void);
#endif

void register_system(void)
{
    register_free();
    register_restart();
    register_filter();
#if WITH_TASKS_INFO
    register_tasks();
#endif
}

/* 'restart' command restarts the program */
static int restart(int argc, char **argv)
{
    ESP_LOGI(TAG, "Restarting");
    esp_restart();
}

static void register_restart(void)
{
    const esp_console_cmd_t cmd = {.command = "restart",
                                   .help = "Software reset of the chip",
                                   .hint = NULL,
                                   .func = &restart,
                                   .argtable = NULL};
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/* 'free' command prints available heap memory */
static int free_mem(int argc, char **argv)
{
    printf("Total free heap: %d\n", esp_get_free_heap_size());
    printf("Total free internal heap: %d\n",
           heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    printf("Total free external heap: %d\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    printf("Largest availible block in internal heap: %d\n",
           heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    printf("Largest availible block in external heap: %d\n",
           heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
    printf("Minimal free internal heap: %d\n",
           heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL));
    printf("Minimal free external heap: %d\n",
           heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM));
    return 0;
}

static void register_free(void)
{
    const esp_console_cmd_t cmd = {.command = "free",
                                   .help = "Get the current size of free heap memory",
                                   .hint = NULL,
                                   .func = &free_mem,
                                   .argtable = NULL};
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/* 'tasks' command prints the list of tasks and related information */
#if WITH_TASKS_INFO

typedef struct {
    std::string name;
    std::string status;
    std::string prio;
    std::string hwm;
    std::string number;
    std::string affinity;
    std::string cpu_ticks;
    std::string cpu_percents;
} task_info_t;

static void append_stats(std::string &mem_str, std::vector<std::string> cpu_stat)
{
    // delete "\n" character
    mem_str.pop_back();

    std::string task_name = mem_str.substr(0, mem_str.find("\t"));

    for (auto const &token : cpu_stat) {
        if (std::string::npos != token.find(task_name)) {
            mem_str += "\t\t" + token.substr(task_name.size() + 1);
            break;
        }
    }
}

static task_info_t splitLine(const std::string &str)
{
    std::vector<std::string> tokens;
    task_info_t info{};

    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, '\t')) {
        tokens.push_back(token);
    }

    if (tokens.size() == 10) {
        info.name = tokens[0];
        info.status = tokens[1];
        info.prio = tokens[2];
        info.hwm = tokens[3];
        info.number = tokens[4];
        info.affinity = tokens[5];
        info.cpu_ticks = tokens[7];
        info.cpu_percents = tokens[9];
    }

    return info;
}

static std::vector<std::string> splitString(const std::string &str)
{
    std::vector<std::string> tokens;

    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, '\n')) {
        tokens.push_back(token);
    }

    return tokens;
}

static void print_task_info_line(task_info_t info)
{
    const uint8_t name_width = 16;
    const uint8_t width = 10;
    const uint8_t cpu_ticks_width = 13;

    std::cout << std::setw(name_width) << info.name;
    std::cout << std::setw(width) << info.status;
    std::cout << std::setw(width) << info.prio;
    std::cout << std::setw(width) << info.hwm;
    std::cout << std::setw(width) << info.number;
    std::cout << std::setw(width) << info.affinity;
    std::cout << std::setw(cpu_ticks_width) << info.cpu_ticks;
    std::cout << std::setw(width) << info.cpu_percents << std::endl;
}

static int tasks_info(int argc, char **argv)
{
    const size_t bytes_per_task = 40; /* see vTaskList description */
    size_t task_list_buffer_size = uxTaskGetNumberOfTasks() * bytes_per_task;
    char *task_list_buffer = (char *)malloc(task_list_buffer_size);
    char *task_cpu_buffer = (char *)malloc(task_list_buffer_size);

    vTaskList(task_list_buffer);
    vTaskGetRunTimeStats(task_cpu_buffer);

    std::vector<std::string> mem_stat = splitString(task_list_buffer);
    std::vector<std::string> cpu_stat = splitString(task_cpu_buffer);

    // map with tasks info sorted by task number
    std::map<uint8_t, task_info_t> info_stat;
    for (auto &token : mem_stat) {
        append_stats(token, cpu_stat);
        task_info_t tski = splitLine(token);
        info_stat.insert(std::make_pair(std::stoi(tski.number), tski));
    }

    fputs("\nTask name\t      Status   Priority    HWM        #      Affinity  "
          "CPU ticks    CPU %\n",
          stdout);

    for (auto &token : info_stat) {
        print_task_info_line(token.second);
    }

    free(task_list_buffer);
    free(task_cpu_buffer);
    return 0;
}

static void register_tasks(void)
{
    const esp_console_cmd_t cmd = {.command = "tasks",
                                   .help = "Get information about running tasks",
                                   .hint = NULL,
                                   .func = &tasks_info,
                                   .argtable = NULL};
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

#endif // WITH_TASKS_INFO

static struct {
    struct arg_int *mode;
    struct arg_end *end;
} filter_args;

/* 'restart' command restarts the program */
static int filter(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, reinterpret_cast<void **>(&filter_args));
    if (nerrors != 0) {
        arg_print_errors(stderr, filter_args.end, argv[0]);
        ESP_LOGE(TAG, "Arguments parsing error");
        return -1;
    }

    int mode = filter_args.mode->ival[0];
    enable_filter((mode == 1));
    return 0;
}

static void register_filter(void)
{
    filter_args.mode = arg_int1(nullptr, nullptr, "<1|0>", "1|0");
    filter_args.end = arg_end(1);
    const esp_console_cmd_t cmd = {.command = "filter",
                                   .help = "Enable/disable noise filter",
                                   .hint = "1|0",
                                   .func = &filter,
                                   .argtable = &filter_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
