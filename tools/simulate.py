#!/usr/local/bin/python3

import matplotlib.pyplot as plt
import math
import re
import numpy as np
import matplotlib.lines as mlines



# 打印矩阵
def print_matrix(m):
    for i in range(m.shape[0]):
        for j in range(m.shape[1]):
            print(f"{m[i, j]:.2f}", end=" ")
        print()
    print()

# 矩阵加法
def mat_add(a, b):
    if a.shape != b.shape:
        return None
    return a + b

# 矩阵乘法
def mat_mul(a, b):
    if a.shape[1] != b.shape[0]:
        return None
    return a @ b
# def mat_mul(a, b):
#     if len(a[0]) != len(b):
#         return None
#     c = [[0 for j in range(len(b[0]))] for i in range(len(a))]
#     for i in range(len(a)):
#         for j in range(len(b[0])):
#             for k in range(len(a[0])):
#                 c[i][j] += a[i][k] * b[k][j]
#     return c

# 绕 X 轴旋转矩阵
def rotate_x(theta):
    rx = np.array([[1, 0, 0],
                   [0, math.cos(theta), -math.sin(theta)],
                   [0, math.sin(theta), math.cos(theta)]])
    return rx

# 绕 Y 轴旋转矩阵
def rotate_y(theta):
    ry = np.array([[math.cos(theta), 0, math.sin(theta)],
                   [0, 1, 0],
                   [-math.sin(theta), 0, math.cos(theta)]])
    return ry

# 绕 Z 轴旋转矩阵
def rotate_z(theta):
    rz = np.array([[math.cos(theta), -math.sin(theta), 0],
                   [math.sin(theta), math.cos(theta), 0],
                   [0, 0, 1]])
    return rz

# 3D 向量旋转
def rotate_vector(x, y, z, theta_x, theta_y, theta_z):
    rx = rotate_x(theta_x)
    ry = rotate_y(theta_y)
    rz = rotate_z(theta_z)
    r = mat_mul(mat_mul(rz, ry), rx)
    v = np.array([[x], [y], [z]])
    res = mat_mul(r, v)
    return res[0][0], res[1][0], res[2][0]

RAD2DEG = 57.27272727 #57.29577951
ALPHA = 0.99
now = 0
# 互补滤波
def mpu6050_complimentory_filter(counter, sample_freq, acce_value, gyro_value, complimentary_angle):
    acce_angle = [0,0]
    gyro_angle = [0,0]
    gyro_rate  = [0,0]

    if counter == 0:
        acce_angle[0] = math.atan2(acce_value[1], acce_value[2]) * RAD2DEG
        acce_angle[1] = math.atan2(acce_value[0], acce_value[2]) * RAD2DEG
        return acce_angle[0], acce_angle[1]

    dt = 1/sample_freq

    acce_angle[0] = math.atan2(acce_value[1], acce_value[2]) * RAD2DEG
    acce_angle[1] = math.atan2(acce_value[0], acce_value[2]) * RAD2DEG

    gyro_rate[0] = gyro_value[0]
    gyro_rate[1] = gyro_value[1]
    gyro_angle[0] = gyro_rate[0] * dt
    gyro_angle[1] = gyro_rate[1] * dt

    roll = ALPHA * (complimentary_angle[0] + gyro_angle[0]) + (1 - ALPHA) * acce_angle[0]
    pitch= ALPHA * (complimentary_angle[1] + gyro_angle[1]) + (1 - ALPHA) * acce_angle[1]

    return roll, pitch



# 全局变量
twoKp = 2.0 * 0.283  # 2 * 比例增益
twoKi = 2.0 * 0.00225  # 2 * 积分增益
sample_freq = 100.0  # 采样频率(Hz)

q0 = 1.0
q1 = 0.0
q2 = 0.0
q3 = 0.0  # 传感器坐标系相对于辅助坐标系的四元数

