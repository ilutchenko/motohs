
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

// static esp_err_t is_valid_rnnoise_filter_samplerate(int samplerate)
// {
//     if (samplerate < 8000 || samplerate > 96000)
//     {
//         ESP_LOGE(TAG, "The sample rate should be within range [8000,96000], here is %d
//         Hz", samplerate); return ESP_FAIL;
//     }
//     return ESP_OK;
// }

// static esp_err_t is_valid_rnnoise_filter_channel(int channel)
// {
//     if (channel != 1 && channel != 2)
//     {
//         ESP_LOGE(TAG, "The number of channels should be either 1 or 2, here is %d",
//         channel); return ESP_FAIL;
//     }
//     return ESP_OK;
// }

// esp_err_t rnnoise_filter_set_src_info(audio_element_handle_t self, int src_rate, int
// src_ch)
// {
//     rnnoise_filter_t *filter = (rnnoise_filter_t *)audio_element_getdata(self);
//     if (filter->rnnoise_info->src_rate == src_rate && filter->rnnoise_info->src_ch ==
//     src_ch)
//     {
//         return ESP_OK;
//     }
//     if (is_valid_rnnoise_filter_samplerate(src_rate) != ESP_OK ||
//     is_valid_rnnoise_filter_channel(src_ch) != ESP_OK)
//     {
//         return ESP_ERR_INVALID_ARG;
//     }
//     else
//     {
//         filter->flag = 1;
//         filter->rnnoise_info->src_rate = src_rate;
//         filter->rnnoise_info->src_ch = src_ch;
//         ESP_LOGI(TAG, "reset sample rate of source data : %d, reset channel of source
//         data : %d",
//                  filter->rnnoise_info->src_rate, filter->rnnoise_info->src_ch);
//     }
//     return ESP_OK;
// }

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
    // rnnoise_info_t *rnnoise_info = filter->rnnoise_info;
    filter->st = rnnoise_create(NULL);
    filter->flag = 0;
    // if (rnnoise_info->sample_bits != 16)
    // {
    //     ESP_LOGE(TAG, "Currently, the only supported bit width is 16 bits.");
    //     return ESP_ERR_INVALID_ARG;
    // }
    // if (rnnoise_info->complexity < 0)
    // {
    //     ESP_LOGI(TAG, "Currently, the complexity is %d, that is less than 0, it has
    //     been set 0.", rnnoise_info->complexity); rnnoise_info->complexity = 0;
    // }
    // else if (rnnoise_info->complexity > COMPLEXITY_MAX_NUM)
    // {
    //     ESP_LOGI(TAG, "Currently, the complexity is %d, that is more than the maximal
    //     of complexity, it has been set the maximal of complexity.",
    //     rnnoise_info->complexity); rnnoise_info->complexity = COMPLEXITY_MAX_NUM;
    // }
    // if (filter->rnnoise_info->mode == RESAMPLE_DECODE_MODE)
    // {
    //     rnnoise_info->max_indata_bytes = MAX(rnnoise_info->max_indata_bytes,
    //     RESAMPLING_POINT_NUM * rnnoise_info->src_ch); //`RESAMPLING_POINT_NUM *
    //     rnnoise_info->src_ch` is for mininum of `rnnoise_info->max_indata_bytes`,
    //     enough extra buffer for safety filter->in_offset =
    //     rnnoise_info->max_indata_bytes;
    // }
    // else if (filter->rnnoise_info->mode == RESAMPLE_ENCODE_MODE)
    // {
    //     int tmp = (int)((float)rnnoise_info->src_rate * rnnoise_info->src_ch) /
    //     ((float)rnnoise_info->dest_rate * rnnoise_info->dest_ch); tmp = tmp > 1.0 ? tmp
    //     : 1.0; rnnoise_info->max_indata_bytes = (int)(rnnoise_info->out_len_bytes *
    //     tmp) + RESAMPLING_POINT_NUM * rnnoise_info->src_ch; //`RESAMPLING_POINT_NUM *
    //     rnnoise_info->src_ch` has no meaning, just enought extra buffer for safety
    //     filter->in_offset = 0;
    // }
    // filter->flag = 0;
    // filter->rnnoise_hd = esp_filter_create(nullptr,
    //                                         (unsigned char **)&filter->in_buf,
    //                                         (unsigned char **)&filter->out_buf);
    // if (filter->rnnoise_hd == NULL)
    // {
    //     ESP_LOGE(TAG, "Failed to create the rnnoise handler");
    //     return ESP_FAIL;
    // }
    // ESP_LOGI(TAG, "sample rate of source data : %d, channel of source data : %d, sample
    // rate of destination data : %d, channel of destination data : %d",
    //          filter->rnnoise_info->src_rate, filter->rnnoise_info->src_ch,
    //          filter->rnnoise_info->dest_rate, filter->rnnoise_info->dest_ch);
    ESP_LOGI(TAG, "opened");
    return ESP_OK;
}

static esp_err_t rnnoise_filter_close(audio_element_handle_t self)
{
    rnnoise_filter_t *filter = (rnnoise_filter_t *)audio_element_getdata(self);
    // rnnoise_destroy(filter->rnnoise_info->st);
    // if (filter->rnnoise_hd != NULL)
    // {
    //     // rnnoise_filter_destroy(filter->rnnoise_hd);
    //     filter->rnnoise_hd = 0;
    // }
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
    // rnnoise_info_t *rnnoise_info = audio_calloc(1, sizeof(rnnoise_info_t));
    // AUDIO_MEM_CHECK(TAG, rnnoise_info,
    //                 {
    //                     audio_free(filter);
    //                     return NULL;
    //                 });
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
        // audio_free(rnnoise_info);
        return NULL;
    });
    // filter->rnnoise_info = rnnoise_info;
    audio_element_setdata(el, filter);
    audio_element_info_t info = AUDIO_ELEMENT_INFO_DEFAULT();
    audio_element_setinfo(el, &info);
    // rnnoise_info->st = rnnoise_create(NULL);
    ESP_LOGI(TAG, "rnnoise_filter_init");
    return el;
}
