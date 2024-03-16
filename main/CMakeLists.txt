set(srcs "msp/msp.c" "msp/msp_serial.c"
         "fc/init.c"
         "drivers/system.c" "drivers/system_esp32.c" "drivers/persistent.c"
         "drivers/bluetooth/ble.c"  
         "common/queue.c"
         "utils/util.c"
         "io/led.c"
         "sensors/voltage.c" "sensors/imu.c"
        #  "libs/mpu6050/src/driver_mpu6050.c"
        #  "libs/mpu6050/example/driver_mpu6050_basic.c"
        #  "drivers/mpu6050/driver_mpu6050_interface.c"
         "libs/MahonyAHRS/MahonyAHRS.c"
)

if(CONFIG_FOO_ENABLE_BAR)
    list(APPEND srcs "bar.c")
endif()

idf_component_register(SRCS "main.c" "${srcs}"
                    INCLUDE_DIRS "" "msp" "fc" "drivers" "common"
                    "drivers/bluetooth" "utils" "io" "sensors" "flight"
                    # "libs/mpu6050/interface" "libs/mpu6050/src" "libs/mpu6050/example"
                    "libs/MahonyAHRS"
                    )