integral_fbx = 0.0
integral_fby = 0.0
integral_fbz = 0.0  # 经过Ki缩放的积分误差项

# def math.sqrt(x):
    # half_x = 0.5 * x
    # y = x
    # i = int.from_bytes(y.to_bytes(4, 'little'), byteorder='little')
    # i = 0x5f3759df - (i >> 1)
    # y = i.to_bytes(4, 'little')
    # y = struct.unpack('f', y)[0]
    # y = y * (1.5 - (half_x * y * y))
    # return y

def mahony_ahrs_update(gx, gy, gz, ax, ay, az, mx, my, mz):
    global q0, q1, q2, q3, integral_fbx, integral_fby, integral_fbz

    # 如果磁力计测量值无效，使用IMU算法
    if mx == 0.0 and my == 0.0 and mz == 0.0:
        mahony_ahrs_update_imu(gx, gy, gz, ax, ay, az)
        return

    # 只有在加速度计测量值有效时才计算反馈
    if not (ax == 0.0 and ay == 0.0 and az == 0.0):
        # 归一化加速度计测量值
        recip_norm = 1.0 / math.sqrt(ax * ax + ay * ay + az * az)
        ax *= recip_norm
        ay *= recip_norm
        az *= recip_norm

        # 归一化磁力计测量值
        recip_norm = 1.0 / math.sqrt(mx * mx + my * my + mz * mz)
        mx *= recip_norm
        my *= recip_norm
        mz *= recip_norm

        # 辅助变量，避免重复计算
        q0q0 = q0 * q0
        q0q1 = q0 * q1
        q0q2 = q0 * q2
        q0q3 = q0 * q3
        q1q1 = q1 * q1
        q1q2 = q1 * q2
        q1q3 = q1 * q3
        q2q2 = q2 * q2
        q2q3 = q2 * q3
        q3q3 = q3 * q3

        # 地球磁场的参考方向
        hx = 2.0 * (mx * (0.5 - q2q2 - q3q3) + my * (q1q2 - q0q3) + mz * (q1q3 + q0q2))
        hy = 2.0 * (mx * (q1q2 + q0q3) + my * (0.5 - q1q1 - q3q3) + mz * (q2q3 - q0q1))
        bx = math.sqrt(hx * hx + hy * hy)
        bz = 2.0 * (mx * (q1q3 - q0q2) + my * (q2q3 + q0q1) + mz * (0.5 - q1q1 - q2q2))

        # 重力和磁场的估计方向
        halfvx = q1q3 - q0q2
        halfvy = q0q1 + q2q3
        halfvz = q0q0 - 0.5 + q3q3
        halfwx = bx * (0.5 - q2q2 - q3q3) + bz * (q1q3 - q0q2)
        halfwy = bx * (q1q2 - q0q3) + bz * (q0q1 + q2q3)
        halfwz = bx * (q0q2 + q1q3) + bz * (0.5 - q1q1 - q2q2)

        # 误差是场向量的估计方向与测量方向的叉积和
        halfex = (ay * halfvz - az * halfvy) + (my * halfwz - mz * halfwy)
        halfey = (az * halfvx - ax * halfvz) + (mz * halfwx - mx * halfwz)
        halfez = (ax * halfvy - ay * halfvx) + (mx * halfwy - my * halfwx)

        # 如果启用，计算并应用积分反馈
        if twoKi > 0.0:
            integral_fbx += twoKi * halfex * (1.0 / sample_freq)
            integral_fby += twoKi * halfey * (1.0 / sample_freq)
            integral_fbz += twoKi * halfez * (1.0 / sample_freq)
            gx += integral_fbx
            gy += integral_fby
            gz += integral_fbz
        else:
            integral_fbx = 0.0
            integral_fby = 0.0
            integral_fbz = 0.0

        # 应用比例反馈
        gx += twoKp * halfex
        gy += twoKp * halfey
        gz += twoKp * halfez

    # 积分四元数变化率
    gx *= 0.5 * (1.0 / sample_freq)
    gy *= 0.5 * (1.0 / sample_freq)
    gz *= 0.5 * (1.0 / sample_freq)
    qa = q0
    qb = q1
    qc = q2
    q0 += (-qb * gx - qc * gy - q3 * gz)
    q1 += (qa * gx + qc * gz - q3 * gy)
    q2 += (qa * gy - qb * gz + q3 * gx)
    q3 += (qa * gz + qb * gy - qc * gx)

    # 归一化四元数
    recip_norm = 1.0 / math.sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3)
    q0 *= recip_norm
    q1 *= recip_norm
    q2 *= recip_norm
    q3 *= recip_norm

