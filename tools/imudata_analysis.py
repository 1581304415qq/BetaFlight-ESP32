import re
import math
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import font_manager
from sklearn.preprocessing import PolynomialFeatures
from sklearn.linear_model import LinearRegression
from sklearn.pipeline import Pipeline
from sklearn.metrics import mean_squared_error, r2_score
import joblib  # 推荐用于大数据模型
from MahonyAHRS import MahonyAHRS 
from MahonyAHRS_NoMag import MahonyAHRS_NoMag 
from MadgwickAHRS import MadgwickAHRS
from GyroAttitudeEstimator import GyroAttitudeEstimator
from animate import show_animate
from scipy import signal
import time

RAD2DEG = 57.29578
axes = ['gyro_x', 'gyro_y', 'gyro_z','accel_x', 'accel_y', 'accel_z']
acce_sensitivity = 16384
gyro_sensitivity = 131
g                = 9.81

# calibration_gyro_models = 'models/calibration_models_50hz.pkl'
calibration_gyro_models = 'models/calibration_gyro_models_50hz.pkl'
calibration_accel_models = 'calibration_accel_models_50hz.pkl'

# 温度转换
def convertTemp(raw_data): 
    return {
        **raw_data,  # 保留原始数据中所有键值对
        'temp': raw_data['temp'] / 340.0 + 36.53  # 仅修改温度值
    }
# 加速度转换
def convertAccel(raw_data):
    data = {}
    data['accel_x'] = raw_data['accel_x'] / acce_sensitivity
    data['accel_y'] = raw_data['accel_y'] / acce_sensitivity
    data['accel_z'] = raw_data['accel_z'] / acce_sensitivity
    return {**raw_data,**data}
# 角速度转换
def convertGyro(raw_data): 
    data = {}
    data['gyro_x'] = raw_data['gyro_x'] / gyro_sensitivity
    data['gyro_y'] = raw_data['gyro_y'] / gyro_sensitivity
    data['gyro_z'] = raw_data['gyro_z'] / gyro_sensitivity
    return {**raw_data,**data}
    
# ========================
# 零偏校准核心算法
# ========================

# 建立二次多项式回归模型 y = β₀ + β₁·T + β₂·T²
def build_calibration_gyro_model(imu_readings):
    """
    为每个陀螺仪轴建立二次多项式校准模型
    
    参数:
    temperatures: 温度数据数组
    imu_readings: imu读数数组 ['gyro_x', 'gyro_y', 'gyro_z','accel_x', 'accel_y', 'accel_z']
    
    返回:
    models: 三个轴的校准模型
    """
    models = {}
    
    for i, axis in enumerate(['gyro_x', 'gyro_y', 'gyro_z']):
        # 创建二次多项式回归模型管道
        model = Pipeline([
            ('poly', PolynomialFeatures(degree=2)),
            ('linear', LinearRegression())
        ])
        
        # 训练模型
        X = imu_readings['temp'].reshape(-1, 1)
        y = imu_readings[axis]
        model.fit(X, y)
        
        models[axis] = model
    
    return models

def vectorized_calibrate_gyro(raw_data, models):
    """
    向量化版本的陀螺仪校准函数，一次处理整个数据集
    
    参数:
    temperatures: 温度数据数组 (n,)
    raw_data: 原始陀螺仪数据 ['gyro_x', 'gyro_y', 'gyro_z','accel_x', 'accel_y', 'accel_z']，每个是长度为n的数组
    models: 校准模型列表，长度为3，对应x、y、z轴
    
    返回:
    calibrated_data: 包含校准后数据的字典
    """
    calibrated_data = {
        'tamp': raw_data['tamp'],
        'temp': raw_data['temp'],
        'accel_x': raw_data['accel_x'],
        'accel_y': raw_data['accel_y'],
        'accel_z': raw_data['accel_z'],
    }
    
    # 将温度数据重塑为模型所需的形状
    temps_reshaped = raw_data['temp'].reshape(-1, 1)
    
    # 对每个轴分别预测偏差并应用校准
    for i, axis in enumerate(['gyro_x', 'gyro_y', 'gyro_z']):
        # 预测当前轴在所有温度下的偏差
        biases = models[axis].predict(temps_reshaped)
        
        # 从原始数据中减去偏差，得到校准后的数据
        calibrated_data[axis] = raw_data[axis] - biases

    return calibrated_data

