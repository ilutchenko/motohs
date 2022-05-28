#include "logger.h"
#include "console.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "audio_pipeline.h"
#include "audio_common.h"
#include "audio_element.h"
#include "i2s_stream.h"
#include "driver/i2s.h"
#include "filter_rnnoise.h"

const char TAG[] = "Main";
#define I2S_BCK_IO (GPIO_NUM_13)
#define I2S_WS_IO (GPIO_NUM_21)
#define I2S_DO_IO (GPIO_NUM_15)
#define I2S_DI_IO (-1)
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_26
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_25
#define I2S_MIC_SERIAL_DATA GPIO_NUM_27

extern "C" {
void app_main();
}

void app_main(void)
{
    Logger::init();
    cli_start();
    LOGI_TAGGED("Main", "Motoheadset started");

    audio_pipeline_handle_t pipeline;
    audio_element_handle_t bt_stream_reader, i2s_stream_writer, i2s_stream_reader,
        rnnoise_filter;

    ESP_LOGI(TAG, "[ 3 ] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);

    ESP_LOGI(TAG, "[3.1] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.i2s_config.sample_rate = 16000;
    i2s_stream_cfg_t i2s_cfg1 = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg1.i2s_config.sample_rate = 16000;

    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    i2s_cfg1.type = AUDIO_STREAM_READER;
    i2s_stream_reader = i2s_stream_init(&i2s_cfg1);
    
    rnnoise_filter_cfg_t rnnoise_cfg = DEFAULT_RNNOISE_FILTER_CONFIG();
    rnnoise_cfg.max_indata_bytes = FRAME_SIZE;
    rnnoise_filter = rnnoise_filter_init(&rnnoise_cfg);

    i2s_pin_config_t i2sPins = {.bck_io_num = I2S_BCK_IO,
                                .ws_io_num = I2S_WS_IO,
                                .data_out_num = I2S_DO_IO,
                                .data_in_num = -1};
    i2s_set_pin(i2s_cfg.i2s_port, &i2sPins);

    i2s_pin_config_t i2sPins1 = {.bck_io_num = I2S_MIC_SERIAL_CLOCK,
                                 .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
                                 .data_out_num = I2S_PIN_NO_CHANGE,
                                 .data_in_num = I2S_MIC_SERIAL_DATA};
    i2s_set_pin(i2s_cfg1.i2s_port, &i2sPins1);

    ESP_LOGI(TAG, "[3.2] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");
    audio_pipeline_register(pipeline, i2s_stream_reader, "i2s_mic");
    audio_pipeline_register(pipeline, rnnoise_filter, "rnnoise");

    const char *link_tag[3] = {"i2s_mic", "rnnoise", "i2s"};
    audio_pipeline_link(pipeline, &link_tag[0], 3);

    ESP_LOGI(TAG, "[ 6 ] Start audio_pipeline");
    audio_pipeline_run(pipeline);

    while (1){
        printf("SECOND\n");
        vTaskDelay(100);
    }
    /* Sheduler started automatically on boot,
        so we don't need to call vTaskStartScheduler()*/
}
