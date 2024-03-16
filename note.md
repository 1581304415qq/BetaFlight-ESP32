板子拔掉GND VCC飞线
idf.py build
idf.py flash
ctrl+] 推出monitor

开机 串口1有输出时无法flash,按住boot插电烧录

compomemt config -- esp systerm setting -- channel console output
bootloader config -- bootloader log verbosity


如果启用了选项 CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS ，则可以使用 FreeRTOS API vTaskGetRunTimeStats() 来获取各个 FreeRTOS 任务运行时占用处理器的时间。