'''
    z+ : 40:240
    z- : 470:670
    y+ : 1150:1350
    y- : 1680:1880
    x+ : 2360:2560
    x- : 3000:3200
'''
# [acc_x_meas, acc_y_meas, acc_z_meas] = K * [a_x_true, a_y_true, a_z_true] + b
# 6面法加速度零偏校准 
'''
    temperatures=[]
    imu_reading={'accel_x':[],'accel_y':[],'accel_z':[]}
'''
def build_calibration_accel_model(imu_reading):    
    # 六面法数据采集范围
    data_ranges = {
        'x+': [2360, 2560],
        'x-': [3000, 3200],
        'y+': [1150, 1350],
        'y-': [1680, 1880],
        'z+': [40, 240],
        'z-': [470, 670]
    }
    # 理论重力分量（六面法标准值）
    true_accelerations = np.array([
        [g, 0, 0],    # +X
        [-g, 0, 0],   # -X
        [0, g, 0],    # +Y
        [0, -g, 0],   # -Y
        [0, 0, g],    # +Z
        [0, 0, -g]    # -Z
    ])

    measured_accelerations = []
    for i, axis in enumerate(['x+', 'x-', 'y+', 'y-', 'z+', 'z-']):
        # 获取当前面的数据切片
        start, end = data_ranges[axis]
        # 计算各轴均值
        sample = [
            np.mean(imu_reading['accel_x'][start:end]),
            np.mean(imu_reading['accel_y'][start:end]),
            np.mean(imu_reading['accel_z'][start:end])
        ]
        measured_accelerations.append(sample)

    # print(measured_accelerations)
    # 使用线性回归拟合参数
    model = LinearRegression(fit_intercept=True)
    model.fit(true_accelerations, measured_accelerations)
    
    # 提取尺度因子矩阵和零偏
    K = model.coef_
    b = model.intercept_
    print({'K': K, 'b': b})

    return model


def calibrate_accel(raw_reading, model):
    """
    输入:
        acc_meas: 原始测量数组 (n, 3)
        K: 尺度因子矩阵 (3x3)
        b: 零偏向量 (3,)
    输出:
        acc_calibrated: 校准后数组 (n, 3)
    """
    calibrated_data = {
        'tamp': raw_reading['tamp'],
        'temp': raw_reading['temp'],
        'gyro_x': raw_reading['gyro_x'],
        'gyro_y': raw_reading['gyro_y'],
        'gyro_z': raw_reading['gyro_z'],
    }
    # 转换为二维数组 (n_samples, 3)
    acc_meas = np.column_stack([
        raw_reading['accel_x'],
        raw_reading['accel_y'],
        raw_reading['accel_z']
    ])
    # 提取参数矩阵
    K = model.coef_     # 3x3 尺度因子矩阵
    b = model.intercept_ # 零偏向量 (3,)

    # 矩阵求逆（需验证K的条件数）
    K_inv = np.linalg.inv(K)
    # 批量校准计算
    caba = (acc_meas - b) @ K_inv.T  # 等价于 K_inv @ (acc_meas - b).T

    calibrated_data['accel_x'] = caba[:, 0]
    calibrated_data['accel_y'] = caba[:, 1]
    calibrated_data['accel_z'] = caba[:, 2]

    return calibrated_data

def calibrate_accel_bias(raw_data):
    #返回的 calibrated_data 数据结构与原数据相同
    calibrated_data={}
    """执行零偏校准并返回校准后的数据"""
    for i, axis in enumerate(['accel_x', 'accel_y', 'accel_z']):
        # 计算零偏（平均值）
        bias = np.mean(raw_data[axis])
        
        # 应用校准
        calibrated_data[axis] = raw_data[axis] - bias
        if axis == 'accel_z':
           calibrated_data[axis] = calibrated_data[axis] + 1
    
    return {**raw_data,**calibrated_data}