def mahony_ahrs_update_imu(gx, gy, gz, ax, ay, az):
    global q0, q1, q2, q3, integral_fbx, integral_fby, integral_fbz

    # 只有在加速度计测量值有效时才计算反馈
    if not (ax == 0.0 and ay == 0.0 and az == 0.0):
        # 归一化加速度计测量值
        recip_norm = math.sqrt(ax * ax + ay * ay + az * az)
        ax /= recip_norm
        ay /= recip_norm
        az /= recip_norm

        # 重力的估计方向和垂直于磁通量的向量
        halfvx = q1 * q3 - q0 * q2
        halfvy = q0 * q1 + q2 * q3
        halfvz = q0 * q0 - 0.5 + q3 * q3

        # 误差是重力的估计方向与测量方向的叉积和
        halfex = ay * halfvz - az * halfvy
        halfey = az * halfvx - ax * halfvz
        halfez = ax * halfvy - ay * halfvx

        # 如果启用，计算并应用积分反馈
        if twoKi > 0.0:
            integral_fbx += twoKi * halfex * (1.0 / sample_freq)
            integral_fby += twoKi * halfey * (1.0 / sample_freq)
            integral_fbz += twoKi * halfez * (1.0 / sample_freq)
            gx += integral_fbx
            gy += integral_fby
            gz += integral_fbz
        else:
            integral_fbx = 0.0
            integral_fby = 0.0
            integral_fbz = 0.0

        # 应用比例反馈
        gx += twoKp * halfex
        gy += twoKp * halfey
        gz += twoKp * halfez

    # 积分四元数变化率
    gx *= (0.5 * (1.0 / sample_freq))
    gy *= (0.5 * (1.0 / sample_freq))
    gz *= (0.5 * (1.0 / sample_freq))
    qa = q0
    qb = q1
    qc = q2
    q0 += (-qb * gx - qc * gy - q3 * gz)
    q1 += (qa * gx + qc * gz - q3 * gy)
    q2 += (qa * gy - qb * gz + q3 * gx)
    q3 += (qa * gz + qb * gy - qc * gx)

    # 归一化四元数
    recip_norm = math.sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3)
    q0 /= recip_norm
    q1 /= recip_norm
    q2 /= recip_norm
    q3 /= recip_norm
    
pitch = 0.0
roll  = 0.0
yaw   = 0.0
def quaternion2euler():
    global yaw, roll, pitch, q0, q1, q2, q3
    
    # yaw = math.atan2(2 * q1 * q2 - 2 * q0 * q3, 2 * q0 * q0 + 2 * q1 * q1 - 1)
    # pitch = - math.asin(2 * q1 * q3 + 2 * q0 * q2)
    # roll = math.atan2(2 * q2 * q3 - 2 * q0 * q1, 2 * q0 * q0 + 2 * q3 * q3 - 1)
    roll = math.atan2(2 * q0 * q1 + 2 * q2 * q3, 1 - 2 * q1 * q1 - 2 * q2 * q2)
    pitch = math.asin(2 * q0 * q2 - 2 * q3 * q1)
    yaw = math.atan2(2 * q0 * q3 + 2 * q1 * q2, 1 - 2 * q2 * q2 - 2 * q3 * q3)
    # yaw = -math.atan2(2.0f * (q1 * q2 + q0 * q3), q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3)
    # pitch = math.asin(2.0f * (q1 * q3 - q0 * q2))
    # roll = math.atan2(2.0f * (q0 * q1 + q2 * q3), q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3)

