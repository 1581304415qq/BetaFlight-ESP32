#include <stdio.h>
#include "unity.h"
#include "freertos/FreeRTOS.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

// 从Kconfig获取配置参数
#define I2C_PORT        CONFIG_IMU_I2C_PORT_NUM
#define IMU_ADDR        CONFIG_IMU_DEVICE_ADDR
#define SDA_PIN         11//CONFIG_IMU_I2C_SDA_IO
#define SCL_PIN         10//CONFIG_IMU_I2C_SCL_IO
#define I2C_FREQ        CONFIG_IMU_I2C_FREQ_HZ

// MPU6050寄存器定义
#define WHO_AM_I        0x75
#define PWR_MGMT_1      0x6B
#define ACCEL_CONFIG    0x1C
#define GYRO_CONFIG     0x1B

static const char* TAG = "IMU_TEST";
i2c_master_dev_handle_t dev_handle;

// I2C初始化
static void i2c_master_init() {
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT,
        .scl_io_num = SCL_PIN,
        .sda_io_num = SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = IMU_ADDR,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
}

// 测试设备ID读取
// TEST_CASE("IMU basic communication", "[imu][i2c]") {
//     uint8_t data[2] = { 0 };
//     esp_err_t ret;

//     // 读取WHO_AM_I寄存器
//     data[0] = WHO_AM_I;
//     ret = i2c_master_write_read_device(I2C_PORT, IMU_ADDR,
//         data, 1, data, 1,
//         pdMS_TO_TICKS(1000));
//     TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, ret, "I2C读写操作失败");

//     ESP_LOGI(TAG, "Device ID: 0x%02X", data[0]);
//     TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x68, data[0],
//         "设备ID验证失败，请检查：\n"
//         "1. 物理连接(SDA/SCL)\n"
//         "2. I2C地址配置\n"
//         "3. 电源供应");
// }

// 传感器初始化测试
void imu_init_test()
{
    uint8_t cfg_data[2] = { 0 };

    // 唤醒设备
    cfg_data[0] = PWR_MGMT_1;
    cfg_data[1] = 0x00; // 解除睡眠模式
    TEST_ASSERT_EQUAL(ESP_OK,
        i2c_master_transmit(dev_handle,
            cfg_data, 2,
            pdMS_TO_TICKS(500)));

    // 配置加速度计 ±2g
    cfg_data[0] = ACCEL_CONFIG;
    cfg_data[1] = 0x00;
    TEST_ASSERT_EQUAL(ESP_OK,
        i2c_master_transmit(dev_handle,
            cfg_data, 2,
            pdMS_TO_TICKS(500)));

    // 配置陀螺仪 ±250°/s
    cfg_data[0] = GYRO_CONFIG;
    cfg_data[1] = 0x00;
    TEST_ASSERT_EQUAL(ESP_OK,
        i2c_master_transmit(dev_handle,
            cfg_data, 2,
            pdMS_TO_TICKS(500)));
}

// 数据读取测试
void read_sensor_data() {
    uint8_t reg = 0x3B; // ACCEL_XOUT_H
    uint8_t data[14] = { 0 };

    TEST_ASSERT_EQUAL(ESP_OK,
        i2c_master_transmit_receive(dev_handle,
            &reg, 1, data, 14,
            pdMS_TO_TICKS(1000)));

    // 数据解析
    int16_t accel_x = (data[0] << 8) | data[1];
    int16_t accel_y = (data[2] << 8) | data[3];
    int16_t accel_z = (data[4] << 8) | data[5];
    int16_t temp = (data[6] << 8) | data[7];
    int16_t gyro_x = (data[8] << 8) | data[9];
    int16_t gyro_y = (data[10] << 8) | data[11];
    int16_t gyro_z = (data[12] << 8) | data[13];

    ESP_LOGI(TAG,
        "Accel: X=%6d, Y=%6d, Z=%6d\n"
        "Temp: %6d\n"
        "Gyro: X=%6d, Y=%6d, Z=%6d",
        accel_x, accel_y, accel_z,
        temp,
        gyro_x, gyro_y, gyro_z);

    // 数据合理性检查
    // TEST_ASSERT_INT_WITHIN(1000, 0, accel_z); // Z轴应有1g重力
    // TEST_ASSERT_INT_WITHIN(1000, 0, gyro_x); // 静止状态角速度接近0
}

// 综合测试用例
// TEST_CASE("IMU full function test", "[imu][integration]") {
//     imu_init_test();
//     for (int i = 0; i < 5; i++) {
//         read_sensor_data();
//         vTaskDelay(pdMS_TO_TICKS(100));
//     }
// }

// 测试套件配置
void setUp(void) {
    i2c_master_init();
}

void tearDown(void) {
    // i2c_driver_delete(I2C_PORT);
}