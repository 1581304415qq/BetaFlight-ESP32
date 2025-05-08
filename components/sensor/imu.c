#include "imu.h"
#include "imu_types.h"
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

#define I2C_MASTER_FREQ_HZ_MAX (1250000)   // 1.25 MHz
#define I2C_MASTER_FREQ_HZ_1250K (1250000) // 1.25 MHz
#define I2C_MASTER_FREQ_HZ_1000K (1000000) // 1 MHz
#define I2C_MASTER_FREQ_HZ_400K (400000)   // 400 KHz
#define I2C_MASTER_FREQ_HZ_100K (100000)   // 100 KHz

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

#define TASK_STACK_SIZE 1024 * 10
#define TAG "BETA FLIGHT IMU"

volatile float twoKp = (2.0 * 0.246f);                     // 2 * proportional gain 比例增益
volatile float twoKi = (2.0 * 0.00035f);                   // 2 * integral gain     积分增益
volatile float sampleFreq = 200.0f;                        // sample frequency in Hz
volatile float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f; // quaternion of sensor frame relative to auxiliary frame

typedef void (*imu_data_callback_t)(
    mpu6050_raw_acce_value_t *mpu6050_raw_acce_value,
    mpu6050_raw_gyro_value_t *mpu6050_raw_gyro_value,
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

    while ((ret = mpu6050_wake_up(mpu6050)) != ESP_OK)
    {
        printf("mpu6050 wake up ret=%d\n", ret);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    while ((ret = mpu6050_config(mpu6050, ACCE_FS_2G, GYRO_FS_250DPS)) != ESP_OK)
    {
        printf("mpu6050 config ret=%d\n", ret);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    mpu6050_sample_rate(mpu6050, 50);
}

#define RAD2DEG 57.29577951f
static void quaternion2Angle(float *roll, float *pitch, float *yaw)
{
    // 计算欧拉角
    // *yaw = atan2(2 * q1 * q2 - 2 * q0 * q3, 2 * q0 * q0 + 2 * q1 * q1 - 1) * RAD2DEG;
    // *pitch = -asin(2 * q1 * q3 + 2 * q0 * q2) * RAD2DEG;
    // *roll = atan2(2 * q2 * q3 - 2 * q0 * q1, 2 * q0 * q0 + 2 * q3 * q3 - 1) * RAD2DEG;
    *roll = atan2(2 * q0 * q1 + 2 * q2 * q3, 1 - 2 * q1 * q1 - 2 * q2 * q2) * RAD2DEG;
    *pitch = asin(2 * q0 * q2 - 2 * q3 * q1) * RAD2DEG;
    *yaw = atan2(2 * q0 * q3 + 2 * q1 * q2, 1 - 2 * q2 * q2 - 2 * q3 * q3) * RAD2DEG;
    // *yaw = -atan2(2.0f * (q1 * q2 + q0 * q3), q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3) * RAD2DEG;
    // *pitch = asin(2.0f * (q1 * q3 - q0 * q2)) * RAD2DEG;
    // *roll = atan2(2.0f * (q0 * q1 + q2 * q3), q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3) * RAD2DEG;
}

// 默认取样个数
#define DEFAULT_CALIBRATION_NUMSAMPLES 1000
// 误差抖动 阈值
#define DEFAULT_CALIBRATION_ACCEL_DEADZONE 0.01f * 0.01f
#define DEFAULT_CALIBRATION_GYRO_DEADZONE 2000

enum IMU_STATE
{
    IMU_STATE_IDLE = 0,
    IMU_STATE_CALIBRATE,
    IMU_STATE_READ,
    IMU_STATE_PROCESS,
};

#define GYRO_NBR_OF_AXES 3
#define DEFAULT_CALIBRATION_NUMSAMPLES 1000
#define GYRO_MIN_BIAS_TIMEOUT_MS        M2T(1*1000)
int32_t varianceSampleTime;

typedef union
{
    struct
    {
        int16_t x;
        int16_t y;
        int16_t z;
    };
    int16_t axis[3];
} Axis3i16;
struct Calibrate
{
    Axis3i16 accel_buffer[DEFAULT_CALIBRATION_NUMSAMPLES];
    Axis3i16 gyro_buffer[DEFAULT_CALIBRATION_NUMSAMPLES];
    float acce_sensitivity;
    float gyro_sensitivity;
};

struct IMU
{
    enum IMU_STATE state;
    struct Calibrate *calibrate;
    mpu6050_acce_value_t bias_accel;
    mpu6050_gyro_value_t bias_gyro;

    mpu6050_raw_acce_value_t mpu6050_raw_acce_value;
    mpu6050_raw_gyro_value_t mpu6050_raw_gyro_value;
    int16_t mpu6050_temp_value;
    float yaw;
    float pitch;
    float roll;
} imu;

static void calcMean(Axis3i16 *sample, Axis3f *mean)
{
    int64_t sum[GYRO_NBR_OF_AXES] = {0};

    for (int i = 0; i < DEFAULT_CALIBRATION_NUMSAMPLES; i++)
    {
        sum[0] += sample[i].x;
        sum[1] += sample[i].y;
        sum[2] += sample[i].z;
    }

    mean->x = (float)sum[0] / DEFAULT_CALIBRATION_NUMSAMPLES;
    mean->y = (float)sum[1] / DEFAULT_CALIBRATION_NUMSAMPLES;
    mean->z = (float)sum[2] / DEFAULT_CALIBRATION_NUMSAMPLES;
}

/**
 * @brief 计算方差和均值
 * @param bias 偏差
 * @param variance 方差
 * @param mean 均值
 */
static void calcVarianceAndMean(Axis3i16 *bias, Axis3f *variance, Axis3f *mean)
{
    int64_t sumSquared[GYRO_NBR_OF_AXES] = {0};

    for (int i = 0; i < DEFAULT_CALIBRATION_NUMSAMPLES; i++)
    {
        sumSquared[0] += bias[i].x * bias[i].x;
        sumSquared[1] += bias[i].y * bias[i].y;
        sumSquared[2] += bias[i].z * bias[i].z;
    }
    calcMean(bias, mean);

    variance->x = fabs(sumSquared[0] / DEFAULT_CALIBRATION_NUMSAMPLES - mean->x * mean->x);
    variance->y = fabs(sumSquared[1] / DEFAULT_CALIBRATION_NUMSAMPLES - mean->y * mean->y);
    variance->z = fabs(sumSquared[2] / DEFAULT_CALIBRATION_NUMSAMPLES - mean->z * mean->z);
}
bool imu_calibrate(struct IMU *imu)
{
    // 读取采用数平均值
    Axis3f mean;
    calcMean(imu->calibrate->accel_buffer, &mean);
    mpu6050_get_acce_sensitivity(mpu6050, &imu->calibrate->acce_sensitivity);
    mpu6050_get_gyro_sensitivity(mpu6050, &imu->calibrate->gyro_sensitivity);
    imu->bias_accel.acce_x = (int16_t)(mean.x + 0.5f);
    imu->bias_accel.acce_y = (int16_t)(mean.y + 0.5f);
    imu->bias_accel.acce_z = (int16_t)(mean.z + 0.5f) - imu->calibrate->acce_sensitivity;


    Axis3f variance;
    calcVarianceAndMean(imu->calibrate->gyro_buffer, &variance, &mean);

    if (variance.x < DEFAULT_CALIBRATION_GYRO_DEADZONE
        && variance.y < DEFAULT_CALIBRATION_GYRO_DEADZONE
        && variance.z < DEFAULT_CALIBRATION_GYRO_DEADZONE
        // && (varianceSampleTime + GYRO_MIN_BIAS_TIMEOUT_MS < xTaskGetTickCount())
        )
      {
        varianceSampleTime = xTaskGetTickCount();
        imu->bias_gyro.gyro_x = (int16_t)(mean.x + 0.5f);
        imu->bias_gyro.gyro_y = (int16_t)(mean.y + 0.5f);
        imu->bias_gyro.gyro_z = (int16_t)(mean.z + 0.5f);
        return true;
      }
    return false;
}

static void calculateVelocity(
    float acce_x,
    float acce_y,
    float acce_z,
    float pitch_rad,
    float roll_rad,
    float yaw_rad)
{
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
#define ERROR_BIT BIT1

static void imuTask(void *param)
{
    esp_err_t ret;

    uint8_t mpu6050_deviceid = 0;
    ret = mpu6050_get_deviceid(mpu6050, &mpu6050_deviceid);
    ESP_LOGI(TAG, "ret=%d, mpu6050_deviceid=0x%x\n", ret, mpu6050_deviceid);

    uint32_t calibrate_count = 0;
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
        if (bits == DATA_READY_BIT && mpu6050_is_data_ready_interrupt(out_intr_status))
        {
            int ret = 0;
            // 使用一个状态机控制， 1，校准 2，读取数据 3，处理数据
            switch (imu.state)
            {
            case IMU_STATE_IDLE:
                // 采集数据校准需要数据
                ret = mpu6050_get_raw_data(
                    mpu6050,
                    &imu.calibrate->accel_buffer[calibrate_count],
                    &imu.calibrate->gyro_buffer[calibrate_count],
                    NULL);
                if (++calibrate_count == DEFAULT_CALIBRATION_NUMSAMPLES)
                {
                    calibrate_count = 0;
                    imu.state = IMU_STATE_CALIBRATE;
                    ESP_LOGI(TAG, "imu calibrate start");
                }
                break;
            case IMU_STATE_CALIBRATE:
                if (imu_calibrate(&imu) == 0)
                {
                    imu.state = IMU_STATE_READ;
                    ESP_LOGI(TAG, "imu calibrate success");
                }
                else
                {
                    imu.state = IMU_STATE_IDLE;
                    ESP_LOGI(TAG, "imu calibrate failed");
                }
                break;
            case IMU_STATE_READ:
                ret = mpu6050_get_raw_data(
                    mpu6050,
                    &imu.mpu6050_raw_acce_value,
                    &imu.mpu6050_raw_gyro_value,
                    &imu.mpu6050_temp_value);
                imu.mpu6050_raw_acce_value.raw_acce_x -= imu.bias_accel.acce_x;
                imu.mpu6050_raw_acce_value.raw_acce_y -= imu.bias_accel.acce_y;
                imu.mpu6050_raw_acce_value.raw_acce_z -= imu.bias_accel.acce_z;
                imu.mpu6050_raw_gyro_value.raw_gyro_x -= imu.bias_gyro.gyro_x;
                imu.mpu6050_raw_gyro_value.raw_gyro_y -= imu.bias_gyro.gyro_y;
                imu.mpu6050_raw_gyro_value.raw_gyro_z -= imu.bias_gyro.gyro_z;
                ESP_LOGD(TAG, "ret=%d,"
                              "Accel: X=%6d, Y=%6d, Z=%6d\n",
                         ret,
                         imu.mpu6050_raw_acce_value.raw_acce_x, imu.mpu6050_raw_acce_value.raw_acce_y, imu.mpu6050_raw_acce_value.raw_acce_z);

                if (imu_data_callback)
                    imu_data_callback(&imu.mpu6050_raw_acce_value, &imu.mpu6050_raw_gyro_value, imu.mpu6050_temp_value);
                break;
            case IMU_STATE_PROCESS:
                // MahonyAHRSupdateIMU(gyro.gyro_x / RAD2DEG, gyro.gyro_y / RAD2DEG, gyro.gyro_z / RAD2DEG, acce.acce_x, acce.acce_y, acce.acce_z);

                // quaternion2Angle();
                break;
            }
        }

#else
    mpu6050_acce_value_t acce;
    mpu6050_gyro_value_t gyro;
    mpu6050_temp_value_t temp;

    while (1)
    {
        ret = mpu6050_get_raw_data(
            mpu6050,
            &mpu6050_raw_acce_value,
            &mpu6050_raw_gyro_value,
            &mpu6050_temp_value);
        ESP_LOGD(TAG, "ret=%d,"
                      "Accel: X=%6d, Y=%6d, Z=%6d\n",
                 ret,
                 mpu6050_raw_acce_value.raw_acce_x, mpu6050_raw_acce_value.raw_acce_y, mpu6050_raw_acce_value.raw_acce_z);
        if (imu_data_callback)
            imu_data_callback(
                &mpu6050_raw_acce_value,
                &mpu6050_raw_gyro_value,
                mpu6050_temp_value);
        vTaskDelay((1000 / 50) / portTICK_PERIOD_MS);
    }

#endif
    }
}

static void IRAM_ATTR imu_isr_handler(void *arg)
{
    mpu6050_handle_t *mpu6050_handle = (mpu6050_handle_t *)arg;

    // 清除按键按下标志位，防止重复触发
    xEventGroupClearBitsFromISR(s_imu_event_group, DATA_READY_BIT);

    BaseType_t xHigherPriorityTaskWoken, xResult;
    xHigherPriorityTaskWoken = pdFALSE;

    xResult = xEventGroupSetBitsFromISR(s_imu_event_group, DATA_READY_BIT, &xHigherPriorityTaskWoken);
    if (xResult == pdPASS)
    {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

static TaskHandle_t xHandle = NULL;
void imuInit(void)
{
    imu.calibrate = malloc(sizeof(struct Calibrate));
    i2c_sensor_mpu6050_init();

#ifdef CONFIG_IMU_INT_ENABLE
    mpu6050_int_config_t mpu6050_int_config = {
        .interrupt_pin = CONFIG_IMU_INT_IO,
        .active_level = CONFIG_IMU_INT_LEVEL,
        .interrupt_clear_behavior = INTERRUPT_CLEAR_ON_STATUS_READ,
        .interrupt_latch = INTERRUPT_LATCH_UNTIL_CLEARED,
        .pin_mode = INTERRUPT_PIN_OPEN_DRAIN,
    };
    mpu6050_config_interrupts(mpu6050, &mpu6050_int_config);
    mpu6050_register_isr(mpu6050, imu_isr_handler);
    mpu6050_enable_interrupts(mpu6050, MPU6050_DATA_RDY_INT_BIT);
#endif

    uint8_t out_intr_status = 0;
    mpu6050_get_interrupt_status(mpu6050, &out_intr_status);
}

void imuDeinit(void)
{
    // 结束线程
    if (xHandle != NULL)
    {
        vTaskDelete(xHandle);
    }
    mpu6050_delete(mpu6050);

    esp_err_t ret = i2c_master_bus_rm_device(dev_handle);
    i2c_del_master_bus(bus_handle);
}

int imuStart(void *callback)
{
    imu_data_callback = callback;
    return xTaskCreate(imuTask, "imu_task", TASK_STACK_SIZE, NULL, 10, &xHandle);
}