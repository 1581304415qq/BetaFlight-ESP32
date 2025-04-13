set(srcs "msp/msp.c" "msp/msp_serial.c"
         "fc/init.c"
         "utils/util.c"
)

if(CONFIG_FOO_ENABLE_BAR)
    list(APPEND srcs "bar.c")
endif()

idf_component_register(SRCS "main.c" "${srcs}"
                    "drivers/system.c" "drivers/system_esp32.c"

                    INCLUDE_DIRS "" "msp" "fc" "utils" "flight" "drivers"
                    
                    REQUIRES esp_timer driver storage wifi sensor common btlink device server
                    )

spiffs_create_partition_image(html ../spiffs FLASH_IN_PROJECT)
