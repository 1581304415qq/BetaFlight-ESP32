set(srcs "msp/msp.c" "msp/msp_serial.c"
         "fc/init.c"
         "drivers/system.c" "drivers/system_esp32.c"
         "utils/util.c"
         "sensors/voltage.c"
)

if(CONFIG_FOO_ENABLE_BAR)
    list(APPEND srcs "bar.c")
endif()

idf_component_register(SRCS "main.c" "${srcs}"
                    INCLUDE_DIRS "" "msp" "fc" "drivers"
                    "utils" "sensors" "flight"

                    REQUIRES driver esp_adc storage wifilink sensor common btlink device
                    )
