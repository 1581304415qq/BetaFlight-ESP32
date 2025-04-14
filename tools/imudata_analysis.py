import re
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import font_manager
from sklearn.preprocessing import PolynomialFeatures
from sklearn.linear_model import LinearRegression
from sklearn.pipeline import Pipeline
from sklearn.metrics import mean_squared_error, r2_score
import joblib  # 推荐用于大数据模型

axes = ['gyro_x', 'gyro_y', 'gyro_z','accel_x', 'accel_y', 'accel_z']

# ========================
# 零偏校准核心算法
# ========================
def calibrate_bias(raw_data, sensitivity=0.00006103515625):
    #返回的 calibrated_data 数据结构与原数据相同
    calibrated_data={}
    """执行零偏校准并返回校准后的数据"""
    for key, values in raw_data.items():
        # 计算零偏（平均值）
        bias = np.mean(values)
        
        # 应用校准
        calibrated_data[key] = values - bias
        if key=='accel_z':
           calibrated_data[key] = calibrated_data[key] + 16384
    
    return calibrated_data


# 2. 建立二次多项式回归模型 y = β₀ + β₁·T + β₂·T²
def build_calibration_model(temperatures, imu_readings):
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
    for i, axis in enumerate(['gyro_x', 'gyro_y', 'gyro_z','accel_x', 'accel_y', 'accel_z']):
        # 创建二次多项式回归模型管道
        model = Pipeline([
            ('poly', PolynomialFeatures(degree=2)),
            ('linear', LinearRegression())
        ])
        
        # 训练模型
        X = temperatures.reshape(-1, 1)
        y = imu_readings[axis]
        model.fit(X, y)
        
        models[axis] = model
    
    return models

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

def vectorized_calibrate_gyro(temperatures, raw_data, models):
    """
    向量化版本的陀螺仪校准函数，一次处理整个数据集
    
    参数:
    temperatures: 温度数据数组 (n,)
    raw_data: 原始陀螺仪数据 ['gyro_x', 'gyro_y', 'gyro_z','accel_x', 'accel_y', 'accel_z']，每个是长度为n的数组
    models: 校准模型列表，长度为3，对应x、y、z轴
    
    返回:
    calibrated_data: 包含校准后数据的字典
    """
    n_samples = len(temperatures)
    calibrated_data = {
        'gyro_x': np.zeros(n_samples),
        'gyro_y': np.zeros(n_samples),
        'gyro_z': np.zeros(n_samples)
    }
    
    # 将温度数据重塑为模型所需的形状
    temps_reshaped = temperatures.reshape(-1, 1)
    
    # 对每个轴分别预测偏差并应用校准
    for i, axis in enumerate(['gyro_x', 'gyro_y', 'gyro_z','accel_x', 'accel_y', 'accel_z']):
        # 预测当前轴在所有温度下的偏差
        biases = models[axis].predict(temps_reshaped)
        
        # 从原始数据中减去偏差，得到校准后的数据
        calibrated_data[axis] = raw_data[axis] - biases
    
    return calibrated_data

# 4. 模型评估函数
def evaluate_model(models, temperatures, imu_readings):
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
    axes = ['gyro_x', 'gyro_y', 'gyro_z','accel_x', 'accel_y', 'accel_z']
    
    # for axis in range(3):
    for i, axis in enumerate(axes):
        model = models[axis]
        X = temperatures.reshape(-1, 1)
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

def visualize_calibrate_data(calibrate_data):
    # 创建图形和子图（3行2列的布局）
    fig, axs = plt.subplots(2, 2, figsize=(15, 12))
    fig.set_size_inches(14, 8)  # 实时修改为8x6英寸‌:ml-citation{ref="5,6" data="citationList"}

    time_original = np.arange(len(calibrate_data['gyro_x']))

    axs[0, 0].plot(time_original, calibrate_data['gyro_x'], label='X轴')
    axs[0, 0].plot(time_original, calibrate_data['gyro_y'], label='Y轴')
    axs[0, 0].plot(time_original, calibrate_data['gyro_z'], label='Z轴')
    axs[0, 0].set_title('原始角速度计数据')
    axs[0, 0].set_ylabel('角速度 (校准值)')
    axs[0, 0].legend()
    axs[0, 0].grid(True)
    # 调整布局
    plt.tight_layout()
    plt.show()
    
