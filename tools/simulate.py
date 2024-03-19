import matplotlib.pyplot as plt
import math
import re
import numpy as np

# 全局变量
twoKp = 2.0 * 2.46  # 2 * 比例增益
twoKi = 2.0 * 0.0003  # 2 * 积分增益
sample_freq = 200.0  # 采样频率(Hz)

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
        halfex = (ay * halfvz - az * halfvy)
        halfey = (az * halfvx - ax * halfvz)
        halfez = (ax * halfvy - ay * halfvx)

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
    recip_norm = math.sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3)
    q0 /= recip_norm
    q1 /= recip_norm
    q2 /= recip_norm
    q3 /= recip_norm
    
RAD2DEG = 57.29577951
pitch = 0.0
roll  = 0.0
yaw   = 0.0
def quaternion2euler():
    global yaw, rool, pitch, q0, q1, q2, q3
    
    # yaw = math.atan2(2 * q1 * q2 - 2 * q0 * q3, 2 * q0 * q0 + 2 * q1 * q1 - 1) * RAD2DEG
    # pitch = - math.asin(2 * q1 * q3 + 2 * q0 * q2) * RAD2DEG
    # roll = math.atan2(2 * q2 * q3 - 2 * q0 * q1, 2 * q0 * q0 + 2 * q3 * q3 - 1) * RAD2DEG
    roll = math.atan2(2 * q0 * q1 + 2 * q2 * q3, 1 - 2 * q1 * q1 - 2 * q2 * q2) * RAD2DEG
    pitch = math.asin(2 * q0 * q2 - 2 * q3 * q1) * RAD2DEG
    yaw = math.atan2(2 * q0 * q3 + 2 * q1 * q2, 1 - 2 * q2 * q2 - 2 * q3 * q3) * RAD2DEG
    # yaw = -math.atan2(2.0f * (q1 * q2 + q0 * q3), q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3) * RAD2DEG
    # pitch = math.asin(2.0f * (q1 * q3 - q0 * q2)) * RAD2DEG
    # roll = math.atan2(2.0f * (q0 * q1 + q2 * q3), q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3) * RAD2DEG

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
 
def generateData():
    result={}
    with open('./log/foo.txt') as fo:
        lines = fo.readlines()
        for line in lines:
            # print(line)
            pattern = r'BETA FLIGHT IMU: ([^:]+):'
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
            # else:
            #     print("No match found")
        fo.close()
    return result

# 创建图表
fig, ax = plt.subplots()
lines={}
line_data=[]
def main():
    global sample_freq, q0, q1, q2, q3, lines

    data = generateData()
    for key in data.keys():
        print(len(data[key]))

    # lines['imu'].set_data()

    # for value in data['imu']:
    #     print(value)
    
    draw_data=[]
    for index, value in enumerate(data['imu']):
        sample_freq = float(data['sampleFreq'][index])
        data_list = list(map(float, value.split(', ')))
        # print(sample_freq, data_list)
        draw_data.append(data_list)
        
        mahony_ahrs_update_imu(data_list[3]/RAD2DEG, data_list[4]/RAD2DEG, data_list[5]/RAD2DEG, data_list[0], data_list[1], data_list[2])
        # print(q0,q1,q2,q3)
        
        quaternion2euler()
        # roll, pitch, yaw = quaternion_to_euler(q0,q1,q2,q3)
        
        print(data['angle'][index])
        print(yaw, pitch,roll)
        
    data = np.array(draw_data)
    data = data[:,2].tolist()
    # print(data)
    # ax.plot(range(len(data)), data, lw=1, label=key)
    # ax.set_ylim(min(data),max(data))  # 根据需要调整y轴范围
    # print(min(data),max(data))

main()

ax.set_title('Real-time Data Visualization')
ax.set_xlabel('Sample')
ax.set_ylabel('Value')
# plt.show()
