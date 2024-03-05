set(srcs "msp/msp.c" "msp/msp_serial.c"
         "fc/init.c"
         "drivers/system.c" "drivers/system_esp32.c"
         "drivers/ble/bluetooth.c"
         "common/queue.c"
         "utils/util.c"
         "io/led.c"
         "sensors/voltage.c"
)

if(CONFIG_FOO_ENABLE_BAR)
    list(APPEND srcs "bar.c")
endif()

idf_component_register(SRCS "main.c" "${srcs}"
                    INCLUDE_DIRS "" "msp" "fc" "drivers" "common"
                    "drivers/ble" "utils" "io" "sensors" "flight"
                    )
