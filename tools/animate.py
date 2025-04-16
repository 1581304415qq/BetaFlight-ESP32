import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.animation import FuncAnimation

def euler_to_rotation_matrix(roll, pitch, yaw):
    """将欧拉角转换为旋转矩阵 (ZYX顺序)"""
    roll = np.radians(roll)
    pitch = np.radians(pitch)
    yaw = np.radians(yaw)

    Rz = np.array([[np.cos(yaw), -np.sin(yaw), 0],
                   [np.sin(yaw), np.cos(yaw), 0],
                   [0, 0, 1]])

    Ry = np.array([[np.cos(pitch), 0, np.sin(pitch)],
                   [0, 1, 0],
                   [-np.sin(pitch), 0, np.cos(pitch)]])

    Rx = np.array([[1, 0, 0],
                   [0, np.cos(roll), -np.sin(roll)],
                   [0, np.sin(roll), np.cos(roll)]])

    return Rz @ Ry @ Rx

class VectorVisualizer:
    def __init__(self):
        self.fig = plt.figure(figsize=(10, 8))
        self.ax = self.fig.add_subplot(111, projection='3d')

        # 初始化坐标系向量
        self.origin = np.array([0, 0, 0])
        self.x_axis = np.array([1, 0, 0])
        self.y_axis = np.array([0, 1, 0])
        self.z_axis = np.array([0, 0, 1])

        # 绘制初始坐标系
        self.quiver_x = self.ax.quiver(*self.origin, *self.x_axis, color='r', label='X', lw=2)
        self.quiver_y = self.ax.quiver(*self.origin, *self.y_axis, color='g', label='Y', lw=2)
        self.quiver_z = self.ax.quiver(*self.origin, *self.z_axis, color='b', label='Z', lw=2)

        # 设置坐标轴属性
        self.ax.set_xlim([-1.5, 1.5])
        self.ax.set_ylim([-1.5, 1.5])
        self.ax.set_zlim([-1.5, 1.5])
        self.ax.set_xlabel('X')
        self.ax.set_ylabel('Y')
        self.ax.set_zlabel('Z')
        self.ax.view_init(elev=20, azim=30)  # 设置初始视角
        self.ax.legend()
        

    def update(self, angles):
        """更新坐标系可视化"""
        roll, pitch, yaw = angles['roll'], angles['pitch'], angles['yaw']
        
        # 计算旋转矩阵
        R = euler_to_rotation_matrix(roll, pitch, yaw)
        
        # 应用旋转
        new_x = R @ self.x_axis
        new_y = R @ self.y_axis
        new_z = R @ self.z_axis

        # 更新箭头（修正转置错误）
        self.quiver_x.set_segments([np.array([self.origin, new_x])])
        self.quiver_y.set_segments([np.array([self.origin, new_y])])
        self.quiver_z.set_segments([np.array([self.origin, new_z])])
        
        # 重绘图形
        plt.draw()

def show_animate(angles):
    vis = VectorVisualizer()
    # 创建动画
    def animate(frame):
        vis.update({'roll': angles[0][frame], 'pitch': angles[1][frame], 'yaw': angles[2][frame]})
        return (vis.quiver_x, vis.quiver_y, vis.quiver_z)

    ani = FuncAnimation(vis.fig, 
                      animate,
                      frames=range(len(angles[0])),  # 完整旋转360度
                      interval=20,                   # 20ms帧间隔
                      blit=False)                    # 3D图形不支持blitting
    
    plt.show()

# if __name__ == "__main__":
#     angles=[np.arange(0, 360, 1),np.arange(0, 360, 1),np.arange(0, 360, 1)]
#     show_animate(angles)