# 获取X轴模型表达式、
def display_model_expressions(models, precision=6):
    """
    显示scikit-learn多项式回归模型的数学表达式
    
    参数:
    models: 训练好的scikit-learn Pipeline模型列表
    precision: 系数显示的小数位数
    
    返回:
    expressions: 包含模型表达式的字典
    """
    expressions = {}
    axes = ['gyro_x', 'gyro_y', 'gyro_z','accel_x', 'accel_y', 'accel_z']
    
    print("\n校准模型数学表达式:")
    print("=" * 50)
    
    for i, axis in enumerate(axes):
        if i >= len(models):
            print(f"错误: 没有{axis}轴的模型")
            continue
            
        model = models[axis]
        
        # 从管道中提取多项式特征和线性回归组件
        if hasattr(model, 'named_steps'):
            # 对于Pipeline对象
            try:
                poly = model.named_steps['polynomialfeatures']
                lr = model.named_steps['linearregression']
            except KeyError:
                # 尝试其他可能的键名（取决于如何创建管道）
                poly_key = [k for k in model.named_steps.keys() if 'poly' in k.lower()][0]
                lr_key = [k for k in model.named_steps.keys() if 'linear' in k.lower() or 'regress' in k.lower()][0]
                poly = model.named_steps[poly_key]
                lr = model.named_steps[lr_key]
        else:
            print(f"错误: {axis}轴模型不是Pipeline对象")
            continue
        
        # 获取系数和截距
        coeffs = lr.coef_
        intercept = lr.intercept_
        
        # 获取特征名称（对于二次多项式，应该是 [1, x, x^2]）
        try:
            # scikit-learn 1.0+
            feature_names = poly.get_feature_names_out(['T'])
        except AttributeError:
            try:
                # 旧版scikit-learn
                feature_names = poly.get_feature_names(['T'])
            except Exception:
                feature_names = [f"特征{i}" for i in range(len(coeffs))]
        
        # 构建LaTeX风格的表达式和常规表达式
        latex_expr = f"bias_{{{axis}}}(T) = {intercept:.{precision}f}"
        normal_expr = f"bias_{axis}(T) = {intercept:.{precision}f}"
        
        for j, (coef, feature) in enumerate(zip(coeffs, feature_names)):
            # 跳过截距项（已经添加）
            if feature == '1':
                continue
                
            # 将"T^2"替换为更好的显示
            feature_display = feature.replace("T^", "T^").replace("T 1", "T")
            
            # 对于LaTeX表达式
            if coef >= 0:
                latex_expr += f" + {coef:.{precision}f} \\cdot {feature_display}"
            else:
                latex_expr += f" - {abs(coef):.{precision}f} \\cdot {feature_display}"
                
            # 对于常规表达式
            if coef >= 0:
                normal_expr += f" + {coef:.{precision}f}*{feature_display}"
            else:
                normal_expr += f" - {abs(coef):.{precision}f}*{feature_display}"
        
        # 存储表达式
        expressions[axis] = {
            "latex": latex_expr,
            "normal": normal_expr,
            "coefficients": {
                "intercept": float(intercept),
                "linear": float(coeffs[1]) if len(coeffs) > 1 else 0,
                "quadratic": float(coeffs[2]) if len(coeffs) > 2 else 0
            }
        }
        
        # 打印表达式
        print(f"\n{axis}轴校准模型:")
        print(f"常规形式: {normal_expr}")
        print(f"LaTeX形式: {latex_expr}")
        
        # 简化形式（假设是二次多项式）
        if len(coeffs) >= 3:
            a = float(intercept)
            b = float(coeffs[1]) if len(coeffs) > 1 else 0  # 线性项系数
            c = float(coeffs[2]) if len(coeffs) > 2 else 0  # 二次项系数
            
            simplified = f"bias_{axis}(T) = {a:.{precision}f} + {b:.{precision}f}*T + {c:.{precision}f}*T²"
            expressions[axis]["simplified"] = simplified
            print(f"简化形式: {simplified}")
    
    print("\n代码实现:")
    print("```python")
    print("def calculate_bias(T):")
    print("    \"\"\"根据温度计算陀螺仪零偏\"\"\"")
    print("    bias_x = 0")
    print("    bias_y = 0")
    print("    bias_z = 0")
    print("    ")
    for axis in axes:
        if axis in expressions:
            coefs = expressions[axis]["coefficients"]
            a = coefs["intercept"]
            b = coefs["linear"]
            c = coefs["quadratic"]
            print(f"    # {axis}轴零偏计算")
            print(f"    bias_{axis.lower()} = {a:.{precision}f} + {b:.{precision}f}*T + {c:.{precision}f}*T*T")
    print("    ")
    print("    return bias_x, bias_y, bias_z")
    print("```")
            
    return expressions


