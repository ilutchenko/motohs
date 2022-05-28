
#include <string.h>
#include "esp_log.h"
#include "audio_error.h"
#include "audio_mem.h"
#include "audio_element.h"
#include "filter_rnnoise.h"
#include "audio_type_def.h"
#include "rnnoise.h"

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

static const char *TAG = "RNNOISE_FILTER";

#define RESAMPLING_POINT_NUM (512)

typedef struct rnnoise_filter {
    // rnnoise_info_t *rnnoise_info;
    unsigned char *out_buf;
    unsigned char *in_buf;
    void *rnnoise_hd;
    int in_offset;
    int8_t flag; // the flag must be 0 or 1. 1 means the parameter in `rnnoise_info_t`
                 // changed. 0 means the parameter in `rnnoise_info_t` is original.
    DenoiseState *st;
} rnnoise_filter_t;

static esp_err_t rnnoise_filter_destroy(audio_element_handle_t self)
{
    rnnoise_filter_t *filter = (rnnoise_filter_t *)audio_element_getdata(self);
    if (filter != NULL) {
        rnnoise_destroy(filter->st);
        audio_free(filter);
    }
    return ESP_OK;
}

static esp_err_t rnnoise_filter_open(audio_element_handle_t self)
{
    rnnoise_filter_t *filter = (rnnoise_filter_t *)audio_element_getdata(self);
    filter->st = rnnoise_create(NULL);
    filter->flag = 0;
    ESP_LOGI(TAG, "opened");
    return ESP_OK;
}

static esp_err_t rnnoise_filter_close(audio_element_handle_t self)
{
    rnnoise_filter_t *filter = (rnnoise_filter_t *)audio_element_getdata(self);
    return ESP_OK;
}
float inb[FRAME_SIZE];
float outb[FRAME_SIZE];

static int rnnoise_filter_process(audio_element_handle_t self, char *in_buffer,
                                  int in_len)
{
    rnnoise_filter_t *filter = (rnnoise_filter_t *)audio_element_getdata(self);
    int out_len = -1;
    int read_len = 0;
    int in_bytes_consumed = 0;
    int ret = 0;
    int16_t *tmp = (int16_t *)in_buffer;


    read_len = audio_element_input(self, in_buffer, in_len);
    
    // ESP_LOG_BUFFER_HEX("s", in_buffer, in_len);
    printf("rnnoise\n");
    for (int i = 0; i <= in_len/2; i = i + 2){
        // in_buffer[i] = 0xff;
        tmp[i] = 0;
    }

    out_len = audio_element_output(self, (char *)in_buffer, read_len);
    return out_len;
}

audio_element_handle_t rnnoise_filter_init(rnnoise_filter_cfg_t *config)
{
    if (config == NULL) {
        ESP_LOGE(TAG, "config is NULL. (line %d)", __LINE__);
        return NULL;
    }
    rnnoise_filter_t *filter =
        (rnnoise_filter_t *)audio_calloc(1, sizeof(rnnoise_filter_t));
    AUDIO_MEM_CHECK(TAG, filter, return NULL);
    audio_element_cfg_t cfg;
    cfg.buffer_len = FRAME_SIZE;
    cfg.task_stack = DEFAULT_ELEMENT_STACK_SIZE;
    cfg.task_prio = DEFAULT_ELEMENT_TASK_PRIO;
    cfg.task_core = 1;
    cfg.multi_in_rb_num = 0;
    cfg.multi_out_rb_num = 0;

    cfg.destroy = rnnoise_filter_destroy;
    cfg.process = rnnoise_filter_process;
    cfg.open = rnnoise_filter_open;
    cfg.close = rnnoise_filter_close;
    // cfg.buffer_len = 0;
    cfg.tag = "rnnoise";
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_rb_size = config->out_rb_size;
    cfg.stack_in_ext = config->stack_in_ext;
    audio_element_handle_t el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, {
        audio_free(filter);
        return NULL;
    });
    audio_element_setdata(el, filter);
    audio_element_info_t info = AUDIO_ELEMENT_INFO_DEFAULT();
    audio_element_setinfo(el, &info);
    ESP_LOGI(TAG, "rnnoise_filter_init");
    return el;
}
