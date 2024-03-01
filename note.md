板子拔掉GND VCC飞线
idf.py build
idf.py flash
ctrl+] 推出monitor

开机 串口1有输出时无法flash,按住boot插电烧录

compomemt config -- esp systerm setting -- channel console output
bootloader config -- bootloader log verbosity