import numpy as np

class GyroAttitudeEstimator:
    def __init__(self, initial_roll=0, initial_pitch=0, initial_yaw=0):
        """
        初始化姿态估计器
        参数:
            initial_roll: 初始横滚角(度)
            initial_pitch: 初始俯仰角(度)
            initial_yaw: 初始偏航角(度)
        """
        # 初始化姿态角(转换为弧度)
        self.roll = np.radians(initial_roll)
        self.pitch = np.radians(initial_pitch)
        self.yaw = np.radians(initial_yaw)
        
        # 上一次更新的时间戳
        self.last_time = None
    
    def update(self, gyro_data, current_time):
        """
        使用新的陀螺仪数据更新姿态估计
        参数:
            gyro_data: 包含x,y,z角速度的字典 {'gyro_x': , 'gyro_y': , 'gyro_z': }
            current_time: 当前时间戳(秒)
        """
        # 如果是第一次更新，只记录时间
        if self.last_time is None:
            self.last_time = current_time
            return
        
        # 计算时间差(秒)
        dt = current_time - self.last_time
        self.last_time = current_time
        
        # 获取角速度值(弧度/秒)
        gx = np.radians(gyro_data['gyro_x'])  # 绕X轴旋转角速度
        gy = np.radians(gyro_data['gyro_y'])  # 绕Y轴旋转角速度
        gz = np.radians(gyro_data['gyro_z'])  # 绕Z轴旋转角速度
        
        # 更新姿态角(积分)
        self.roll += gx * dt
        self.pitch += gy * dt
        self.yaw += gz * dt
        
        # 可选: 限制角度范围到[-π, π]
        self.roll = self._wrap_to_pi(self.roll)
        self.pitch = self._wrap_to_pi(self.pitch)
        self.yaw = self._wrap_to_pi(self.yaw)
    
    def get_attitude(self):
        """
        获取当前姿态角(度)
        返回:
            包含roll, pitch, yaw的字典(度)
        """
        return {
            'roll': np.degrees(self.roll),
            'pitch': np.degrees(self.pitch),
            'yaw': np.degrees(self.yaw)
        }
    
    def _wrap_to_pi(self, angle):
        """将角度限制在[-π, π]范围内"""
        while angle > np.pi:
            angle -= 2 * np.pi
        while angle < -np.pi:
            angle += 2 * np.pi
        return angle

# 示例使用
if __name__ == "__main__":
    # 初始化估计器(假设初始水平)
    estimator = GyroAttitudeEstimator()
    
    # 模拟数据(假设采样频率为100Hz)
    gyro_data = {
        'gyro_x': 0.5,  # 度/秒
        'gyro_y': 0.3,  # 度/秒
        'gyro_z': 1.0   # 度/秒
    }
    
    # 模拟更新循环
    for t in np.arange(0, 10, 0.01):  # 10秒，100Hz
        estimator.update(gyro_data, t)
        
        # 每1秒打印一次
        if t % 1.0 < 0.01:
            attitude = estimator.get_attitude()
            print(f"Time: {t:.1f}s - Roll: {attitude['roll']:.2f}°, Pitch: {attitude['pitch']:.2f}°, Yaw: {attitude['yaw']:.2f}°")