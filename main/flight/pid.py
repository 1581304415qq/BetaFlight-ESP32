
import numpy as np
import matplotlib.pyplot as plt

def heater(temp, power, delta_t):
    """
    加热器模型
    temp: 当前温度
    power: 功率输入
    delta_t: 时间步长
    """
    alpha = 0.1  # 加热系数
    beta = 0.05  # 热损失系数
    temp_new = temp + alpha * power * delta_t - beta * temp * delta_t
    return temp_new
def pid_control(target, current, kp, ki, kd, integral, derivative, delta_t):
    """
    PID控制算法
    target: 目标温度
    current: 当前温度
    kp: 比例系数
    ki: 积分系数
    kd: 微分系数
    integral: 前一时刻积分值
    derivative: 前一时刻微分值
    delta_t: 时间步长
    """
    error = target - current
    integral = integral + error * delta_t
    derivative = (error - derivative) / delta_t
    power = kp * error + ki * integral + kd * derivative
    return power, integral, derivative

temp_initial = 20  # 初始温度
temp_target = 50  # 目标温度
temp = temp_initial  # 当前温度

"""
比例项(P): 根据实际姿态与目标姿态的差异,产生一个与误差成比例的输出信号。这个信号的作用是快速减小姿态误差,但可能导致超调和震荡。
积分项(I): 积分项用来消除静态误差,它根据误差的累积值产生一个输出信号,用于消除长期存在的误差。这有助于飞行器更快地达到目标姿态,但过大的积分项可能导致系统不稳定。
微分项(D): 微分项根据误差变化的速度产生一个输出信号,用于抑制姿态的快速变化,防止系统过冲或震荡。
"""

kp = 1.234  # 比例系数
ki = 0.36  # 积分系数
kd = 0.1  # 微分系数

integral = 0  # 初始积分值
derivative = 0  # 初始微分值

time_step = 1  # 时间步长
simulation_time = 100  # 模拟时间

temp_data = []  # 存储温度数据
power_data = []  # 存储功率数据
time_data = []  # 存储时间数据
for t in range(simulation_time):
    power, integral, derivative = pid_control(temp_target, temp, kp, ki, kd, integral, derivative, time_step)
    temp = heater(temp, power, time_step)
    temp_data.append(temp)
    power_data.append(power)
    time_data.append(t)
    
plt.figure(figsize=(10, 6))
plt.subplot(2, 1, 1)
plt.plot(time_data, temp_data)
plt.xlabel('Time')
plt.ylabel('Temperature')
plt.title('Temperature vs Time')

plt.subplot(2, 1, 2)
plt.plot(time_data, power_data)
plt.xlabel('Time')
plt.ylabel('Power')
plt.title('Power vs Time')

plt.tight_layout()
plt.show()