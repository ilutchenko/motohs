
#include <string.h>
#include "esp_log.h"
#include "audio_error.h"
#include "audio_mem.h"
#include "audio_element.h"
#include "audio_type_def.h"
#include "webrtc_ns.hpp"
#include "noise_suppression.h"

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

static const char *TAG = "WEBRTC_NS_FILTER";
// static const int k_denoiseFrameSize = 480;
#define RESAMPLING_POINT_NUM (2048)

typedef struct webrtc_ns_filter {
    // webrtc_ns_info_t *webrtc_ns_info;
    unsigned char *out_buf;
    unsigned char *in_buf;
    void *webrtc_ns_hd;
    int in_offset;
    int8_t flag; // the flag must be 0 or 1. 1 means the parameter in `webrtc_ns_info_t`
                 // changed. 0 means the parameter in `webrtc_ns_info_t` is original.
} webrtc_ns_filter_t;

static esp_err_t webrtc_ns_filter_destroy(audio_element_handle_t self)
{
    webrtc_ns_filter_t *filter = (webrtc_ns_filter_t *)audio_element_getdata(self);
    if (filter != NULL) {
        // webrtc_ns_destroy(filter->st);
        audio_free(filter);
    }
    return ESP_OK;
}

static esp_err_t webrtc_ns_filter_open(audio_element_handle_t self)
{
    webrtc_ns_filter_t *filter = (webrtc_ns_filter_t *)audio_element_getdata(self);
    // webrtc_ns_info_t *webrtc_ns_info = filter->webrtc_ns_info;
    // filter->st = webrtc_ns_create(NULL);
    filter->flag = 0;
    ESP_LOGI(TAG, "opened");
    return ESP_OK;
}

static esp_err_t webrtc_ns_filter_close(audio_element_handle_t self)
{
    webrtc_ns_filter_t *filter = (webrtc_ns_filter_t *)audio_element_getdata(self);
    // webrtc_ns_destroy(filter->webrtc_ns_info->st);
    // if (filter->webrtc_ns_hd != NULL)
    // {
    //     // webrtc_ns_filter_destroy(filter->webrtc_ns_hd);
    //     filter->webrtc_ns_hd = 0;
    // }
    return ESP_OK;
}

#ifndef MIN
#define MIN(A, B)        ((A) < (B) ? (A) : (B))
#endif
static float inb[FRAME_SIZE_NS];
static float outb[FRAME_SIZE_NS];
enum nsLevel {
    kLow,
    kModerate,
    kHigh,
    kVeryHigh
};

int nsProcess(int16_t *buffer, uint32_t sampleRate, uint64_t samplesCount, uint32_t channels, enum nsLevel level) {
    if (buffer == nullptr) return -1;
    if (samplesCount == 0) return -1;
    size_t samples = MIN(160, sampleRate / 100);
    if (samples == 0) return -1;
    uint32_t num_bands = 1;
    int16_t *input = buffer;
    size_t frames = (samplesCount / (samples * channels));
    int16_t *frameBuffer = (int16_t *) malloc(sizeof(*frameBuffer) * channels * samples);
    NsHandle **NsHandles = (NsHandle **) malloc(channels * sizeof(NsHandle *));
    if (NsHandles == NULL || frameBuffer == NULL) {
        if (NsHandles)
            free(NsHandles);
        if (frameBuffer)
            free(frameBuffer);
        fprintf(stderr, "malloc error.\n");
        return -1;
    }
    for (int i = 0; i < channels; i++) {
        NsHandles[i] = WebRtcNs_Create();
        if (NsHandles[i] != NULL) {
            int status = WebRtcNs_Init(NsHandles[i], sampleRate);
            if (status != 0) {
                fprintf(stderr, "WebRtcNs_Init fail\n");
                WebRtcNs_Free(NsHandles[i]);
                NsHandles[i] = NULL;
            } else {
                status = WebRtcNs_set_policy(NsHandles[i], level);
                if (status != 0) {
                    fprintf(stderr, "WebRtcNs_set_policy fail\n");
                    WebRtcNs_Free(NsHandles[i]);
                    NsHandles[i] = NULL;
                }
            }
        }
        if (NsHandles[i] == NULL) {
            for (int x = 0; x < i; x++) {
                if (NsHandles[x]) {
                    WebRtcNs_Free(NsHandles[x]);
                }
            }
            free(NsHandles);
            free(frameBuffer);
            return -1;
        }
    }
    for (int i = 0; i < frames; i++) {
        for (int c = 0; c < channels; c++) {
            for (int k = 0; k < samples; k++)
                frameBuffer[k] = input[k * channels + c];

            int16_t *nsIn[1] = {frameBuffer};   //ns input[band][data]
            int16_t *nsOut[1] = {frameBuffer};  //ns output[band][data]
            WebRtcNs_Analyze(NsHandles[c], nsIn[0]);
            WebRtcNs_Process(NsHandles[c], (const int16_t *const *) nsIn, num_bands, nsOut);
            for (int k = 0; k < samples; k++)
                input[k * channels + c] = frameBuffer[k];
        }
        input += samples * channels;
    }

    for (int i = 0; i < channels; i++) {
        if (NsHandles[i]) {
            WebRtcNs_Free(NsHandles[i]);
        }
    }
    free(NsHandles);
    free(frameBuffer);
    return 1;
}



