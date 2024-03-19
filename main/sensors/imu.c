#include "imu.h"
#include "mpu6050.h"
#include "MahonyAHRS.h"
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#define I2C_MASTER_TIMEOUT_MS 1000
#define I2C_MASTER_NUM 0
#define I2C_MASTER_SDA_IO GPIO_NUM_17
#define I2C_MASTER_SCL_IO GPIO_NUM_18
#define I2C_MASTER_FREQ_HZ 400000
#define I2C_MASTER_RX_BUF_DISABLE 0
#define I2C_MASTER_TX_BUF_DISABLE 0

static mpu6050_handle_t mpu6050 = NULL;

#define TASK_STACK_SIZE 1024*5
#define TAG "BETA FLIGHT IMU"

volatile float twoKp = (2.0 * 2.46f);	// 2 * proportional gain 比例增益
volatile float twoKi = (2.0 * 0.003f);	// 2 * integral gain     积分增益
volatile float sampleFreq = 200.0f;	    // sample frequency in Hz
volatile float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;   // quaternion of sensor frame relative to auxiliary frame
static double yaw, pitch, roll;

/**
 * @brief i2c master initialization
 */
static void i2c_bus_init(void)
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = (gpio_num_t)I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    conf.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;

    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    // TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, ret, "I2C config returned error");

    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    // TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, ret, "I2C install returned error");
}

/**
 * @brief i2c master initialization
 */
static void i2c_sensor_mpu6050_init(void)
{
    esp_err_t ret;

    i2c_bus_init();
    mpu6050 = mpu6050_create(I2C_MASTER_NUM, MPU6050_I2C_ADDRESS);
    // TEST_ASSERT_NOT_NULL_MESSAGE(mpu6050, "MPU6050 create returned NULL");

    ret = mpu6050_config(mpu6050, ACCE_FS_4G, GYRO_FS_500DPS);
    // TEST_ASSERT_EQUAL(ESP_OK, ret);
    printf("mpu6050 config ret=%d\n", ret);

    ret = mpu6050_wake_up(mpu6050);
    // TEST_ASSERT_EQUAL(ESP_OK, ret);
    printf("mpu6050 wake up ret=%d\n", ret);
}

#define  RAD2DEG (double)57.29577951
static void quaternion2Angle() {
    // yaw = atan2(2 * q1 * q2 - 2 * q0 * q3, 2 * q0 * q0 + 2 * q1 * q1 - 1) * RAD2DEG;
    // pitch = -asin(2 * q1 * q3 + 2 * q0 * q2) * RAD2DEG;
    // roll = atan2(2 * q2 * q3 - 2 * q0 * q1, 2 * q0 * q0 + 2 * q3 * q3 - 1) * RAD2DEG;
    roll = atan2(2 * q0 * q1 + 2 * q2 * q3, 1 - 2 * q1 * q1 - 2 * q2 * q2) * RAD2DEG;
    pitch = asin(2 * q0 * q2 - 2 * q3 * q1) * RAD2DEG;
    yaw = atan2(2 * q0 * q3 + 2 * q1 * q2, 1 - 2 * q2 * q2 - 2 * q3 * q3) * RAD2DEG;
    // yaw = -atan2(2.0f * (q1 * q2 + q0 * q3), q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3) * RAD2DEG;
    // pitch = asin(2.0f * (q1 * q3 - q0 * q2)) * RAD2DEG;
    // roll = atan2(2.0f * (q0 * q1 + q2 * q3), q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3) * RAD2DEG;
}


// 默认取样个数
#define DEFAULT_CALIBRATION_NUMSAMPLES 1000
// 误差抖动 阈值
#define DEFAULT_CALIBRATION_ACCEL_DEADZONE  0.001*0.001
#define DEFAULT_CALIBRATION_GYRO_DEADZONE  0.001*0.001

struct Calibrate
{
    mpu6050_acce_value_t acce;
    mpu6050_gyro_value_t gyro;
} calibrate;

void calculateMean(uint32_t numsample, struct Calibrate* calibrate) {
    mpu6050_acce_value_t acce;
    mpu6050_gyro_value_t gyro;
    esp_err_t ret;
    double pre_avg[7] = { 0 };    // 存储取样数量的和
    for (uint32_t i = 0; i < numsample;i++) {
        ret = mpu6050_get_acce(mpu6050, &acce);
        ret = mpu6050_get_gyro(mpu6050, &gyro);

        pre_avg[0] += (acce.acce_x - 1);
        pre_avg[1] += acce.acce_y;
        pre_avg[2] += acce.acce_z;
        pre_avg[3] += gyro.gyro_x;
        pre_avg[4] += gyro.gyro_y;
        pre_avg[5] += gyro.gyro_z;
        // pre_avg[6] += temp;
    }

    calibrate->acce.acce_x = (double)(pre_avg[0] / numsample);
    calibrate->acce.acce_y = (double)(pre_avg[1] / numsample);
    calibrate->acce.acce_z = (double)(pre_avg[2] / numsample);
    calibrate->gyro.gyro_x = (double)(pre_avg[3] / numsample);
    calibrate->gyro.gyro_y = (double)(pre_avg[4] / numsample);
    calibrate->gyro.gyro_z = (double)(pre_avg[5] / numsample);

}

