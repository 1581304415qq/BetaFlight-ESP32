#include "imu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "Matrix.h"
#include "mpu6050_driver.h"
#include "MahonyAHRS.h"
#include "unity.h"
#include "driver/i2c_master.h"

#include <stdio.h>
#include <math.h>

#define I2C_MASTER_FREQ_HZ_MAX            (1250000) // 1.25 MHz
#define I2C_MASTER_FREQ_HZ_1250K          (1250000) // 1.25 MHz
#define I2C_MASTER_FREQ_HZ_1000K          (1000000) // 1 MHz  
#define I2C_MASTER_FREQ_HZ_400K           (400000)  // 400 KHz
#define I2C_MASTER_FREQ_HZ_100K           (100000)  // 100 KHz

#define I2C_MASTER_TIMEOUT_MS 1000
#define I2C_MASTER_NUM 0
#define I2C_MASTER_SDA_IO CONFIG_IMU_I2C_SDA_IO
#define I2C_MASTER_SCL_IO CONFIG_IMU_I2C_SCL_IO
#define I2C_MASTER_FREQ_HZ I2C_MASTER_FREQ_HZ_400K
#define I2C_MASTER_RX_BUF_DISABLE 0
#define I2C_MASTER_TX_BUF_DISABLE 0

static EventGroupHandle_t s_imu_event_group;


static mpu6050_handle_t mpu6050 = NULL;
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;

#define TASK_STACK_SIZE 1024*10
#define TAG "BETA FLIGHT IMU"

volatile float twoKp = (2.0 * 0.246f);	// 2 * proportional gain 比例增益
volatile float twoKi = (2.0 * 0.00035f);	// 2 * integral gain     积分增益
volatile float sampleFreq = 200.0f;	    // sample frequency in Hz
volatile float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;   // quaternion of sensor frame relative to auxiliary frame
static float yaw, pitch, roll;

typedef void (*imu_data_callback_t)(
    mpu6050_raw_acce_value_t* mpu6050_raw_acce_value,
    mpu6050_raw_gyro_value_t* mpu6050_raw_gyro_value,
    int16_t mpu6050_temp_value);
static imu_data_callback_t imu_data_callback = NULL;

/**
 * @brief i2c master initialization
 */
static void i2c_bus_init(void)
{
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_I2C_ADDRESS,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
}

/**
 * @brief i2c master initialization
 */