static audio_element_err_t webrtc_ns_filter_process(audio_element_handle_t self, char *in_buffer,
                                  int in_len)
{
    webrtc_ns_filter_t *filter = (webrtc_ns_filter_t *)audio_element_getdata(self);
    int out_len = -1;
    int read_len = 0;
    int in_bytes_consumed = 0;
    int ret = 0;
    short *tmp = (short *)in_buffer;

    read_len = audio_element_input(self, in_buffer, in_len);
    // printf("%d %d | %d %d %d %d\n", in_len, read_len, in_buffer[0], in_buffer[1],
    //        in_buffer[2], in_buffer[3]);

    if (read_len < FRAME_SIZE_NS) {
        return AEL_IO_FAIL;
    }

    // int i = 0;
    // while (i < (FRAME_SIZE_NS / 2)) {
    //     inb[i] = (float)(in_buffer[i] << 8 | in_buffer[i + 1]);
    //     i = i + 2;
    // }

    int i = 0;
    while (i < (FRAME_SIZE_NS / 2)) {
        inb[i] = (float)(in_buffer[i] << 8 | in_buffer[i + 1]);
        i = i + 2;
    }

    // webrtc_ns_process_frame(filter->st, outb, inb);
    
    for (int j = 0; j < 4; j++) {
        // nsProc(tmp, read_len, 16000, 1);
        nsProcess(tmp, 16000, read_len, 1, kModerate);
        // webrtc_ns_process_frame(filter->st, outb + (480 * j), inb + (480 * j));

    }

    // i = 0;
    // uint16_t b;
    // while (i < (FRAME_SIZE_NS)) {
    //     inb[i] = (float)(in_buffer[i] << 8 | in_buffer[i + 1]);
    //     b = (uint16_t)outb[i/2];
    //     in_buffer[i] = (char)b;
    //     in_buffer[i+1] = (char)(b >> 8);
    //     i = i + 2;
    // }

    // for (int i = 0; i < FRAME_SIZE_NS; i++) {
    //     in_buffer[i] = (char)outb[i];
    // }

    out_len = audio_element_output(self, in_buffer, read_len);
    audio_element_update_byte_pos(self, out_len);
    return AEL_IO_OK;
}

audio_element_handle_t webrtc_ns_filter_init(webrtc_ns_filter_cfg_t *config)
{
    if (config == NULL) {
        ESP_LOGE(TAG, "config is NULL. (line %d)", __LINE__);
        return NULL;
    }
    webrtc_ns_filter_t *filter =
        (webrtc_ns_filter_t *)audio_calloc(1, sizeof(webrtc_ns_filter_t));
    AUDIO_MEM_CHECK(TAG, filter, return NULL);

    audio_element_cfg_t cfg;
    cfg.buffer_len = FRAME_SIZE_NS;
    cfg.task_stack = DEFAULT_ELEMENT_STACK_SIZE;
    cfg.task_prio = DEFAULT_ELEMENT_TASK_PRIO;
    cfg.task_core = 1;
    cfg.multi_in_rb_num = 0;
    cfg.multi_out_rb_num = 0;

    cfg.destroy = webrtc_ns_filter_destroy;
    cfg.process = webrtc_ns_filter_process;
    cfg.open = webrtc_ns_filter_open;
    cfg.close = webrtc_ns_filter_close;
    // cfg.buffer_len = 0;
    cfg.tag = "webrtc_ns";
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_rb_size = config->out_rb_size;
    cfg.stack_in_ext = config->stack_in_ext;
    audio_element_handle_t el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, {
        audio_free(filter);
        // audio_free(webrtc_ns_info);
        return NULL;
    });
    // filter->webrtc_ns_info = webrtc_ns_info;
    audio_element_setdata(el, filter);
    audio_element_info_t info = AUDIO_ELEMENT_INFO_DEFAULT();
    audio_element_setinfo(el, &info);
    // webrtc_ns_info->st = webrtc_ns_create(NULL);
    ESP_LOGI(TAG, "webrtc_ns_filter_init");
    return el;
}
