#include <stdio.h>
#include <stdbool.h>

// PID 控制器参数
#define KP 0.6  // 比例增益
#define KI 0.2  // 积分增益
#define KD 0.1  // 微分增益

// PID 控制器状态
float error_prior = 0;  // 上一次误差
float integral = 0;     // 积分项

// 目标姿态
float target_angle = 0; // 目标姿态角度

// 获取当前姿态角度的函数（模拟）
float get_current_angle() {
    // 这里模拟返回当前的姿态角度
    return 10.0; // 用一个固定值代替
}

// PID 控制器函数
float pid_controller(float current_angle) {
    float error = target_angle - current_angle; // 计算当前误差

    // 计算 PID 控制输出
    float proportional = KP * error;
    integral += error; // 累积误差
    float derivative = KD * (error - error_prior);
    float output = proportional + KI * integral + derivative;

    // 保存当前误差，以备下一次使用
    error_prior = error;

    return output;
}



#define CONCAT_STRING(str, num) (str # num)


int main() {
    float current_angle = 0; // 当前姿态角度

    // 模拟飞行器在一段时间内的运行
    for (int i = 0; i < 100; i++) {
        current_angle = get_current_angle(); // 获取当前姿态角度

        // 使用 PID 控制器计算输出
        float control_output = pid_controller(current_angle);

        // 模拟应用输出控制信号的过程
        // 在实际应用中，此处会发送控制信号给飞行器
        printf("Control Output: %.2f\n", control_output);

        // 模拟飞行器姿态调整过程
        // 在实际应用中，此处会根据控制输出调整飞行器的姿态
        // 这里简单地将当前角度增加控制输出作为新的姿态角度
        current_angle += control_output;

        // 在实际应用中，还需要限制姿态角度的范围，避免超出可控制范围
    }

    
    int num = 42;
    char* combined_str = CONCAT_STRING("Hello, ", num);

    char key[25]={0};
    sprintf(key, "nvs_%d", num);
    printf("%s, %s\n", combined_str, key);

    return 0;
}
