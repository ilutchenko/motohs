idf_component_register(SRCS 
                        "filter_rnnoise.c"
                        "./rnnoise/src/celt_lpc.c"
                        "./rnnoise/src/denoise.c"
                        "./rnnoise/src/kiss_fft.c"
                        "./rnnoise/src/pitch.c"
                        "./rnnoise/src/rnn_data.c"
                        "./rnnoise/src/rnn_reader.c"
                        "./rnnoise/src/rnn.c"
                        INCLUDE_DIRS "."
                                "./rnnoise/include"
                                "./rnnoise/src"
REQUIRES audio_sal audio_pipeline debug_helpers)
