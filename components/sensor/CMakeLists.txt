idf_component_register(
    SRCS "mpu6050_driver.c" "imu.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_timer filter math
)
