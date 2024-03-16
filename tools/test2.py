import serial

# 配置串口参数
ser = serial.Serial(
    port='/dev/cu.usbmodem578E0044521',  # 串口设备路径
    baudrate=115200,        # 波特率
    parity=serial.PARITY_NONE,  # 无校验位
    stopbits=serial.STOPBITS_ONE,  # 1个停止位
    bytesize=serial.EIGHTBITS,  # 8位数据位
    timeout=1  # 读取超时时间(秒)
)

try:
    while True:
        # 读取串口数据
        data = ser.readline().decode().rstrip()
        if data:
            print(f"Received data: {data}")

except KeyboardInterrupt:
    # 捕获键盘中断异常,用于安全退出程序
    print("Exiting program...")

finally:
    # 确保在程序退出时关闭串口
    ser.close()