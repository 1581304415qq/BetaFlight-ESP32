import re
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import font_manager

# 设置中文字体
def set_chinese_font():
    try:
        # 尝试设置微软雅黑字体(Windows)
        # plt.rcParams['font.sans-serif'] = ['Microsoft YaHei']
        # 或者尝试设置黑体字体(Linux/Mac)
        plt.rcParams['font.sans-serif'] = ['Arial Unicode MS']
        plt.rcParams['axes.unicode_minus'] = False  # 解决负号显示问题
        print("中文字体设置成功")
    except:
        print("警告: 无法设置中文字体，可能会导致中文显示为方框")

def parse_mpu6050_data(file_path):
    # 初始化数据列表
    accel_x, accel_y, accel_z = [], [], []
    gyro_x, gyro_y, gyro_z = [], [], []
    temp = []
    tamp = []
    
    with open(file_path, 'r') as file:
        content = file.read()
        
    # 提取加速度计数据
    accel_pattern = r'Accel: X=\s*(-?\d+), Y=\s*(-?\d+), Z=\s*(-?\d+)'
    accel_matches = re.findall(accel_pattern, content)
    for x, y, z in accel_matches:
        accel_x.append(int(x))
        accel_y.append(int(y))
        accel_z.append(int(z))
    
    # 提取陀螺仪数据
    gyro_pattern = r'Gyro: X=\s*(-?\d+), Y=\s*(-?\d+), Z=\s*(-?\d+)'
    gyro_matches = re.findall(gyro_pattern, content)
    for x, y, z in gyro_matches:
        gyro_x.append(int(x))
        gyro_y.append(int(y))
        gyro_z.append(int(z))
    
    # 提取温度数据
    temp_pattern = r'Temp:\s*(-?\d+)'
    temp_matches = re.findall(temp_pattern, content)
    for t in temp_matches:
        temp.append(int(t))
    
    # 提取时间戳
    tamp_pattern = r'Tamp:\s*(-?\d+)'
    tamp_matches = re.findall(tamp_pattern, content)
    for t in tamp_matches:
        tamp.append(int(t))

    return {
        'accel_x': accel_x, 'accel_y': accel_y, 'accel_z': accel_z,
        'gyro_x': gyro_x, 'gyro_y': gyro_y, 'gyro_z': gyro_z,
        'temp': temp, 'tamp': tamp
    }

def moving_average(data, window_size):
    """对数据应用移动平均滤波"""
    weights = np.ones(window_size) / window_size
    return np.convolve(data, weights, mode='valid')

def apply_filters(data, window_size=5):
    """对所有数据应用滤波器"""
    filtered_data = {}
    
    # 对每个数据序列应用移动平均滤波
    for key, values in data.items():
        filtered_data[key] = moving_average(values, window_size)
    
    return filtered_data

def visualize_mpu6050_data(original_data, filtered_data):
    """可视化原始数据和滤波后的数据对比"""
    # 创建时间轴
    time_original = np.arange(len(original_data['accel_x']))
    time_filtered = np.arange(len(filtered_data['accel_x']))
    
    # 创建图形和子图（3行2列的布局）
    fig, axs = plt.subplots(4, 2, figsize=(15, 12))
    fig.set_size_inches(14, 8)  # 实时修改为8x6英寸‌:ml-citation{ref="5,6" data="citationList"}

    # 绘制加速度计数据对比
    # 原始数据
    axs[0, 0].plot(time_original, original_data['accel_x'], label='X轴')
    axs[0, 0].plot(time_original, original_data['accel_y'], label='Y轴')
    axs[0, 0].plot(time_original, original_data['accel_z'], label='Z轴')
    axs[0, 0].set_title('原始加速度计数据')
    axs[0, 0].set_ylabel('加速度 (原始值)')
    axs[0, 0].legend()
    axs[0, 0].grid(True)
    
    # 滤波后数据
    axs[0, 1].plot(time_filtered, filtered_data['accel_x'], label='X轴')
    axs[0, 1].plot(time_filtered, filtered_data['accel_y'], label='Y轴')
    axs[0, 1].plot(time_filtered, filtered_data['accel_z'], label='Z轴')
    axs[0, 1].set_title('滤波后加速度计数据')
    axs[0, 1].set_ylabel('加速度 (滤波值)')
    axs[0, 1].legend()
    axs[0, 1].grid(True)
    
    # 绘制陀螺仪数据对比
    # 原始数据
    axs[1, 0].plot(time_original, original_data['gyro_x'], label='X轴')
    axs[1, 0].plot(time_original, original_data['gyro_y'], label='Y轴')
    axs[1, 0].plot(time_original, original_data['gyro_z'], label='Z轴')
    axs[1, 0].set_title('原始陀螺仪数据')
    axs[1, 0].set_ylabel('角速度 (原始值)')
    axs[1, 0].legend()
    axs[1, 0].grid(True)
    
    # 滤波后数据
    axs[1, 1].plot(time_filtered, filtered_data['gyro_x'], label='X轴')
    axs[1, 1].plot(time_filtered, filtered_data['gyro_y'], label='Y轴')
    axs[1, 1].plot(time_filtered, filtered_data['gyro_z'], label='Z轴')
    axs[1, 1].set_title('滤波后陀螺仪数据')
    axs[1, 1].set_ylabel('角速度 (滤波值)')
    axs[1, 1].legend()
    axs[1, 1].grid(True)
    
    # 绘制温度数据对比
    # 原始数据
    axs[2, 0].plot(time_original, original_data['temp'], color='red')
    axs[2, 0].set_title('原始温度数据')
    axs[2, 0].set_xlabel('样本点')
    axs[2, 0].set_ylabel('温度 (原始值)')
    axs[2, 0].grid(True)
    
    # 滤波后数据
    axs[2, 1].plot(time_filtered, filtered_data['temp'], color='red')
    axs[2, 1].set_title('滤波后温度数据')
    axs[2, 1].set_xlabel('样本点')
    axs[2, 1].set_ylabel('温度 (滤波值)')
    axs[2, 1].grid(True)
    
    # 时间戳
    # axs[3, 0].plot(time_original, original_data['tamp'], color='green')
    # axs[3, 0].set_title('时间戳')
    # axs[3, 0].set_xlabel('样本点')
    # axs[3, 0].grid(True)
    
    time_intervals = np.diff(original_data['tamp'])
    sampling_frequency = 1000000 / time_intervals
    
    # 采用频率
    axs[3, 0].plot(time_original[:-1], sampling_frequency, color='green')
    axs[3, 0].set_title('采样频率')
    axs[3, 0].grid(True)
    
    # 调整布局
    plt.tight_layout()
    plt.show()

# 主函数
def main():
    # 设置中文字体
    set_chinese_font()
    
    file_path = 'mpu6050_data.txt'  # 替换为你的文件路径
    window_size = 10  # 滤波窗口大小，可以根据需要调整

    try:
        original_data = parse_mpu6050_data(file_path)
        
        # 应用滤波
        filtered_data = apply_filters(original_data, window_size)
        
        visualize_mpu6050_data(original_data, filtered_data)
    except Exception as e:
        print(f"处理数据时出错: {e}")

if __name__ == "__main__":
    main()