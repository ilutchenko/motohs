
#include "audio_pipeline.h"
#include "audio_device.hpp"

#define I2S_BCK_IO (GPIO_NUM_13)
#define I2S_WS_IO (GPIO_NUM_21)
#define I2S_DO_IO (GPIO_NUM_15)
#define I2S_DI_IO (-1)
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_26
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_25
#define I2S_MIC_SERIAL_DATA GPIO_NUM_27

class AudioManager
{
public:
    AudioManager()
    {
        audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
        _pipeline = audio_pipeline_init(&pipeline_cfg);

        audio_pipeline_register(_pipeline, speaker.element(), speaker.name());
        audio_pipeline_register(_pipeline, mic.element(), mic.name());

        const char *link_tag[2] = {mic.name(), speaker.name()};
        audio_pipeline_link(_pipeline, &link_tag[0], 2);
    }

    bool start()
    {
        return (audio_pipeline_run(_pipeline) == ESP_OK);
    }

private:
    audio_pipeline_handle_t _pipeline;
    AudioDevice speaker{"i2s_speaker", AUDIO_STREAM_WRITER, 48000,    I2S_BCK_IO,
                        I2S_WS_IO,     I2S_DO_IO,           I2S_DI_IO};
    AudioDevice mic{"i2s_mic",
                    AUDIO_STREAM_READER,
                    48000,
                    I2S_MIC_SERIAL_CLOCK,
                    I2S_MIC_LEFT_RIGHT_CLOCK,
                    I2S_PIN_NO_CHANGE,
                    I2S_MIC_SERIAL_DATA};
};