def visualize_mpu6050_data(original_data, filtered_data, calibrate_data, calibrated_data):
    """可视化原始数据和滤波后的数据对比"""
    # 创建时间轴
    time_original = np.arange(len(original_data['accel_x']))
    time_filtered = np.arange(len(filtered_data['accel_x']))
    
    # 创建图形和子图（3行2列的布局）
    fig, axs = plt.subplots(4, 4, figsize=(15, 12))
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
    
    # 校准后数据
    axs[0, 2].plot(time_filtered, calibrate_data['accel_x'], label='X轴')
    axs[0, 2].plot(time_filtered, calibrate_data['accel_y'], label='Y轴')
    axs[0, 2].plot(time_filtered, calibrate_data['accel_z'], label='Z轴')
    axs[0, 2].set_title('校准加速度计数据(平均值)')
    axs[0, 2].set_ylabel('加速度 (校准值)')
    axs[0, 2].legend()
    axs[0, 2].grid(True)
    
    axs[0, 3].plot(time_filtered, calibrated_data['accel_x'], label='X轴')
    axs[0, 3].plot(time_filtered, calibrated_data['accel_y'], label='Y轴')
    axs[0, 3].plot(time_filtered, calibrated_data['accel_z'], label='Z轴')
    axs[0, 3].set_title('校准加速度计数据(线性回归)')
    axs[0, 3].set_ylabel('加速度 (校准值)')
    axs[0, 3].legend()
    axs[0, 3].grid(True)
    
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
    
    # 校准后数据
    axs[1, 2].plot(time_filtered, calibrate_data['gyro_x'], label='X轴')
    axs[1, 2].plot(time_filtered, calibrate_data['gyro_y'], label='Y轴')
    axs[1, 2].plot(time_filtered, calibrate_data['gyro_z'], label='Z轴')
    axs[1, 2].set_title('校准角速度计数据(平均值)')
    axs[1, 2].set_ylabel('角速度 (校准值)')
    axs[1, 2].legend()
    axs[1, 2].grid(True)
    
    axs[1, 3].plot(time_filtered, calibrated_data['gyro_x'], label='X轴')
    axs[1, 3].plot(time_filtered, calibrated_data['gyro_y'], label='Y轴')
    axs[1, 3].plot(time_filtered, calibrated_data['gyro_z'], label='Z轴')
    axs[1, 3].set_title('校准角速度计数据(线性回归)')
    axs[1, 3].set_ylabel('角速度 (校准值)')
    axs[1, 3].legend()
    axs[1, 3].grid(True)
    
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
    
    file_path = 'data/mpu6050_data.txt'  # 替换为你的文件路径
    window_size = 10  # 滤波窗口大小，可以根据需要调整

    try:
        original_data = parse_mpu6050_data(file_path)
        
        # 应用滤波
        filtered_data = apply_filters(original_data, window_size)
        
        # 零偏校准
        calibrate_data = calibrate_bias(filtered_data)
                
        # 建立误差模型
        # models = build_calibration_model(filtered_data['temp'],filtered_data)
        
        # 保存三个轴的校准模型到单个文件
        # joblib.dump(models, 'calibration_models.pkl')
        
        # 从单个文件加载全部模型
        models = joblib.load('calibration_models.pkl')
        
        display_model_expressions(models)
        
        # 评估模型
        metrics = evaluate_model(models,filtered_data['temp'],filtered_data)
        
        # 使用模型修正数据
        # calibrated_data = {'gyro_x':[],'gyro_y':[],'gyro_z':[]}
        # for i in range(len(filtered_data['gyro_x'])):
        #     calibrated_gyro = calibrate_gyro(filtered_data['temp'][i],[filtered_data['gyro_x'][i],filtered_data['gyro_y'][i],filtered_data['gyro_z'][i]],models)
        #     calibrated_data['gyro_x'].append(calibrated_gyro[0])
        #     calibrated_data['gyro_y'].append(calibrated_gyro[1])
        #     calibrated_data['gyro_z'].append(calibrated_gyro[2])

        calibrated_data = vectorized_calibrate_gyro(filtered_data['temp'],filtered_data,models)

        visualize_mpu6050_data(original_data, filtered_data, calibrate_data, calibrated_data)
        # visualize_calibrate_data(calibrated_data)
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