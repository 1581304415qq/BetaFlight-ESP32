#include "driver_mpu6050_interface.h"

#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"


#define I2C_MASTER_TIMEOUT_MS 1000
#define I2C_MASTER_NUM 0
#define I2C_MASTER_SDA_IO GPIO_NUM_17
#define I2C_MASTER_SCL_IO GPIO_NUM_18
#define I2C_MASTER_FREQ_HZ 400000
#define I2C_MASTER_RX_BUF_DISABLE 0
#define I2C_MASTER_TX_BUF_DISABLE 0


/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

uint8_t mpu6050_interface_iic_init(void) {
    return ESP_OK == i2c_master_init() ? 0 : 1;
}

uint8_t mpu6050_interface_iic_deinit(void) {
    return ESP_OK == i2c_driver_delete(I2C_MASTER_NUM) ? 0 : 1;
}

uint8_t mpu6050_interface_iic_read(uint8_t addr, uint8_t reg, uint8_t* buf, uint16_t len) {
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM, addr, &reg, 1, buf, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    printf("ret=%d, add=0x%x, reg=0x%x, buf=%x,%x\n", ret, addr, reg, buf[0], buf[1]);
    return ESP_OK == ret ? 0 : 1;
}

uint8_t mpu6050_interface_iic_write(uint8_t addr, uint8_t reg, uint8_t* buf, uint16_t len) {
    uint8_t buf_send[len + 1];
    /* clear sent buf */
    memset(buf_send, 0, sizeof(uint8_t) * (len + 1));
    buf_send[0] = reg;
    memcpy(&buf_send[1], buf, len);
    return ESP_OK == i2c_master_write_to_device(I2C_MASTER_NUM, addr, buf_send, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS) ? 0 : 1;
}

void mpu6050_interface_delay_ms(uint32_t ms) {
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

void mpu6050_interface_debug_print(const char* const fmt, ...) {
    char str[256];
    uint16_t len;
    va_list args;

    memset((char*)str, 0, sizeof(char) * 256);
    va_start(args, fmt);
    vsnprintf((char*)str, 255, (char const*)fmt, args);
    va_end(args);

    len = strlen((char*)str);
    (void)printf((char*)str, len);
}

/**
 * @brief     interface receive callback
 * @param[in] type is the irq type
 * @note      none
 */
void mpu6050_interface_receive_callback(uint8_t type)
{
    switch (type)
    {
    case MPU6050_INTERRUPT_MOTION:
    {
        mpu6050_interface_debug_print("mpu6050: irq motion.\n");

        break;
    }
    case MPU6050_INTERRUPT_FIFO_OVERFLOW:
    {
        mpu6050_interface_debug_print("mpu6050: irq fifo overflow.\n");

        break;
    }
    case MPU6050_INTERRUPT_I2C_MAST:
    {
        mpu6050_interface_debug_print("mpu6050: irq i2c master.\n");

        break;
    }
    case MPU6050_INTERRUPT_DMP:
    {
        mpu6050_interface_debug_print("mpu6050: irq dmp\n");

        break;
    }
    case MPU6050_INTERRUPT_DATA_READY:
    {
        mpu6050_interface_debug_print("mpu6050: irq data ready\n");

        break;
    }
    default:
    {
        mpu6050_interface_debug_print("mpu6050: irq unknown code.\n");

        break;
    }
    }
}

/**
 * @brief     interface dmp tap callback
 * @param[in] count is the tap count
 * @param[in] direction is the tap direction
 * @note      none
 */
void mpu6050_interface_dmp_tap_callback(uint8_t count, uint8_t direction)
{
    switch (direction)
    {
    case MPU6050_DMP_TAP_X_UP:
    {
        mpu6050_interface_debug_print("mpu6050: tap irq x up with %d.\n", count);

        break;
    }
    case MPU6050_DMP_TAP_X_DOWN:
    {
        mpu6050_interface_debug_print("mpu6050: tap irq x down with %d.\n", count);

        break;
    }
    case MPU6050_DMP_TAP_Y_UP:
    {
        mpu6050_interface_debug_print("mpu6050: tap irq y up with %d.\n", count);

        break;
    }
    case MPU6050_DMP_TAP_Y_DOWN:
    {
        mpu6050_interface_debug_print("mpu6050: tap irq y down with %d.\n", count);

        break;
    }
    case MPU6050_DMP_TAP_Z_UP:
    {
        mpu6050_interface_debug_print("mpu6050: tap irq z up with %d.\n", count);

        break;
    }
    case MPU6050_DMP_TAP_Z_DOWN:
    {
        mpu6050_interface_debug_print("mpu6050: tap irq z down with %d.\n", count);

        break;
    }
    default:
    {
        mpu6050_interface_debug_print("mpu6050: tap irq unknown code.\n");

        break;
    }
    }
}

/**
 * @brief     interface dmp orient callback
 * @param[in] orientation is the dmp orientation
 * @note      none
 */
void mpu6050_interface_dmp_orient_callback(uint8_t orientation)
{
    switch (orientation)
    {
    case MPU6050_DMP_ORIENT_PORTRAIT:
    {
        mpu6050_interface_debug_print("mpu6050: orient irq portrait.\n");

        break;
    }
    case MPU6050_DMP_ORIENT_LANDSCAPE:
    {
        mpu6050_interface_debug_print("mpu6050: orient irq landscape.\n");

        break;
    }
    case MPU6050_DMP_ORIENT_REVERSE_PORTRAIT:
    {
        mpu6050_interface_debug_print("mpu6050: orient irq reverse portrait.\n");

        break;
    }
    case MPU6050_DMP_ORIENT_REVERSE_LANDSCAPE:
    {
        mpu6050_interface_debug_print("mpu6050: orient irq reverse landscape.\n");

        break;
    }
    default:
    {
        mpu6050_interface_debug_print("mpu6050: orient irq unknown code.\n");

        break;
    }
    }
}
