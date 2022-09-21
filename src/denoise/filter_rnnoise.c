
#include <string.h>
#include "esp_log.h"
#include "audio_error.h"
#include "audio_mem.h"
#include "audio_element.h"
#include "filter_rnnoise.h"
#include "audio_type_def.h"
#include "rnnoise.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "nnom.h"
#include "denoise_weights.h"
#include "mfcc.h"

// the bandpass filter coefficiences
#include "equalizer_coeff.h"

#define NUM_FEATURES NUM_FILTER

#define _MAX(x, y) (((x) > (y)) ? (x) : (y))
#define _MIN(x, y) (((x) < (y)) ? (x) : (y))

#define NUM_CHANNELS 1
#define SAMPLE_RATE 16000
#define AUDIO_FRAME_LEN 1000

static bool filtered = false;

// mfcc features, 0 offset, 26 bands, 512fft, 0 preempha, attached_energy_to_band0
mfcc_t *mfcc; // = mfcc_create(NUM_FEATURES, 0, NUM_FEATURES, 512, 0, true);

// audio buffer for input
float audio_buffer[AUDIO_FRAME_LEN] = {0};
int16_t audio_buffer_16bit[AUDIO_FRAME_LEN] = {0};

// buffer for output
int16_t audio_buffer_filtered[AUDIO_FRAME_LEN / 2] = {0};

// mfcc features and their derivatives
float mfcc_feature[NUM_FEATURES] = {0};
float mfcc_feature_prev[NUM_FEATURES] = {0};
float mfcc_feature_diff[NUM_FEATURES] = {0};
float mfcc_feature_diff_prev[NUM_FEATURES] = {0};
float mfcc_feature_diff1[NUM_FEATURES] = {0};
// features for NN
float nn_features[64] = {0};
int8_t nn_features_q7[64] = {0};

// NN results, which is the gains for each frequency band
float band_gains[NUM_FILTER] = {0};
float band_gains_prev[NUM_FILTER] = {0};

// 0db gains coefficient
float coeff_b[NUM_FILTER][NUM_COEFF_PAIR] = {FILTER_COEFF_B};
float coeff_a[NUM_FILTER][NUM_COEFF_PAIR] = {FILTER_COEFF_A};
// dynamic gains coefficient
float b_[NUM_FILTER][NUM_COEFF_PAIR] = {0};

// nnom model
nnom_model_t *model;

// for microphone related data.
volatile bool is_half_updated = false;
volatile bool is_full_updated = false;
int32_t dma_audio_buffer[AUDIO_FRAME_LEN];

// update the history
void y_h_update(float *y_h, uint32_t len)
{
    for (uint32_t i = len - 1; i > 0; i--)
        y_h[i] = y_h[i - 1];
}

void enable_filter(bool en){
    filtered = en;
}
//  equalizer by multiple n order iir band pass filter.
// y[i] = b[0] * x[i] + b[1] * x[i - 1] + b[2] * x[i - 2] - a[1] * y[i - 1] - a[2] * y[i -
// 2]...
void equalizer(float *x, float *y, uint32_t signal_len, float *b, float *a,
               uint32_t num_band, uint32_t num_order)
{
    // the y history for each band
    static float y_h[NUM_FILTER][NUM_COEFF_PAIR] = {0};
    static float x_h[NUM_COEFF_PAIR * 2] = {0};
    uint32_t num_coeff = num_order * 2 + 1;

    // i <= num_coeff (where historical x is involved in the first few points)
    // combine state and new data to get a continual x input.
    memcpy(x_h + num_coeff, x, num_coeff * sizeof(float));
    for (uint32_t i = 0; i < num_coeff; i++) {
        y[i] = 0;
        for (uint32_t n = 0; n < num_band; n++) {
            y_h_update(y_h[n], num_coeff);
            y_h[n][0] = b[n * num_coeff] * x_h[i + num_coeff];
            for (uint32_t c = 1; c < num_coeff; c++)
                y_h[n][0] += b[n * num_coeff + c] * x_h[num_coeff + i - c] -
                             a[n * num_coeff + c] * y_h[n][c];
            y[i] += y_h[n][0];
        }
    }
    // store the x for the state of next round
    memcpy(x_h, &x[signal_len - num_coeff], num_coeff * sizeof(float));

    // i > num_coeff; the rest data not involed the x history
    for (uint32_t i = num_coeff; i < signal_len; i++) {
        y[i] = 0;
        for (uint32_t n = 0; n < num_band; n++) {
            y_h_update(y_h[n], num_coeff);
            y_h[n][0] = b[n * num_coeff] * x[i];
            for (uint32_t c = 1; c < num_coeff; c++)
                y_h[n][0] +=
                    b[n * num_coeff + c] * x[i - c] - a[n * num_coeff + c] * y_h[n][c];
            y[i] += y_h[n][0];
        }
    }
}

// set dynamic gains. Multiple gains x b_coeff
void set_gains(float *b_in, float *b_out, float *gains, uint32_t num_band,
               uint32_t num_order)
{
    uint32_t num_coeff = num_order * 2 + 1;
    for (uint32_t i = 0; i < num_band; i++)
        for (uint32_t c = 0; c < num_coeff; c++)
            b_out[num_coeff * i + c] = b_in[num_coeff * i + c] * gains[i];
}

