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

        # 新增位移参数
        self.position = np.array([0.0, 0.0, 0.0])  # 初始位置
        
        # 绘制位移轨迹
        self.trajectory, = self.ax.plot([], [], [], 'g--', lw=0.5)
        self.history = []  # 轨迹记录

    def update(self, angles, displacement):
        """更新坐标系可视化"""
        roll, pitch, yaw = angles['roll'], angles['pitch'], angles['yaw']
        
        # 解析位移参数
        dx, dy, dz = displacement['x'], displacement['y'], displacement['z']
        self.position = np.array([dx, dy, dz])

        # 计算旋转矩阵
        R = euler_to_rotation_matrix(roll, pitch, yaw)
        
        # 应用旋转和平移
        new_origin = self.position
        new_x = new_origin + R @ self.x_axis
        new_y = new_origin + R @ self.y_axis
        new_z = new_origin + R @ self.z_axis
        # new_x = R @ self.x_axis
        # new_y = R @ self.y_axis
        # new_z = R @ self.z_axis

        # 更新箭头（修正转置错误）
        self.quiver_x.set_segments([np.array([self.position, new_x])])
        self.quiver_y.set_segments([np.array([self.position, new_y])])
        self.quiver_z.set_segments([np.array([self.position, new_z])])
         # 更新轨迹
        self.history.append(new_origin.copy())
        if len(self.history) > 100:  # 保留最近100个点
            self.history.pop(0)
        trajectory_data = np.array(self.history)
        self.trajectory.set_data(trajectory_data[:,0], trajectory_data[:,1])
        self.trajectory.set_3d_properties(trajectory_data[:,2])

        # 自动调整坐标范围
        self.ax.set_xlim([new_origin[0]-2, new_origin[0]+2])
        self.ax.set_ylim([new_origin[1]-2, new_origin[1]+2])
        self.ax.set_zlim([new_origin[2]-2, new_origin[2]+2])
        
        # 重绘图形
        plt.draw()

def show_animate(angles, displacements):
    vis = VectorVisualizer()
    # 创建动画
    def animate(frame):
        angle_data = {
            'roll': angles['roll'][frame],
            'pitch': angles['pitch'][frame],
            'yaw': angles['yaw'][frame]
        }
        disp_data = {
            'x': displacements['x'][frame],
            'y': displacements['y'][frame],
            'z': displacements['z'][frame]
        }
        vis.update(angles=angle_data, displacement=disp_data)
        return (vis.quiver_x, vis.quiver_y, vis.quiver_z)

    ani = FuncAnimation(vis.fig, 
                      animate,
                      frames=range(len(angles['roll'])),  # 完整旋转360度
                      interval=20,                   # 20ms帧间隔
                      blit=False)                    # 3D图形不支持blitting
    
    plt.show()

if __name__ == "__main__":
    num_frames = 360
    t = np.linspace(0, 4*np.pi, num_frames)
   # 旋转角度参数
    angles = {
        'roll': 10*np.sin(t),
        'pitch': 15*np.cos(0.5*t),
        'yaw': 0*np.linspace(0, 360, num_frames)
    }
    
    displacements = {
        'x': 0.1*np.sin(t),
        'y': 0.1*np.cos(t),
        'z': np.zeros(len(t))
    }
    
    show_animate(angles, displacements)