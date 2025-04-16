import numpy as np

class MahonyAHRS:
    def __init__(self, Kp=1.0, Ki=0.0):
        """
        Mahony AHRS 算法初始化
        :param Kp: 比例增益（误差修正强度）
        :param Ki: 积分增益（积分误差修正强度）
        """
        self.Kp = Kp
        self.Ki = Ki
        self.quaternion = np.array([1.0, 0.0, 0.0, 0.0])  # 初始四元数 [w, x, y, z]
        self.integral_error = np.array([0.0, 0.0, 0.0])    # 积分误差项

    def update(self, gyro, accel, mag, dt):
        """
        更新姿态
        :param gyro: 陀螺仪数据 (rad/s) [wx, wy, wz]
        :param accel: 加速度计数据 (m/s²) [ax, ay, az]
        :param mag: 磁力计数据 (μT) [mx, my, mz]
        :param dt: 时间步长 (s)
        """
        # 归一化加速度计和磁力计数据
        accel = self._normalize(accel)
        mag = self._normalize(mag)

        # 从四元数中提取旋转矩阵元素
        q0, q1, q2, q3 = self.quaternion

        # ------------------- 加速度计误差计算 -------------------
        # 估计的重力方向（基于当前四元数）
        gravity_estimated = np.array([
            2*(q1*q3 - q0*q2),
            2*(q0*q1 + q2*q3),
            q0**2 - q1**2 - q2**2 + q3**2
        ])

        # 加速度计测量方向与估计方向的误差（叉乘）
        error_accel = np.cross(accel, gravity_estimated)

        # ------------------- 磁力计误差计算 -------------------
        # 将磁力计数据转换到地球坐标系
        h = np.array([
            mag[0]*(q0**2 + q1**2 - q2**2 - q3**2) + 2*mag[1]*(q1*q2 - q0*q3) + 2*mag[2]*(q1*q3 + q0*q2),
            2*mag[0]*(q1*q2 + q0*q3) + mag[1]*(q0**2 - q1**2 + q2**2 - q3**2) + 2*mag[2]*(q2*q3 - q0*q1),
            2*mag[0]*(q1*q3 - q0*q2) + 2*mag[1]*(q0*q1 + q2*q3) + mag[2]*(q0**2 - q1**2 - q2**2 + q3**2)
        ])

        # 磁力计方向的参考向量（假设磁场水平向北）
        b = np.array([np.sqrt(h[0]**2 + h[1]**2), 0, h[2]])

        # 磁力计方向误差
        error_mag = np.cross(h, b)

        # ------------------- 误差融合与积分 -------------------
        total_error = error_accel + error_mag
        self.integral_error += total_error * self.Ki * dt

        # ------------------- 修正陀螺仪偏差 -------------------
        # 陀螺仪数据 + 比例项 + 积分项
        gyro_corrected = gyro + self.Kp * total_error + self.integral_error

        # ------------------- 四元数更新 -------------------
        # 四元数微分方程
        q_dot = 0.5 * np.array([
            -q1*gyro_corrected[0] - q2*gyro_corrected[1] - q3*gyro_corrected[2],
            q0*gyro_corrected[0] + q2*gyro_corrected[2] - q3*gyro_corrected[1],
            q0*gyro_corrected[1] - q1*gyro_corrected[2] + q3*gyro_corrected[0],
            q0*gyro_corrected[2] + q1*gyro_corrected[1] - q2*gyro_corrected[0]
        ])

        # 积分四元数（使用欧拉法）
        self.quaternion += q_dot * dt
        self.quaternion = self._normalize(self.quaternion)

    def _normalize(self, vector):
        """归一化向量"""
        norm = np.linalg.norm(vector)
        if norm == 0:
            return vector
        return vector / norm

    def get_euler_angles(self):
        """从四元数计算欧拉角（滚转、俯仰、偏航）"""
        q0, q1, q2, q3 = self.quaternion
        roll = np.arctan2(2*(q0*q1 + q2*q3), 1 - 2*(q1**2 + q2**2))
        pitch = np.arcsin(2*(q0*q2 - q3*q1))
        yaw = np.arctan2(2*(q0*q3 + q1*q2), 1 - 2*(q2**2 + q3**2))
        return np.degrees(roll), np.degrees(pitch), np.degrees(yaw)
    