void quantize_data(float *din, int8_t *dout, uint32_t size, uint32_t int_bit)
{
    float limit = (1 << int_bit);
    for (uint32_t i = 0; i < size; i++)
        dout[i] = (int8_t)(_MAX(_MIN(din[i], limit), -limit) / limit * 127);
}

void Error_Handler()
{
    printf("error\n");
}

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
    // NNoM model
    model = nnom_model_create();
    // mfcc features, 0 offset, 26 bands, 512fft, 0 preempha, attached_energy_to_band0
    mfcc = mfcc_create(NUM_FEATURES, 0, NUM_FEATURES, 512, 0, true);
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
#define SaturaLH(N, L, H) (((N) < (L)) ? (L) : (((N) > (H)) ? (H) : (N)))
    rnnoise_filter_t *filter = (rnnoise_filter_t *)audio_element_getdata(self);
    int out_len = -1;
    int read_len = 0;
    int in_bytes_consumed = 0;
    int ret = 0;
    int16_t *tmp = (int16_t *)in_buffer;
    int64_t m_old_time = esp_timer_get_time();
    read_len = audio_element_input(self, in_buffer, in_len);

    // leave only one channel data
    // for (int i = 0; i <= in_len / 2; i = i + 2) {
    //     tmp[i] = 0;
    // }

    for (int i = 0; i <= in_len / 2; i++) {
        audio_buffer_16bit[i] = tmp[i * 2 + 1];
        // tmp[i] = 0;
    }

    // get mfcc
    mfcc_compute(mfcc, audio_buffer_16bit, mfcc_feature);

    // get the first and second derivative of mfcc
    for (uint32_t i = 0; i < NUM_FEATURES; i++) {
        mfcc_feature_diff[i] = mfcc_feature[i] - mfcc_feature_prev[i];
        mfcc_feature_diff1[i] = mfcc_feature_diff[i] - mfcc_feature_diff_prev[i];
    }
    memcpy(mfcc_feature_prev, mfcc_feature, NUM_FEATURES * sizeof(float));
    memcpy(mfcc_feature_diff_prev, mfcc_feature_diff, NUM_FEATURES * sizeof(float));

    // combine MFCC with derivatives for the NN features
    memcpy(nn_features, mfcc_feature, NUM_FEATURES * sizeof(float));
    memcpy(&nn_features[NUM_FEATURES], mfcc_feature_diff, 10 * sizeof(float));
    memcpy(&nn_features[NUM_FEATURES + 10], mfcc_feature_diff1, 10 * sizeof(float));

    // quantise them using the same scale as training data (in keras), by 2^n.
    quantize_data(nn_features, nn_features_q7, NUM_FEATURES + 20, 3);

    // run the mode with the new input
    memcpy(nnom_input_data, nn_features_q7, sizeof(nnom_input_data));
    model_run(model);
    // nn_time = us_timer_get() - start_time - mfcc_time;

    // read the result, convert it back to float (q0.7 to float)
    for (int i = 0; i < NUM_FEATURES; i++)
        band_gains[i] = (float)(nnom_output_data[i]) / 127.f;

    // one more step, limit the change of gians, to smooth the speech, per RNNoise
    // paper
    for (int i = 0; i < NUM_FEATURES; i++)
        band_gains[i] = _MAX(band_gains_prev[i] * 0.8f, band_gains[i]);
    memcpy(band_gains_prev, band_gains, NUM_FEATURES * sizeof(float));

    // update filter coefficient to applied dynamic gains to each frequency band
    set_gains((float *)coeff_b, (float *)b_, band_gains, NUM_FILTER, NUM_ORDER);

    // convert 16bit to float for equalizer
    for (int i = 0; i < AUDIO_FRAME_LEN / 2; i++)
        audio_buffer[i] = audio_buffer_16bit[i + AUDIO_FRAME_LEN / 2] / 32768.f;

    // finally, we apply the equalizer to this audio frame to denoise
    equalizer(audio_buffer, &audio_buffer[AUDIO_FRAME_LEN / 2], AUDIO_FRAME_LEN / 2,
              (float *)b_, (float *)coeff_a, NUM_FILTER, NUM_ORDER);
    // equalizer_time = us_timer_get() - start_time - (mfcc_time + nn_time);

    // convert it back to int16
    for (int i = 0; i < AUDIO_FRAME_LEN / 2; i++)
        audio_buffer_filtered[i] = audio_buffer[i + AUDIO_FRAME_LEN / 2] * 32768.f *
                                   0.6f; // 0.7 is the filter band overlapping factor

    // voice detection to show an LED PE8 -> GreenLED
    // if (nnom_output_data1[0] >= 64)
    //     led(true);
    // else
    //     led(false);

    // filtered frame is now in audio_buffer_filtered[], size = half_frame
    // you may implement your own method to store or playback the voice.
    if (filtered) {
        for (int i = 0; i <= in_len / 2; i++) {
            tmp[i * 2 + 1] = audio_buffer_filtered[i];
            // tmp[i] = 0;
        }
    }
    out_len = audio_element_output(self, (char *)in_buffer, read_len);
    int64_t m_new_time = esp_timer_get_time();
    // printf("time %f %d\n", (m_new_time - m_old_time) / 1000.0f, in_len);

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
    cfg.buffer_len = 2000;
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
