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


RAD2DEG = 57.29578
axes = ['gyro_x', 'gyro_y', 'gyro_z', 'accel_x', 'accel_y', 'accel_z']
acce_sensitivity = 16384
gyro_sensitivity = 131
g = 9.81

calibration_gyro_models = 'models/calibration_gyro_models_50hz.pkl'
calibration_accel_models = 'models/calibration_accel_models_50hz.pkl'


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
    return {**raw_data, **data}
# 角速度转换


def convertGyro(raw_data):
    data = {}
    data['gyro_x'] = raw_data['gyro_x'] / gyro_sensitivity
    data['gyro_y'] = raw_data['gyro_y'] / gyro_sensitivity
    data['gyro_z'] = raw_data['gyro_z'] / gyro_sensitivity
    return {**raw_data, **data}


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
    axes = ['gyro_x', 'gyro_y', 'gyro_z', 'accel_x', 'accel_y', 'accel_z']

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
                poly_key = [k for k in model.named_steps.keys()
                            if 'poly' in k.lower()][0]
                lr_key = [k for k in model.named_steps.keys(
                ) if 'linear' in k.lower() or 'regress' in k.lower()][0]
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
            print(
                f"    bias_{axis.lower()} = {a:.{precision}f} + {b:.{precision}f}*T + {c:.{precision}f}*T*T")
    print("    ")
    print("    return bias_x, bias_y, bias_z")
    print("```")

    return expressions


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

    # for axis in range(3):
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
        [1, 0, 0],    # +X
        [-1, 0, 0],   # -X
        [0, 1, 0],    # +Y
        [0, -1, 0],   # -Y
        [0, 0, 1],    # +Z
        [0, 0, -1]    # -Z
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


def visualize_data(data):
    fig, axs = plt.subplots(1, 1, figsize=(8, 6))
    axs.plot(np.arange(len(data)), data, label='Data',)
    axs.set_title('Data')
    axs.legend()
    axs.grid(True)

    plt.show()


def visualize_mpu6050_data(original_data, filtered_data):
    """可视化原始数据和滤波后的数据对比"""
    # 创建时间轴
    time_original = np.arange(len(original_data['accel_x']))
    time_filtered = np.arange(len(filtered_data['accel_x']))

    # 创建图形和子图（3行2列的布局）
    fig, axs = plt.subplots(3, 2, figsize=(15, 12))
    # 实时修改为8x6英寸‌:ml-citation{ref="5,6" data="citationList"}
    fig.set_size_inches(14, 8)

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

    axs[2, 0].plot(time_filtered, filtered_data['temp'], label='X轴')
    axs[2, 0].plot(time_filtered, filtered_data['temp'], label='Y轴')
    axs[2, 0].plot(time_filtered, filtered_data['temp'], label='Z轴')
    axs[2, 0].set_title('滤波后温度数据')
    axs[2, 0].set_ylabel('温度 (度)')
    axs[2, 0].legend()
    axs[2, 0].grid(True)

    # 滤波后数据
    axs[0, 1].plot(time_filtered, filtered_data['accel_x'], label='X轴')
    axs[0, 1].plot(time_filtered, filtered_data['accel_y'], label='Y轴')
    axs[0, 1].plot(time_filtered, filtered_data['accel_z'], label='Z轴')
    axs[0, 1].set_title('滤波后加速度计数据')
    axs[0, 1].set_ylabel('加速度 (g)')
    axs[0, 1].legend()
    axs[0, 1].grid(True)

    # 滤波后数据
    axs[1, 1].plot(time_filtered, filtered_data['gyro_x'], label='X轴')
    axs[1, 1].plot(time_filtered, filtered_data['gyro_y'], label='Y轴')
    axs[1, 1].plot(time_filtered, filtered_data['gyro_z'], label='Z轴')
    axs[1, 1].set_title('滤波后陀螺仪数据')
    axs[1, 1].set_ylabel('角速度 (°/s)')
    axs[1, 1].legend()
    axs[1, 1].grid(True)

    # 采用频率
    time_intervals = np.diff(original_data['tamp'])
    sampling_frequency = 1000000 / time_intervals
    axs[2, 1].plot(time_original[:-1], sampling_frequency, color='green')
    axs[2, 1].set_title('采样频率')
    axs[2, 1].set_ylabel('采样频率 (HZ)')
    axs[2, 1].grid(True)

    # 调整布局
    plt.tight_layout()
    # plt.show()

# 主函数


def main():
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

        visualize_mpu6050_data(original_data=original_data,
                               filtered_data=conver_data)

        # models_gyro = build_calibration_gyro_model(conver_data)
        # 保存三个轴的校准模型到单个文件
        # joblib.dump(models_gyro, calibration_gyro_models)
        # 评估模型
        # metrics = evaluate_gyro_model(models_gyro,conver_data)

        # models_accel = build_calibration_accel_model(conver_data)
        # joblib.dump(models_accel, calibration_accel_models)
        plt.show()
    except Exception as e:
        print(f"处理数据时出错: {e}")


if __name__ == "__main__":
    main()
