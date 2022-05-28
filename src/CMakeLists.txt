set(COMPONENT_SRCS "main.cpp")
set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_REQUIRES logging cli audio_pipeline audio_stream denoise)
register_component()
