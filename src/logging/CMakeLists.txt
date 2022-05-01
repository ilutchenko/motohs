idf_component_register(SRCS "logger.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES)
add_subdirectory(fmt)
target_link_libraries(${COMPONENT_LIB} PUBLIC fmt::fmt-header-only)
