idf_component_register(SRC_DIRS
                        "."
                        "./nnom/src/layers/"
                        "./nnom/src/core/"
                        "./nnom/src/backends/"
                        "./rnnoise/src/"
# SRCS 
        #                 "filter_rnnoise.c"
        #                 "mfcc.c"
        #                 "./rnnoise/src/celt_lpc.c"
        #                 "./rnnoise/src/denoise.c"
        #                 "./rnnoise/src/kiss_fft.c"
        #                 "./rnnoise/src/pitch.c"
        #                 "./rnnoise/src/rnn_data.c"
        #                 "./rnnoise/src/rnn_reader.c"
        #                 "./rnnoise/src/rnn.c"
                        
        #                 "./nnom/src/core/nnom.c"
        #                 "./nnom/src/core/nnom_layers.c"
        #                 "./nnom/src/core/nnom_tensor.c"
        #                 "./nnom/src/core/nnom_utils.c"
                        INCLUDE_DIRS "."
                        "./rnnoise/include"
                                "./rnnoise/src"

                                "./nnom/inc/"
                                "./nnom/inc/layers/"
                        REQUIRES audio_sal audio_pipeline debug_helpers)