void read_imu_calibrate(
    mpu6050_acce_value_t* acce,
    mpu6050_gyro_value_t* gyro
) {
    esp_err_t ret;
    // 检查校准后的误差
    ret = mpu6050_get_acce(mpu6050, acce);
    ret = mpu6050_get_gyro(mpu6050, gyro);

    ESP_LOGI(TAG, "raw:%.5f, %.5f, %.5f, %.5f, %.5f, %.5f\n",
        acce->acce_x, acce->acce_y, acce->acce_z,
        gyro->gyro_x, gyro->gyro_y, gyro->gyro_z
    );
    acce->acce_x -= calibrate.acce.acce_x;
    acce->acce_y -= calibrate.acce.acce_y;
    acce->acce_z -= calibrate.acce.acce_z;
    gyro->gyro_x -= calibrate.gyro.gyro_x;
    gyro->gyro_y -= calibrate.gyro.gyro_y;
    gyro->gyro_z -= calibrate.gyro.gyro_z;

}
int imu_calibrate()
{
    esp_err_t ret;
    mpu6050_acce_value_t acce;
    mpu6050_gyro_value_t gyro;

    for (int i = 0;i < 10;i++)
    {
        // 读取采用数平均值
        calculateMean(DEFAULT_CALIBRATION_NUMSAMPLES, &calibrate);

        // 检查校准后的误差
        read_imu_calibrate(&acce, &gyro);

        if (acce.acce_z * acce.acce_z > DEFAULT_CALIBRATION_ACCEL_DEADZONE
            || acce.acce_y * acce.acce_y > DEFAULT_CALIBRATION_ACCEL_DEADZONE
            || (acce.acce_x - 1) * (acce.acce_x - 1) > DEFAULT_CALIBRATION_ACCEL_DEADZONE
            || gyro.gyro_x * gyro.gyro_x > DEFAULT_CALIBRATION_GYRO_DEADZONE
            || gyro.gyro_y * gyro.gyro_y > DEFAULT_CALIBRATION_GYRO_DEADZONE
            || gyro.gyro_z * gyro.gyro_z > DEFAULT_CALIBRATION_GYRO_DEADZONE
            )   continue;

        return 1;
    }
    return 0;
}


static void calculateVelocity(
    float acce_x,
    float acce_y,
    float acce_z,
    float pitch,
    float roll,
    float yaw
) {
    double g = 1.0;
    double g_x = acce_x - g * (cos(yaw) * sin(pitch) * cos(roll) + sin(yaw) * sin(roll));
    double g_y = acce_y - g * (sin(yaw) * sin(pitch) * cos(roll) - cos(yaw) * sin(roll));
    double g_z = acce_z - g * cos(pitch) * cos(roll);

    ESP_LOGI(TAG, "Velocity x:%.3f, y:%.3f, z:%.3f\n", g_x, g_y, g_z);
}

static void imuTask(void* param) {
    esp_err_t ret;
    uint8_t mpu6050_deviceid;
    mpu6050_acce_value_t acce;
    mpu6050_gyro_value_t gyro;
    mpu6050_temp_value_t temp;

    uint64_t now = 0, dt = 0, last_update = 0;
    // struct timeval tv_now;

    ret = mpu6050_get_deviceid(mpu6050, &mpu6050_deviceid);
    ESP_LOGI(TAG, "ret=%d, mpu6050_deviceid=0x%x\n", ret, mpu6050_deviceid);

    while(imu_calibrate());
    for (;;)
    {
        ret = mpu6050_get_temp(mpu6050, &temp);
        // ret = mpu6050_get_acce(mpu6050, &acce);
        // ret = mpu6050_get_gyro(mpu6050, &gyro);

        read_imu_calibrate(&acce, &gyro);

        ESP_LOGI(TAG, "imu:%.5f, %.5f, %.5f, %.5f, %.5f, %.5f\n",
            acce.acce_x, acce.acce_y, acce.acce_z,
            gyro.gyro_x, gyro.gyro_y, gyro.gyro_z
        );
        // ESP_LOGI(TAG, "ret=%d, mpu6050 temp=%f\n", ret, temp.temp);
        // ESP_LOGI(TAG, "acce_x:%.2f, acce_y:%.2f, acce_z:%.2f\n", acce.acce_x, acce.acce_y, acce.acce_z);
        // ESP_LOGI(TAG, "gyro_x:%.2f, gyro_y:%.2f, gyro_z:%.2f\n", gyro.gyro_x, gyro.gyro_y, gyro.gyro_z);

        if (ESP_OK != ret)continue;

        now = esp_timer_get_time()/1000L;
        dt = (now - last_update);
        sampleFreq = (float)(1000.0 / (now - last_update));
        last_update = now;
        ESP_LOGI(TAG, "sampleFreq:%.5f\n", sampleFreq);

        // update mahony imu
        MahonyAHRSupdateIMU(gyro.gyro_x / RAD2DEG, gyro.gyro_y / RAD2DEG, gyro.gyro_z / RAD2DEG, acce.acce_x, acce.acce_y, acce.acce_z);

        quaternion2Angle();
        ESP_LOGI(TAG, "angle:%.5f, %.5f, %.5f\n", yaw, pitch, roll);

        // calculateVelocity(acce.acce_x, acce.acce_y, acce.acce_z, pitch / RAD2DEG, roll / RAD2DEG, yaw / RAD2DEG);

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }


}

static TaskHandle_t xHandle = NULL;
void imuInit(void) {
    i2c_sensor_mpu6050_init();
    xTaskCreate(imuTask, "imu_task", TASK_STACK_SIZE, NULL, 10, &xHandle);
}

void imuDeinit(void) {
    // 结束线程
    if (xHandle != NULL)
    {
        vTaskDelete(xHandle);
    }
    mpu6050_delete(mpu6050);
    esp_err_t ret = i2c_driver_delete(I2C_MASTER_NUM);
}