# 3. 校准函数：给定温度和原始陀螺仪读数，应用校准模型
def calibrate_gyro(temperature, raw_data, models):
    """
    对给定温度下的原始陀螺仪读数应用校准模型
    
    参数:
    temperature: 当前温度
    raw_data: 原始陀螺仪读数 [x, y, z]
    models: 陀螺仪校准模型
    
    返回:
    calibrated_gyro: 校准后的陀螺仪读数 [x, y, z]
    """
    calibrated_gyro = np.zeros(3)
    temp = np.array([[temperature]])

    # for axis in range(3):
    for i, axis in enumerate(['gyro_x', 'gyro_y', 'gyro_z','accel_x', 'accel_y', 'accel_z']):
        # 预测当前温度下的零偏
        bias = models[axis].predict(temp)[0]
        # 校正陀螺仪读数
        calibrated_gyro[i] = raw_data[axis] - bias
    
    return calibrated_gyro

# 4. 模型评估函数
def evaluate_gyro_model(models, imu_readings):
    """
    评估校准模型的性能
    
    参数:
    models: 训练好的模型
    temperatures: 测试温度数据
    imu_readings: 对应的imu读数
    
    返回:
    metrics: 性能指标字典
    """
    metrics = {'r2': [], 'rmse': []}
    axes = ['gyro_x', 'gyro_y', 'gyro_z']
    
    # for axis in range(3):
    for i, axis in enumerate(axes):
        model = models[axis]
        X = imu_readings['temp'].reshape(-1, 1)
        y_true = imu_readings[axis]
        y_pred = model.predict(X)
        
        # 计算R²和RMSE
        r2 = r2_score(y_true, y_pred)
        rmse = np.sqrt(mean_squared_error(y_true, y_pred))
        
        metrics['r2'].append(r2)
        metrics['rmse'].append(rmse)
        
        print(f"轴 {axes[i]} - R²: {r2:.4f}, RMSE: {rmse:.4f}")
    
    return metrics

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

