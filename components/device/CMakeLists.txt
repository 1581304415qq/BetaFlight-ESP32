idf_component_register(
    SRCS "led_driver.c" "led.c" "serial.c" "motor.c"
    "battery.c" "voltage.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES driver esp_adc esp_driver_gpio esp_driver_mcpwm esp_driver_ledc esp_timer esp_ringbuf
)