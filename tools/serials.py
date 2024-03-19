import serial
import re
import matplotlib.pyplot as plt
from matplotlib import animation
import numpy as np
from pynput import keyboard


delimiter = "imu:"

# 配置串口参数
ser = serial.Serial(
    port='/dev/cu.usbmodem578E0044521',  # 串口设备路径
    baudrate=115200,        # 波特率
    parity=serial.PARITY_NONE,  # 无校验位
    stopbits=serial.STOPBITS_ONE,  # 1个停止位
    bytesize=serial.EIGHTBITS,  # 8位数据位
    timeout=1  # 读取超时时间(秒)
)
data=[[],[],[],[],[],[]]

# 创建图表
fig, ax = plt.subplots()
# line, = ax.plot(data)

line_x, = ax.plot([], [], lw=1, label='X')
line_y, = ax.plot([], [], lw=1, label='Y')
line_z, = ax.plot([], [], lw=1, label='Z')
line_a, = ax.plot([], [], lw=1, label='A')
line_b, = ax.plot([], [], lw=1, label='B')
line_c, = ax.plot([], [], lw=1, label='C')
line=[line_x,line_y,line_z,line_a,line_b,line_c]
range_stop=3
if delimiter=="angle:":
    ax.set_ylim(-180, 180)  # 根据需要调整y轴范围
    range_stop=3
elif delimiter=="sampleFreq:":
    ax.set_ylim(0, 100)  # 根据需要调整y轴范围
    range_stop=1
else:
    ax.set_ylim(-1.5, 1.5)  # 根据需要调整y轴范围
    range_stop=6
    
ax.set_title('Real-time Data Visualization')
ax.set_xlabel('Sample')
ax.set_ylabel('Value')
plt.ion()  # 启用交互模式
plt.show()

# 监听键盘输入
def on_press(key):
    global should_exit
    should_exit = True
    return False

listener = keyboard.Listener(on_press=on_press)
listener.start()  # 开始监听键盘输入

should_exit = False  # 控制程序退出
max_length = 50
# 读取串口数据并调用update函数
try:
    while not should_exit:
        lineRecv = ser.readline().decode('utf-8', errors='ignore').rstrip()

        if lineRecv:
            print(f"Received data: {lineRecv}")
            # 找到"raw:"的位置
            start_index = lineRecv.find(delimiter)
            if(start_index == -1):
                continue
            start_index += len(delimiter)

            # 从"raw:"后面开始截取字符串
            raw_data = lineRecv[start_index:]

            # 将字符串分割成列表
            raw_data_list = raw_data.split(", ")
            print(raw_data_list)
            
            # pattern = r"[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?"
            # numbers = re.findall(pattern, lineRecv)

            # new_data = float(lineRecv.split(':')[-1])
            # new_data = np.random.uniform(-1, 1)
            for i in range(0,range_stop):
                data[i].append(float(raw_data_list[i]))
                # 控制列表长度为100
                if len(data[i]) > max_length:
                    data[i].pop(0)
                line[i].set_data(range(len(data[i])), data[i])  # 更新图表数据
                
            
            ax.relim()  # 自动调整x轴范围
            ax.autoscale_view(True, True, True)  # 自动调整坐标轴范围
            fig.canvas.draw()  # 重绘图表
            fig.canvas.flush_events()  # 处理窗口管理器事件
    
except KeyboardInterrupt:
    # 捕获键盘中断异常,用于安全退出程序
    print("Exiting program...")
    exit()

finally:
    # 确保在程序退出时关闭串口
    ser.close()