def visualize_calibrate_data(imu_data, angles, gravity, accel, positions):
    # 创建图形和子图（3行2列的布局）
    fig, axs = plt.subplots(2, 4, figsize=(14, 8))
    time_original = np.arange(len(imu_data['gyro_x']))

    time_intervals = np.diff(imu_data['tamp'])
    sampling_frequency = 1000000 / time_intervals

    axs[0, 0].plot(time_original, sampling_frequency[:len(time_original)], label='X轴')
    axs[0, 0].set_title('采样频率')
    axs[0, 0].set_ylabel('频率（hz）')
    axs[0, 0].legend()
    axs[0, 0].grid(True)
    
    axs[0, 1].plot(time_original, imu_data['accel_x'], label='X轴')
    axs[0, 1].plot(time_original, imu_data['accel_y'], label='Y轴')
    axs[0, 1].plot(time_original, imu_data['accel_z'], label='Z轴')
    axs[0, 1].set_title('加速度数据')
    axs[0, 1].set_ylabel('加速度 (g)')
    axs[0, 1].legend()
    axs[0, 1].grid(True)
    
    axs[0, 2].plot(time_original, imu_data['gyro_x'], label='X轴')
    axs[0, 2].plot(time_original, imu_data['gyro_y'], label='Y轴')
    axs[0, 2].plot(time_original, imu_data['gyro_z'], label='Z轴')
    axs[0, 2].set_title('角速度数据')
    axs[0, 2].set_ylabel('角速度 (°/s)')
    axs[0, 2].legend()
    axs[0, 2].grid(True)
    
    axs[1, 0].plot(time_original, angles['roll'], label='Roll')
    axs[1, 0].plot(time_original, angles['pitch'], label='Pitch', color='red')
    # axs[1, 0].plot(time_original, angles['yaw'], label='Yaw', color='green')
    axs[1, 0].set_title('姿态')
    axs[1, 0].legend()
    axs[1, 0].grid(True)
    
    axs[1, 1].plot(time_original, gravity['x'], label='gravity X')
    axs[1, 1].plot(time_original, gravity['y'], label='gravity Y')
    axs[1, 1].plot(time_original, gravity['z'], label='gravity Z')
    axs[1, 1].set_title('重力')
    axs[1, 1].set_ylabel('重力加速度 (g)')
    axs[1, 1].legend()
    axs[1, 1].grid(True)

    axs[1, 2].plot(time_original, accel['x'], label='accel X')
    axs[1, 2].plot(time_original, accel['y'], label='accel Y')
    axs[1, 2].plot(time_original, accel['z'], label='accel Z')
    axs[1, 2].set_title('运动加速度')
    axs[1, 2].set_ylabel('运动加速度 (m/s²)')
    axs[1, 2].legend()
    axs[1, 2].grid(True)

    axs[1, 3].plot(time_original, positions['x'], label='positions X')
    axs[1, 3].plot(time_original, positions['y'], label='positions Y')
    axs[1, 3].plot(time_original, positions['z'], label='positions Z')
    axs[1, 3].set_title('位置（m）')
    axs[1, 3].legend()
    axs[1, 3].grid(True)

    # 调整布局
    plt.tight_layout()
    # plt.show()
    
def visualize_mpu6050_data(original_data, filtered_data, calibrated_data):
    """可视化原始数据和滤波后的数据对比"""
    # 创建时间轴
    time_original = np.arange(len(original_data['accel_x']))
    time_filtered = np.arange(len(filtered_data['accel_x']))
    
    # 创建图形和子图（3行2列的布局）
    fig, axs = plt.subplots(3, 3, figsize=(15, 12))
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
    
    # 原始数据
    axs[1, 0].plot(time_original, original_data['gyro_x'], label='X轴')
    axs[1, 0].plot(time_original, original_data['gyro_y'], label='Y轴')
    axs[1, 0].plot(time_original, original_data['gyro_z'], label='Z轴')
    axs[1, 0].set_title('原始陀螺仪数据')
    axs[1, 0].set_ylabel('角速度 (原始值)')
    axs[1, 0].legend()
    axs[1, 0].grid(True)

    # 滤波后数据
    axs[0, 1].plot(time_filtered, filtered_data['accel_x'], label='X轴')
    axs[0, 1].plot(time_filtered, filtered_data['accel_y'], label='Y轴')
    axs[0, 1].plot(time_filtered, filtered_data['accel_z'], label='Z轴')
    axs[0, 1].set_title('滤波后加速度计数据')
    axs[0, 1].set_ylabel('加速度 (滤波值)')
    axs[0, 1].legend()
    axs[0, 1].grid(True)
    
    # 滤波后数据
    axs[1, 1].plot(time_filtered, filtered_data['gyro_x'], label='X轴')
    axs[1, 1].plot(time_filtered, filtered_data['gyro_y'], label='Y轴')
    axs[1, 1].plot(time_filtered, filtered_data['gyro_z'], label='Z轴')
    axs[1, 1].set_title('滤波后陀螺仪数据')
    axs[1, 1].set_ylabel('角速度 (滤波值)')
    axs[1, 1].legend()
    axs[1, 1].grid(True)

    # 校准后数据
    axs[0, 2].plot(time_filtered, calibrated_data['accel_x'], label='X轴')
    axs[0, 2].plot(time_filtered, calibrated_data['accel_y'], label='Y轴')
    axs[0, 2].plot(time_filtered, calibrated_data['accel_z'], label='Z轴')
    axs[0, 2].set_title('校准加速度计数据(六面法)')
    axs[0, 2].set_ylabel('加速度 (校准值)')
    axs[0, 2].legend()
    axs[0, 2].grid(True)
    
    # 校准后数据
    axs[1, 2].plot(time_filtered, calibrated_data['gyro_x'], label='X轴')
    axs[1, 2].plot(time_filtered, calibrated_data['gyro_y'], label='Y轴')
    axs[1, 2].plot(time_filtered, calibrated_data['gyro_z'], label='Z轴')
    axs[1, 2].set_title('校准角速度计数据(线性回归)')
    axs[1, 2].set_ylabel('角速度 (°/s)')
    axs[1, 2].legend()
    axs[1, 2].grid(True)
    
    
    # 原始数据
    axs[2, 0].plot(time_original, original_data['temp'], color='red')
    axs[2, 0].set_title('原始温度数据')
    axs[2, 0].set_xlabel('样本点')
    axs[2, 0].set_ylabel('温度 (原始值)')
    axs[2, 0].grid(True)

    # 滤波后数据
    axs[2, 1].plot(time_filtered, calibrated_data['temp'], color='red')
    axs[2, 1].set_title('滤波后温度数据')
    axs[2, 1].set_xlabel('样本点')
    axs[2, 1].set_ylabel('温度 (度)')
    axs[2, 1].grid(True)
    
    time_intervals = np.diff(original_data['tamp'])
    sampling_frequency = 1000000 / time_intervals
    
    # 采用频率
    axs[2, 2].plot(time_original[:-1], sampling_frequency, color='green')
    axs[2, 2].set_title('采样频率')
    axs[2, 2].grid(True)
    
    # 调整布局
    plt.tight_layout()
    # plt.show()

