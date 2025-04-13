idf_component_register(
    SRCS "imu_test.c" "mpu6050_driver.c" "imu.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_driver_i2c esp_timer filter math unity
)
