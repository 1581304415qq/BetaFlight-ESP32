import serial
import re
import matplotlib.pyplot as plt
from matplotlib import animation
import numpy as np


# 配置串口参数
ser = serial.Serial(
    port='/dev/cu.usbmodem578E0044521',  # 串口设备路径
    baudrate=115200,        # 波特率
    parity=serial.PARITY_NONE,  # 无校验位
    stopbits=serial.STOPBITS_ONE,  # 1个停止位
    bytesize=serial.EIGHTBITS,  # 8位数据位
    timeout=1  # 读取超时时间(秒)
)
data=[]

# 创建图表
fig, ax = plt.subplots()
line, = ax.plot(data)
ax.set_ylim(-2, 2)  # 根据需要调整y轴范围
ax.set_title('Real-time Data Visualization')
ax.set_xlabel('Sample')
ax.set_ylabel('Value')
plt.ion()  # 启用交互模式
plt.show()


# 读取串口数据并调用update函数
try:
    while True:
        lineRecv = ser.readline().decode('utf-8', errors='ignore').rstrip()

        if lineRecv:
            print(f"Received data: {lineRecv}")
            # 找到"raw:"的位置
            start_index = lineRecv.find("raw:")
            if(start_index == -1):
                continue
            start_index += 4

            # 从"raw:"后面开始截取字符串
            raw_data = lineRecv[start_index:]

            # 将字符串分割成列表
            raw_data_list = raw_data.split(", ")
            
            # pattern = r"[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?"
            # numbers = re.findall(pattern, lineRecv)

            # new_data = float(lineRecv.split(':')[-1])
            # new_data = np.random.uniform(-1, 1)

            data.append(float(raw_data_list[0]))      # 将新数据添加到列表中
            line.set_data(range(len(data)), data)  # 更新图表数据
            
            ax.relim()  # 自动调整x轴范围
            ax.autoscale_view(True, True, True)  # 自动调整坐标轴范围
            fig.canvas.draw()  # 重绘图表
            fig.canvas.flush_events()  # 处理窗口管理器事件
    
except KeyboardInterrupt:
    # 捕获键盘中断异常,用于安全退出程序
    print("Exiting program...")

finally:
    # 确保在程序退出时关闭串口
    ser.close()