def visualize_attidute(angles):
    fig, axs = plt.subplots(1, 3, figsize=(8, 6))
    time_original = np.arange(len(angles['roll']))

    axs[0].plot(time_original, angles['roll'], label='Roll')
    # axs[0].plot(time_original, angles['pitch'], label='Pitch')
    # axs[0].plot(time_original, angles['yaw'], label='Yaw')
    axs[0].set_title('姿态')
    axs[0].legend()
    axs[0].grid(True)
    
    axs[1].plot(time_original, angles['pitch'], label='Pitch', color='red')
    axs[1].set_title('姿态')
    axs[1].legend()
    axs[1].grid(True)

    axs[2].plot(time_original, angles['yaw'], label='Yaw', color='green')
    axs[2].set_title('姿态')
    axs[2].legend()
    axs[2].grid(True)

def visualize_data(data):
    fig, axs = plt.subplots(1, 1, figsize=(8, 6))
    axs.plot(np.arange(len(data)), data, label='Data',)
    axs.set_title('Data')
    axs.legend()
    axs.grid(True)

    plt.show()


def attitude_calculate(data):
    print(len(data['tamp']),len(data['gyro_x']))
    angles = {'roll':[], 'pitch':[], 'yaw': []}
    # 初始化 Mahony 滤波器
    '''
    KP (比例增益)：通常在0.5到5.0之间
    # KI (积分增益)：通常在0.0到0.1之间
    KP调整：

    增大KP会使系统更快地响应加速度计/磁力计的校正

    但过大的KP会导致高频振动

    50Hz下典型范围：0.5-2.0

    KI调整：

    KI用于消除稳态误差

    过大的KI会导致超调和振荡

    50Hz下典型范围：0.001-0.02

    调整方法：

    先设KI=0，调整KP直到系统响应快速但不振荡

    然后慢慢增加KI以消除稳态误差

    典型场景参数：

    缓慢运动：KP=0.5-1.0, KI=0.001-0.005

    快速运动：KP=2.0-5.0, KI=0.005-0.02

    高振动环境：KP=0.1-0.5, KI=0.0-0.001
    '''
    ahrs = MahonyAHRS(sample_freq=50.0, kp=0.409, ki=0.008)

    # 模拟传感器数据（示例）
    dt = 0.02  # 20ms 时间步长
    for t in range(len(data['gyro_x'])):
        # 陀螺仪数据（假设绕 Z 轴旋转）
        gyro = [data['gyro_x'][t],data['gyro_y'][t],data['gyro_z'][t]]
        
        # 加速度计数据（假设静止，指向下方）
        accel = [data['accel_x'][t],data['accel_y'][t],data['accel_z'][t]]  # 重力加速度
        
        # 更新姿态
        ahrs.update_imu(gyro[0]/RAD2DEG, gyro[1]/RAD2DEG, gyro[2]/RAD2DEG, accel[0], accel[1], accel[2])

        # q0, q1, q2, q3 = ahrs.get_quaternion()
        # print(f"Quaternion: ({q0:.4f}, {q1:.4f}, {q2:.4f}, {q3:.4f})")
        
        # 获取欧拉角
        roll, pitch, yaw = ahrs.get_euler_angles()
        # print(f"Roll: {roll:.2f}°, Pitch: {pitch:.2f}°, Yaw: {yaw:.2f}°")
        
        angles['roll'].append(roll)
        angles['pitch'].append(pitch)
        angles['yaw'].append(yaw)
    
    return angles
    
