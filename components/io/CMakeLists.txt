idf_component_register(
    SRCS "led_driver.c" "led.c" "serial.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_driver_gpio esp_timer esp_ringbuf
)