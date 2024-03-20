import pygame
from math import sin, cos, radians

# 顶点旋转函数
def rotate_vertex(vertex, rotation_x, rotation_y):
    # 绕X轴旋转
    x = vertex[0]
    y = vertex[1] * rotation_x[1][1] + vertex[2] * rotation_x[1][2]
    z = vertex[1] * rotation_x[2][1] + vertex[2] * rotation_x[2][2]

    # 绕Y轴旋转
    x = x * rotation_y[0][0] + z * rotation_y[0][2]
    y = y * rotation_y[1][1]
    z = x * rotation_y[2][0] + z * rotation_y[2][2]

    return [x, y, z]


# 初始化Pygame
pygame.init()

# 设置显示窗口
width, height = 800, 600
screen = pygame.display.set_mode((width, height))
pygame.display.set_caption("Rotating Cube")

# 定义立方体顶点
vertices = [
    [-1, -1, -1], [1, -1, -1], [1, 1, -1], [-1, 1, -1],
    [-1, -1, 1], [1, -1, 1], [1, 1, 1], [-1, 1, 1]
]

# 定义立方体面和对应的颜色
faces = [
    ([0, 1, 2, 3], (255, 0, 0)),   # 红色
    ([1, 5, 6, 2], (0, 255, 0)),   # 绿色
    ([5, 4, 7, 6], (0, 0, 255)),   # 蓝色
    ([4, 0, 3, 7], (255, 255, 0)), # 黄色
    ([0, 4, 5, 1], (255, 0, 255)), # 紫色
    ([3, 2, 6, 7], (0, 255, 255))  # 青色
]

# 设置相机位置和观察方向
camera_position = [0, 0, -5]
target = [0, 0, 0]

# 初始旋转角度
angle = 0

# 游戏循环
running = True
while running:
    # 处理事件
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    # 清空屏幕
    screen.fill((0, 0, 0))

    # 绘制立方体
    angle += 1  # 旋转角度增加
    for face, color in faces:
        # 计算旋转矩阵
        rotation_x = [[1, 0, 0],
                      [0, cos(radians(angle)), -sin(radians(angle))],
                      [0, sin(radians(angle)), cos(radians(angle))]]
        rotation_y = [[cos(radians(angle)), 0, sin(radians(angle))],
                      [0, 1, 0],
                      [-sin(radians(angle)), 0, cos(radians(angle))]]

        # 将顶点坐标旋转
        projected_vertices = [rotate_vertex(vertex, rotation_x, rotation_y) for vertex in [vertices[i] for i in face]]

        # 绘制多边形
        pygame.draw.polygon(screen, color, [
            [width // 2 + (vertex[0] - camera_position[0]) * 100,
             height // 2 + (vertex[1] - camera_position[1]) * 100]
            for vertex in projected_vertices
        ], 0)

    # 更新显示
    pygame.display.flip()

# 退出Pygame
pygame.quit()