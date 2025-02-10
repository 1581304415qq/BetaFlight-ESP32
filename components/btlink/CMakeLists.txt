if(NOT DEFINED CONFIG_IDF_TARGET_ESP32S2)
    list(APPEND src "ble.c")
endif()

idf_component_register(
    SRCS ${src}
    INCLUDE_DIRS "."
    PRIV_REQUIRES bt common
)