def attitude_integral(data):
    angles = {'roll':[], 'pitch':[], 'yaw': []}
    estimator = GyroAttitudeEstimator()
    for i in range(len(data['gyro_x'])):
        gyro_data={'gyro_x': data['gyro_x'][i],  
                    'gyro_y': data['gyro_y'][i], 
                    'gyro_z': data['gyro_z'][i]
                    }
        estimator.update(gyro_data, data['tamp'][i]/1000000)
        attitude = estimator.get_attitude()
        angles['roll'].append(attitude['roll'])
        angles['pitch'].append(attitude['pitch'])
        angles['yaw'].append(attitude['yaw'])
    return angles

'''
    Convert degrees to radians for the example
    roll_deg = 30
    pitch_deg = 45
    
    roll_rad = math.radians(roll_deg)
    pitch_rad = math.radians(pitch_deg)
    
    gx, gy, gz = gravity_components(roll_rad, pitch_rad)
    
    print(f"For roll = {roll_deg}° and pitch = {pitch_deg}°:")
    print(f"Gravity components:")
    print(f"gx = {gx:.4f} m/s²")
    print(f"gy = {gy:.4f} m/s²") 
    print(f"gz = {gz:.4f} m/s²")
    
    # Verify that magnitude is still g
    magnitude = math.sqrt(gx*gx + gy*gy + gz*gz)
    print(f"Magnitude = {magnitude:.4f} m/s² (should be close to 9.81)")
'''

def gravity_components(roll, pitch):
    """
    Calculate the components of gravity vector on x, y, and z axes
    given roll and pitch angles.
    
    Args:
        roll: Roll angle in radians (rotation around x-axis)
        pitch: Pitch angle in radians (rotation around y-axis)
        g: Gravity constant, default is 9.81 m/s²
    
    Returns:
        tuple: (gx, gy, gz) components of gravity on each axis
    """
    # Create rotation matrix for roll (around x-axis)
    R_roll = np.array([
        [1, 0, 0],
        [0, math.cos(roll), -math.sin(roll)],
        [0, math.sin(roll), math.cos(roll)]
    ])
    
    # Create rotation matrix for pitch (around y-axis)
    R_pitch = np.array([
        [math.cos(pitch), 0, math.sin(pitch)],
        [0, 1, 0],
        [-math.sin(pitch), 0, math.cos(pitch)]
    ])
    
    # Combine rotations (first roll, then pitch)
    R = np.matmul(R_pitch, R_roll)
    
    # Gravity vector in world frame (pointing down in z-axis)
    gravity_world = np.array([0, 0, 1])
    
    # Rotate gravity to body frame
    gravity_body = np.matmul(R.transpose(), gravity_world)
    
    # Extract components
    gx, gy, gz = gravity_body
    
    return gx, gy, gz

