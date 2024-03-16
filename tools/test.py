import matplotlib.pyplot as plt
import numpy as np
from matplotlib import animation

# 创建figure和axes对象
fig, ax = plt.subplots()

# 初始化数据列表
data = []

# 初始化线对象
line, = ax.plot([], [], 'r-')

# 设置x和y轴范围
ax.set_xlim(0, 100)
ax.set_ylim(-1, 1)

# 动画更新函数
def update(frame):
    # 生成新的数据点
    new_data = np.random.uniform(-1, 1)
    data.append(new_data)

    # 限制数据长度为100
    if len(data) > 100:
        data.pop(0)

    # 更新线对象的数据
    line.set_data(range(len(data)), data)

    # 重新设置x轴范围
    ax.set_xlim(max(0, len(data) - 100), len(data))

    return line,

# 创建动画对象
ani = animation.FuncAnimation(fig, update, frames=np.linspace(0, 10, 1000), interval=50, blit=True)

# 显示动画
plt.show()