def quaternion_to_euler(q0, q1, q2, q3):
    """
    将四元数转换为欧拉角(roll、pitch、yaw)
    输入参数:
        q0, q1, q2, q3: 四元数的四个分量
    返回值:
        roll, pitch, yaw: 欧拉角(单位: 弧度)
    """
    # 计算roll(x-axis rotation)
    sinr_cosp = 2 * (q0 * q1 + q2 * q3)
    cosr_cosp = 1 - 2 * (q1 * q1 + q2 * q2)
    roll = math.atan2(sinr_cosp, cosr_cosp)
    
    # 计算pitch(y-axis rotation)
    sinp = 2 * (q0 * q2 - q3 * q1)
    if abs(sinp) >= 1:
        pitch = math.copysign(math.pi / 2, sinp)  # 使用90度或-90度
    else:
        pitch = math.asin(sinp)
    
    # 计算yaw(z-axis rotation)
    siny_cosp = 2 * (q0 * q3 + q1 * q2)
    cosy_cosp = 1 - 2 * (q2 * q2 + q3 * q3)
    yaw = math.atan2(siny_cosp, cosy_cosp)
    
    return roll, pitch, yaw
 
filename = './log/2024-03-23 09:37:31.txt'
def generateData():
    result={}
    with open(filename) as fo:
        lines = fo.readlines()
        for line in lines:
            # print(line)
            pattern = r'BETA FLIGHT IMU ([^:]+):'
            match = re.search(pattern, line)

            if match:
                key = match.group(1)
                # print(key)
                start_index = line.find(key)
                if(start_index == -1):
                    continue
                start_index += len(key)

                # 从"raw:"后面开始截取字符串
                raw_data = line[start_index+1:].rstrip()

                if key in result:
                    result[key].append(raw_data)
                else:
                    result[key] = [raw_data]
            else:
                print("No match found")
        fo.close()
    return result

selectDimensions="Velocity"

