#!/usr/bin/python3
import serial
import re
import time

localtime = time.localtime(time.time())
date=time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
print ("本地时间为 :", date)

# 配置串口参数
ser = serial.Serial(
    port='/dev/cu.usbmodem578E0044521',  # 串口设备路径
    baudrate=115200,        # 波特率
    parity=serial.PARITY_NONE,  # 无校验位
    stopbits=serial.STOPBITS_ONE,  # 1个停止位
    bytesize=serial.EIGHTBITS,  # 8位数据位
    timeout=1  # 读取超时时间(秒)
)

# 读取串口数据并调用update函数
try:
    fo = open("log/"+date+".txt", "w")
    while True:
        lineRecv = ser.readline().decode('utf-8', errors='ignore')#.rstrip()

        if lineRecv:
            print(f"Received data: {lineRecv}")
            fo.write(lineRecv)
          
except KeyboardInterrupt:
    # 捕获键盘中断异常,用于安全退出程序
    print("Exiting program...")
    exit()

finally:
    # 确保在程序退出时关闭串口
    ser.close()
    fo.close()