static void i2c_sensor_mpu6050_init(void)
{
    esp_err_t ret;
    s_imu_event_group = xEventGroupCreate();

    i2c_bus_init();
    mpu6050 = mpu6050_create(dev_handle, MPU6050_I2C_ADDRESS);

    mpu6050_sample_rate(mpu6050, 200);
    while ((ret = mpu6050_config(mpu6050, ACCE_FS_2G, GYRO_FS_250DPS)) != ESP_OK) {
        printf("mpu6050 config ret=%d\n", ret);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    while ((ret = mpu6050_wake_up(mpu6050)) != ESP_OK) {
        printf("mpu6050 wake up ret=%d\n", ret);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

#define  RAD2DEG 57.29577951f
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
#define DEFAULT_CALIBRATION_ACCEL_DEADZONE  0.01f*0.01f
#define DEFAULT_CALIBRATION_GYRO_DEADZONE  0.01f*0.01f

struct Calibrate
{
    mpu6050_acce_value_t acce;
    mpu6050_gyro_value_t gyro;
} calibrate;

void calculateMean(uint32_t numsample, struct Calibrate* calibrate) {
    mpu6050_acce_value_t acce;
    mpu6050_gyro_value_t gyro;

    float pre_avg[7] = { 0 };    // 存储取样数量的和
    for (uint32_t i = 0; i < numsample;i++) {
        while (mpu6050_get_acce(mpu6050, &acce));
        while (mpu6050_get_gyro(mpu6050, &gyro));

        pre_avg[0] += (acce.acce_x - 1);
        pre_avg[1] += acce.acce_y;
        pre_avg[2] += acce.acce_z;
        pre_avg[3] += gyro.gyro_x;
        pre_avg[4] += gyro.gyro_y;
        pre_avg[5] += gyro.gyro_z;
        // pre_avg[6] += temp;
    }

    calibrate->acce.acce_x = (float)(pre_avg[0] / numsample);
    calibrate->acce.acce_y = (float)(pre_avg[1] / numsample);
    calibrate->acce.acce_z = (float)(pre_avg[2] / numsample);
    calibrate->gyro.gyro_x = (float)(pre_avg[3] / numsample);
    calibrate->gyro.gyro_y = (float)(pre_avg[4] / numsample);
    calibrate->gyro.gyro_z = (float)(pre_avg[5] / numsample);

}

void read_imu_calibrate(
    mpu6050_acce_value_t* acce,
    mpu6050_gyro_value_t* gyro,
    mpu6050_temp_value_t* temp
) {
    // 检查校准后的误差
    while (mpu6050_get_acce(mpu6050, acce));
    while (mpu6050_get_gyro(mpu6050, gyro));
    while (mpu6050_get_temp(mpu6050, temp));

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
    mpu6050_temp_value_t temp;

    for (int i = 0;i < 10;i++)
    {
        // 读取采用数平均值
        calculateMean(DEFAULT_CALIBRATION_NUMSAMPLES, &calibrate);
        vTaskDelay(200 / portTICK_PERIOD_MS);

        // 检查校准后的误差
        read_imu_calibrate(&acce, &gyro, &temp);

        if (acce.acce_z * acce.acce_z > DEFAULT_CALIBRATION_ACCEL_DEADZONE
            || acce.acce_y * acce.acce_y > DEFAULT_CALIBRATION_ACCEL_DEADZONE
            || (acce.acce_x - 1) * (acce.acce_x - 1) > DEFAULT_CALIBRATION_ACCEL_DEADZONE
            || gyro.gyro_x * gyro.gyro_x > DEFAULT_CALIBRATION_GYRO_DEADZONE
            || gyro.gyro_y * gyro.gyro_y > DEFAULT_CALIBRATION_GYRO_DEADZONE
            || gyro.gyro_z * gyro.gyro_z > DEFAULT_CALIBRATION_GYRO_DEADZONE
            )   continue;

        return 0;
    }
    return 1;
}


static void calculateVelocity(
    float acce_x,
    float acce_y,
    float acce_z,
    float pitch_rad,
    float roll_rad,
    float yaw_rad
) {
    static float velocity_x, velocity_y, velocity_z;

    rotate_vector(&acce_x, &acce_y, &acce_z, roll_rad, pitch_rad, yaw_rad);
    ESP_LOGI(TAG, "motionAcce:%.3f, %.3f, %.3f\n", acce_x, acce_y, acce_z);

#define g 9.8
    velocity_x += (acce_x * g / sampleFreq);
    velocity_y += (acce_y * g / sampleFreq);
    velocity_z += ((acce_z - 1.0) * g / sampleFreq);
    ESP_LOGI(TAG, "velocity:%.3f, %.3f, %.3f\n", velocity_x, velocity_y, velocity_z);

}

#define DATA_READY_BIT BIT0
#define ERROR_BIT      BIT1

static void imuTask(void* param) {
    esp_err_t ret;
    mpu6050_raw_acce_value_t mpu6050_raw_acce_value;
    mpu6050_raw_gyro_value_t mpu6050_raw_gyro_value;
    int16_t mpu6050_temp_value;

#ifdef CONFIG_IMU_INT_ENABLE
    while (1)
    {
        EventBits_t bits = xEventGroupWaitBits(s_imu_event_group,
            DATA_READY_BIT | ERROR_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

        uint8_t out_intr_status = 0;
        mpu6050_get_interrupt_status(mpu6050, &out_intr_status);
        ESP_LOGD(TAG, "imu read readed %d", out_intr_status);

        if (bits == DATA_READY_BIT && mpu6050_is_data_ready_interrupt(out_intr_status)) {
            ret = mpu6050_get_raw_data(
                mpu6050,
                &mpu6050_raw_acce_value,
                &mpu6050_raw_gyro_value,
                &mpu6050_temp_value
            );
            ESP_LOGD(TAG, "ret=%d,"
                "Accel: X=%6d, Y=%6d, Z=%6d\n", ret,
                mpu6050_raw_acce_value.raw_acce_x, mpu6050_raw_acce_value.raw_acce_y, mpu6050_raw_acce_value.raw_acce_z
            );
            if (imu_data_callback)
                imu_data_callback(
                    &mpu6050_raw_acce_value,
                    &mpu6050_raw_gyro_value,
                    mpu6050_temp_value
                );
        }
    }
#endif


    uint8_t mpu6050_deviceid = 0;
    mpu6050_acce_value_t acce;
    mpu6050_gyro_value_t gyro;
    mpu6050_temp_value_t temp;

    uint64_t now = 0, last_update = 0;
    // struct timeval tv_now;

    ret = mpu6050_get_deviceid(mpu6050, &mpu6050_deviceid);
    ESP_LOGI(TAG, "ret=%d, mpu6050_deviceid=0x%x\n", ret, mpu6050_deviceid);


    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }



    while (imu_calibrate());

    for (;;)
    {
        now = esp_timer_get_time();
        sampleFreq = (float)(1000000.0f / (now - last_update));
        last_update = now;

        read_imu_calibrate(&acce, &gyro, &temp);

        // ESP_LOGI(TAG, "imu:%.5f, %.5f, %.5f, %.5f, %.5f, %.5f, %.5f\n",
        //     acce.acce_x, acce.acce_y, acce.acce_z,
        //     gyro.gyro_x, gyro.gyro_y, gyro.gyro_z,
        //     temp.temp
        //     );
        // ESP_LOGI(TAG, "ret=%d, mpu6050 temp=%f\n", ret, temp.temp);
        // ESP_LOGI(TAG, "acce_x:%.2f, acce_y:%.2f, acce_z:%.2f\n", acce.acce_x, acce.acce_y, acce.acce_z);
        // ESP_LOGI(TAG, "gyro_x:%.2f, gyro_y:%.2f, gyro_z:%.2f\n", gyro.gyro_x, gyro.gyro_y, gyro.gyro_z);

        // char buffer[256];
        // size_t len = sprintf(buffer, "%s imu:%.5f, %.5f, %.5f, %.5f, %.5f, %.5f, %.5f, %.5f\n", TAG,
        //     acce.acce_x, acce.acce_y, acce.acce_z,
        //     gyro.gyro_x, gyro.gyro_y, gyro.gyro_z,
        //     temp.temp, sampleFreq
        // );
        // serialWrite(buffer, len);
        // vTaskDelay(10 / portTICK_PERIOD_MS);

        // continue;
        // update mahony imu
        MahonyAHRSupdateIMU(gyro.gyro_x / RAD2DEG, gyro.gyro_y / RAD2DEG, gyro.gyro_z / RAD2DEG, acce.acce_x, acce.acce_y, acce.acce_z);

        quaternion2Angle();
        // yaw += (gyro.gyro_z / sampleFreq);
        printf("roll:%.5f,pitch:%.5f,yaw:%.5f\n", roll, pitch, yaw);

        // calculateVelocity(acce.acce_x, acce.acce_y, acce.acce_z, pitch / RAD2DEG, roll / RAD2DEG, yaw / RAD2DEG);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

    }


}

static void IRAM_ATTR imu_isr_handler(void* arg)
{
    mpu6050_handle_t* mpu6050_handle = (mpu6050_handle_t*)arg;

    xEventGroupSetBits(s_imu_event_group, DATA_READY_BIT);
}

static TaskHandle_t xHandle = NULL;
void imuInit(void) {
    i2c_sensor_mpu6050_init();

#ifdef CONFIG_IMU_INT_ENABLE
    mpu6050_int_config_t mpu6050_int_config = {
        .interrupt_pin = CONFIG_IMU_INT_IO,
        .active_level = 0,//CONFIG_IMU_INT_LEVEL,
        .interrupt_clear_behavior = INTERRUPT_CLEAR_ON_STATUS_READ,//INTERRUPT_CLEAR_ON_ANY_READ,
        .interrupt_latch = INTERRUPT_LATCH_UNTIL_CLEARED,//INTERRUPT_LATCH_50US
        .pin_mode = INTERRUPT_PIN_OPEN_DRAIN,
    };
    mpu6050_config_interrupts(mpu6050, &mpu6050_int_config);
    mpu6050_register_isr(mpu6050, imu_isr_handler);
    mpu6050_enable_interrupts(mpu6050, MPU6050_DATA_RDY_INT_BIT);
#endif

    uint8_t out_intr_status = 0;
    mpu6050_get_interrupt_status(mpu6050, &out_intr_status);
}

void imuDeinit(void) {
    // 结束线程
    if (xHandle != NULL)
    {
        vTaskDelete(xHandle);
    }
    mpu6050_delete(mpu6050);

    esp_err_t ret = i2c_master_bus_rm_device(dev_handle);
    i2c_del_master_bus(bus_handle);
}

int imuStart(void* callback)
{
    imu_data_callback = callback;
    return xTaskCreate(imuTask, "imu_task", TASK_STACK_SIZE, NULL, 10, &xHandle);
}