# labels=['yaw','pitch','roll']
labels=['acce_x','acce_y','acce_z']
# labels=['velocity_x','velocity_y','velocity_z']
# 创建图表
fig, ax = plt.subplots()
line_data = []
lines     = {}
def main():
    global sample_freq, q0, q1, q2, q3, lines, roll, pitch

    # 串口数据处理生成备用数据
    data = generateData()
    if len(data)<1:
        return
    for key in data.keys():
        print(len(data[key]))

    # lines['imu'].set_data()

    # for value in data['imu']:
    #     print(value)
    
    draw_data  = []
    draw_correction_data=[]
    velocity_x = 0
    velocity_y = 0
    velocity_z = 0
    
    distance_x = 0
    distance_y = 0
    distance_z = 0
    
    
    # 遍历校准后的数据
    for index, value in enumerate(data['imu']):
        # 数据切片
        # if(index<2000):
        #     continue
        # if(index>2555):
        #     break
        
        # 分割原始数据. 三轴加速度和三轴角速度
        data_list = list(map(float, value.split(', ')))
        # print(data_list)
        # draw_data.append(data_list)
        
        # 提取输出的采样率数值
        sample_freq = data_list[-1]
        # print('sampleFreq',sample_freq)
        
        # 姿态算法,计算姿态四元数
        mahony_ahrs_update_imu(data_list[3]/RAD2DEG, data_list[4]/RAD2DEG, data_list[5]/RAD2DEG, data_list[0], data_list[1], data_list[2])
        # print(q0,q1,q2,q3)
        # 四元数转欧拉角
        # quaternion2euler()
        roll, pitch, yaw = quaternion_to_euler(q0,q1,q2,q3)
        
        # roll, pitch = mpu6050_complimentory_filter(index, sample_freq, [data_list[0],data_list[1],data_list[2]],
        #                                            [data_list[3],data_list[4],data_list[5]],[roll, pitch])
        
        
        # draw_data.append([yaw, pitch, roll])
        # print('angle:',yaw, pitch, roll)

        
        # 提取输出的欧拉角数据
        # print(data['angle'][index])
        # angle = list(map(float, data['angle'][index].split(', ')))
        # draw_data.append(angle)
        
        # 提取输出计算的运动加速度
        # acce_list = list(map(float, data['motionAcce'][index].split(', ')))
        # draw_data.append(acce_list)
        
        # print(data['motionAcce'][index])
        # velocity_x += (acce_list[0] * 9.8 / sample_freq)
        # velocity_y += (acce_list[1] * 9.8 / sample_freq)
        # velocity_z += ((acce_list[2] - 0.9989182681980652) * 9.8 / sample_freq)

        # 使用计算后的姿态角,imu加速度转成当前坐标系下
        acce_x,acce_y,acce_z = rotate_vector( data_list[0], data_list[1], data_list[2],roll, pitch, yaw)
        # print("acce:",acce_x,acce_y,acce_z)
        # draw_data.append([acce_x,acce_y,acce_z])

        # 用运动加速度计算 X,Y,Z方向的速度
        velocity_x += (acce_x * 9.8 / sample_freq)
        velocity_y += (acce_y * 9.8 / sample_freq)
        velocity_z += ((acce_z - 1) * 9.8 / sample_freq)
        
        # print("velocity:",velocity_x,velocity_y,velocity_z)
        # draw_data.append([velocity_x, velocity_y, velocity_z])
        # draw_correction_data.append([velocity_x, velocity_y, velocity_z])
        # draw_correction_data.append([velocity_x-0.000067*index, velocity_y-0.000024*index, velocity_z-0.000449*index])
        
        # 计算位移
        # for velocity in draw_correction_data:
        #     distance_x += (acce_x / 2*(sample_freq*sample_freq))
        #     distance_y += (acce_y / 2*(sample_freq*sample_freq))
        #     distance_z += (acce_z / 2*(sample_freq*sample_freq))
        #     draw_data.append([distance_x, distance_y, distance_z])

    data = np.array(draw_data)
    data_correction = np.array(draw_correction_data)
    
    # 计算平均值
    for i in range(3):
        drawData = data[:,i]
        drawData_mean = np.mean(drawData)
        print(f"The average of drawData is: {drawData_mean}")
    
        # 对数据进行线性拟合
        a, b = np.polyfit(range(len(drawData)), drawData, 1)
        print("%d:y = %fx + (%f)" % (i,a,b))
    
    # 可视化数据
    minVal = -1
    maxVal = 1
    line_artists=[]
    for i in range(3):
        drawData = data[:,i].tolist()
        # drawData = data_correction[:,i].tolist()
        # 画线
        line, = ax.plot(range(len(drawData)), drawData, lw=1, label=key)
        # 为每条线条创建一个虚拟artist对象
        line_artist = mlines.Line2D([], [], color=line.get_color(), label=line.get_label())
        # 画点
        # ax.scatter(range(len(drawData)), drawData, s=2)
        line_artists.append(line_artist)
        temp= min(drawData)
        if minVal > temp:
            minVal = temp 
        temp= max(drawData)
        if maxVal < temp:
            maxVal = temp 
    
    plt.legend(handles=line_artists,labels=labels ,loc='best')
    ax.set_ylim(minVal,maxVal)  # 根据需要调整y轴范围
    # print(minVal,maxVal)

main()

ax.set_title('Real-time Data Visualization')
ax.set_xlabel('Sample')
ax.set_ylabel(selectDimensions)
plt.show()