def gravity_calculate(attidudes):
    g_xyz={'x':[],'y':[],'z':[]}
    for  i in range(len(attidudes['roll'])):
        gx,gy,gz = gravity_components(attidudes['roll'][i]/RAD2DEG, attidudes['pitch'][i]/RAD2DEG)
        g_xyz['x'].append(gx)
        g_xyz['y'].append(gy)
        g_xyz['z'].append(gz)
    return g_xyz

# accel={'x':[],'y':[],'z':[]} [m/s²]
def calculate_position(accel, dt):
    positions = {'x': [], 'y': [], 'z': []}
    print(accel)
    for axis in ['x', 'y', 'z']:
        v = 0.0  # 初始速度
        s = 0.0  # 初始位移
        positions[axis] = []
        for a in accel[axis]:
            v_new = v + a * dt        # 积分加速度得到新速度
            # ds = 0.5 * (v + v_new) * dt  # 使用平均速度计算位移增量
            ds = v * dt + 0.5 * a * dt * dt # 使用平均速度计算位移增量
            s += ds
            positions[axis].append(s)
            # v = v_new  # 更新速度
            v = v + a * dt
    return positions

# 主函数
def main():
    # build_models = True
    build_models = False
    
    # 设置中文字体
    set_chinese_font()
    
    file_path = 'data/mpu6050_data.txt'  # 替换为你的文件路径
    window_size = 10  # 滤波窗口大小，可以根据需要调整

    try:
        original_data = parse_mpu6050_data(file_path)
        # 应用滤波
        filtered_data = apply_filters(original_data, window_size)
        
        # 温度转换
        conver_data = convertTemp(filtered_data)
        # 加速度转换(g)
        conver_data = convertAccel(conver_data)
        # 角速度转换
        conver_data = convertGyro(conver_data)


        # 从单个文件加载全部模型
        models_gyro = joblib.load(calibration_gyro_models)
        models_accel = joblib.load(calibration_accel_models)
        # display_model_expressions(models_gyro)
        # display_model_expressions(models_accel)
        
        # 零偏校准
        # calibrated_data = calibrate_accel(conver_data, models_accel)
        # calibrated_data = vectorized_calibrate_gyro(calibrated_data, models_gyro)
        calibrated_data = calibrate_accel_bias(conver_data)
        calibrated_data = vectorized_calibrate_gyro(calibrated_data, models_gyro)
        visualize_mpu6050_data(original_data, filtered_data, calibrated_data)


        angles = attitude_calculate(calibrated_data)
        # angles = attitude_integral(calibrated_data)
    
        # 计算重力加速度在x,y,z轴上的分量
        # {'x':[],'y':[],'z':[]}
        g_xyz = gravity_calculate(angles)
        # print(g_xyz)

        # print(angles)
        # visualize_attidute(angles=angles)
        
        # 计算物体的运动加速度
        accel = {}
        accel['x'] = (calibrated_data['accel_x'] - g_xyz['x'])*g
        accel['y'] = (calibrated_data['accel_y'] - g_xyz['y'])*g
        accel['z'] = (calibrated_data['accel_z'] - g_xyz['z'])*g

        # 计算位移
        positions = calculate_position(accel=accel, dt=1.0/50)

        visualize_calibrate_data({**calibrated_data, 'tamp':original_data['tamp']},
                                 angles=angles,
                                 gravity=g_xyz,
                                 accel=accel,
                                 positions=positions)

        # show_animate(angles, positions)
        plt.show()
    except Exception as e:
        print(f"处理数据时出错: {e}")

if __name__ == "__main__":
    main()
    
    
    
'''
    校正模型
    # X轴零偏计算
    bias_x = -588.622831 + 0.011269*T + -0.000004*T*T
    # Y轴零偏计算
    bias_y = -33.989779 + 0.004566*T + 0.000000*T*T
    # Z轴零偏计算
    bias_z = 98.785609 + 0.013705*T + 0.000001*T*T
'''