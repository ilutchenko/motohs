
#include <string>

#include "audio_common.h"
#include "audio_element.h"
#include "i2s_stream.h"
#include "driver/i2s.h"

class AudioDevice
{
public:
    AudioDevice(std::string n, audio_stream_type_t device_type, uint32_t sample_rate,
                int bck_pin, int ws_pin, int dout_pin, int din_pin)

    {
        _i2s_cfg.type = device_type;
        _i2s_cfg.i2s_config.sample_rate = sample_rate;
        _i2sPins.bck_io_num = bck_pin;
        _i2sPins.ws_io_num = ws_pin;
        _i2sPins.data_out_num = dout_pin;
        _i2sPins.data_in_num = din_pin;
        _name = n;
        i2s_set_pin(_i2s_cfg.i2s_port, &_i2sPins);
        _audio_element = i2s_stream_init(&_i2s_cfg);
    }

    audio_element_handle_t element()
    {
        return _audio_element;
    }

    const char* name()
    {
        return _name.c_str();
    }

private:
    std::string _name{};
    audio_element_handle_t _audio_element;
    i2s_stream_cfg_t _i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_pin_config_t _i2sPins;
};
