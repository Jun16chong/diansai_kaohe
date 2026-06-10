"""
K230 可见光定位 — 从 K210 MaixPy 移植到 K230 CanMV
硬件: 庐山派 + GC2093 + ST7701 MIPI 屏 (800x480)
功能: 摄像头识别 LED 光斑 → 三灯定位 → 频率检测 → 坐标/角度/区域 → UART 发送 MSPM0G

原 K210 代码: D:\online_learning\电赛\可见光定位源码\K210可见光定位.py
移植日期: 2026-06-08
"""

# === K230 导入 (替代 K210 的 import sensor, image, lcd) ===
from media.sensor import *       # Sensor 摄像头
from media.display import *      # Display ST7701 MIPI 屏
from media.media import *        # MediaManager 媒体管线
import image                     # 图像处理 (find_blobs, draw_xxx)
import math
import time
import os                        # os.exitpoint() IDE Ctrl+C 中断
import gc                        # 垃圾回收
import struct                    # UART 帧打包

from machine import Timer        # 软定时器 Timer(-1)
from machine import UART, FPIOA  # UART 通信 + 引脚复用

# === FPIOA 配置 UART2 (GPIO11=TXD, GPIO12=RXD) 发 MSPM0G ===
fpioa = FPIOA()
fpioa.set_function(11, FPIOA.UART2_TXD)
fpioa.set_function(12, FPIOA.UART2_RXD)
uart = UART(UART.UART2, baudrate=115200,
            bits=UART.EIGHTBITS, parity=UART.PARITY_NONE, stop=UART.STOPBITS_ONE)

# === 显示屏参数 ===
SW, SH = 800, 480           # ST7701 屏幕尺寸
CW, CH = 320, 240           # QVGA 摄像头帧尺寸
OFFSET_X = (SW - CW) // 2   # 320x240 在 800x480 上居中: x=240
OFFSET_Y = (SH - CH) // 2   # y=120

# === LAB 阈值: 白色 LED 光斑 (现场必须重校准!) ===
my_threshold = [92, 100, 101, -49, -41, 127]

# my_limit 预留 (原代码定义但未使用, 保留)
my_limit = [15, 15, 15, 15, 15, 15]

# === 全局变量 ===
Time = 0
Time2 = 0
flag = 1
Time_Achanges = 0
Time_Bchanges = 0
Time_Cchanges = 0

# === 闪烁计数 (发挥部分2: 接收LED发送的数字) ===
# 状态机: IDLE → LISTENING → DECODED
blink_state = 0         # 0=IDLE, 1=LISTENING, 2=DECODED
blink_led = 0            # 当前在闪烁的LED编号 (1=A, 2=B, 3=C)
blink_count_A = 0        # A灯闪烁计数
blink_count_B = 0        # B灯闪烁计数
blink_count_C = 0        # C灯闪烁计数
blink_last_A = False     # A灯上一帧是否在低亮度
blink_last_B = False     # B灯上一帧是否在低亮度
blink_last_C = False     # C灯上一帧是否在低亮度
blink_stable_cnt = 0     # 连续稳定帧计数 (用于判断闪烁结束)
decoded_num_A = -1       # 解码出的数字 (-1=未收到)
decoded_num_B = -1
decoded_num_C = -1

# === 函数: 取 array1 最大 3 个值在 array2 中的索引 ===
# NOTE: 原 K210 代码中 sorted(array1) 未赋值, array1 实际未排序
# 此处保留相同行为以确保算法一致
def maymax(array1, array2):
    uu = [0, 0, 0]
    sorted(array1)
    if len(array1) >= 3:
        for num in range(0, 3):
            for u in range(0, len(array1)):
                if array1[len(array1) - num - 1] == array2[u]:
                    uu[num] = u
                    break
    return uu

# === 定时器回调 ===
def on_timer(timer):
    global Time
    Time = Time + 1

def on_timer2(timer):
    global Time2
    Time2 = 1

# === 区域判断: 坐标 → A/B/C/D/E 区域 ===
def Area(x, y):
    if x > (-20) and x < 20 and y > (-20) and y < 20:
        return 'A'
    if y > 20 and y < 40 and x > (-y) and x < y:
        return 'B'
    if x > 20 and x < 40 and y < x and y > (-x):
        return 'C'
    if y > (-40) and y < (-20) and x > y and x < (-y):
        return 'D'
    if x > (-40) and x < (-20) and y < (-x) and y > x:
        return 'E'

# === UART 帧发送 (10 字节协议) ===
# 帧格式: 0xA5 | 0x5A | CMD | X(2B, int16) | Y(2B, int16) | EXTRA(2B) | CHKSUM
# CMD: 0x01=光斑坐标, 0x04=目标丢失
def uart_send_position(cmd, x, y, extra=0):
    buf = bytearray(10)
    buf[0] = 0xA5
    buf[1] = 0x5A
    buf[2] = cmd
    struct.pack_into('<h', buf, 3, int(x * 10))   # X 坐标 (0.1cm 单位)
    struct.pack_into('<h', buf, 5, int(y * 10))   # Y 坐标 (0.1cm 单位)
    struct.pack_into('<h', buf, 7, int(extra))
    buf[9] = sum(buf[:9]) & 0xFF ^ 0xFF           # 校验: 前9字节异或取反
    uart.write(buf)

# === 软定时器 (K230 硬件定时器 0-5 不可用, 用 Timer(-1)) ===
# tim: 100ms 定时器, 初始不启动 (原 K210 start=False)
#      在 LED 频率检测时动态 start/stop → K230 用 init/deinit
tim = Timer(-1)

# tim1: 500ms 定时器, 初始启动 (原 K210 start=True)
tim1 = Timer(-1)
tim1.init(period=500, mode=Timer.PERIODIC, callback=on_timer2)

# === 变量初始化 (字典改为列表, 兼容 K230 整数索引) ===
c = 0
s = []        # blob 面积列表
ss = []       # blob 面积副本
x = []        # blob 中心 X 坐标
y = []        # blob 中心 Y 坐标
AS = 0.01     # A 灯上一帧面积 (初始小值防除零)
BS = 0.01
CS = 0.01
AS2 = 0
BS2 = 0
CS2 = 0

# === K230 初始化顺序 (铁律!) ===
# 1. Sensor 构造 + reset
sensor = Sensor(id=2)          # 庐山派 GC2093 在 CSI 2 接口
sensor.reset()

# 2. 设置分辨率和像素格式
sensor.set_framesize(width=CW, height=CH)   # QVGA 320x240 高帧率
sensor.set_pixformat(Sensor.RGB565)

# 3. 画面镜像 (原 lcd.rotation(0) 的等效)
sensor.set_hmirror(False)
sensor.set_vflip(False)

# 4. Display 初始化 (MIPI ST7701)
Display.init(Display.ST7701, width=SW, height=SH)

# 5. MediaManager 初始化
MediaManager.init()

# 6. 启动摄像头
sensor.run()

# 跳过前 30 帧 (原 sensor.skip_frames(30), K230 用循环替代)
for _ in range(30):
    sensor.snapshot()
    time.sleep_ms(10)

# === 帧率统计 ===
clock = time.clock()
fc = 0
last_blobs = []
last_send_ms = time.ticks_ms()

# === 主循环 ===
while True:
    os.exitpoint()              # K230 必须: 允许 IDE Ctrl+C 中断
    clock.tick()
    fc += 1

    img = sensor.snapshot()     # 抓帧 QVGA 320x240

    # 隔帧检测优化帧率 (800x480 直出场景, 降低计算压力)
    if fc % 2 == 0:
        blobs = img.find_blobs([my_threshold],
                                roi=(0, 0, CW, CH),
                                x_stride=2, y_stride=2,
                                pixels_threshold=5, area_threshold=10,
                                merge=False, margin=0)
    else:
        blobs = last_blobs
    last_blobs = blobs

    if blobs:
        # --- 收集所有检测到的光斑 ---
        for b in blobs:
            tmp = img.draw_rectangle(b.x(), b.y(), b.w(), b.h(),
                                      color=(255, 255, 0))   # 黄色外接框

            x.append(b.cx())          # b[5] → b.cx()
            y.append(b.cy())          # b[6] → b.cy()
            s.append(b.pixels())      # b[4] → b.pixels()
            ss.append(b.pixels())     # b[4] → b.pixels()
            c = c + 1

        # === 单灯模式 ===
        if len(ss) == 1:
            x1 = x[0]
            y1 = y[0]
            xx = (x1 - 160) / 5 + 3.3
            yy = -(y1 - 120) / 5 + 9
            tmp = img.draw_cross(x1, y1, color=(255, 255, 0))   # 十字标

            tmp = img.draw_string(3, 3, "(%.1f,%.1f)" % (xx, yy),
                                   color=(0, 0, 255), scale=2)
            tmp = img.draw_string(3, 30, "area=%s" % Area(xx, yy),
                                   color=(0, 0, 255), scale=2)

            # UART 发送单灯坐标
            now = time.ticks_ms()
            if time.ticks_diff(now, last_send_ms) > 50:
                last_send_ms = now
                uart_send_position(0x01, xx, yy, 1)  # extra=灯数

        # === 双灯模式 ===
        if len(ss) == 2:
            x1 = x[0]
            y1 = y[0]
            x2 = x[1]
            y2 = y[1]
            x4 = (x1 + x2) / 2
            y4 = (y1 + y2) / 2

            xx = (x4 - 160) / 4.8
            yy = -(y4 - 120) / 5 - 5

            tmp = img.draw_cross(x1, y1, color=(255, 0, 0))
            tmp = img.draw_cross(x2, y2, color=(255, 0, 0))
            tmp = img.draw_string(3, 3, "(%.1f,%.1f)" % (xx, yy),
                                   color=(0, 0, 255), scale=2)
            tmp = img.draw_string(3, 30, "area=%s" % Area(xx, yy),
                                   color=(0, 0, 255), scale=2)

            # UART 发送双灯中点坐标
            now = time.ticks_ms()
            if time.ticks_diff(now, last_send_ms) > 50:
                last_send_ms = now
                uart_send_position(0x01, xx, yy, 2)  # extra=灯数

        # === 三灯及以上模式 (定位核心算法) ===
        if len(ss) >= 3:
            # 取面积最大的 3 个光斑
            x1 = x[maymax(s, ss)[0]]
            y1 = y[maymax(s, ss)[0]]
            S1 = ss[maymax(s, ss)[0]]

            x2 = x[maymax(s, ss)[1]]
            y2 = y[maymax(s, ss)[1]]
            S2 = ss[maymax(s, ss)[1]]

            x3 = x[maymax(s, ss)[2]]
            y3 = y[maymax(s, ss)[2]]
            S3 = ss[maymax(s, ss)[2]]

            # 三灯两两距离
            d12 = math.pow((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2), 0.5)
            d13 = math.pow((x1 - x3) * (x1 - x3) + (y1 - y3) * (y1 - y3), 0.5)
            d23 = math.pow((x2 - x3) * (x2 - x3) + (y2 - y3) * (y2 - y3), 0.5)

            # 6 种距离排列情况 → 分配 A/B/C 灯标签
            # A: 最近两灯之一(与B配对), B/C: 较远两灯
            # AS/BS/CS 在每种情况中都会被更新为当前光斑面积
            if d12 < d13 and d13 < d23:          # 情况1
                Ax, Ay, AS = x1, y1, S1
                Bx, By, BS = x2, y2, S2
                Cx, Cy, CS = x3, y3, S3
            if d13 < d12 and d12 < d23:          # 情况2
                Ax, Ay, AS = x1, y1, S1
                Bx, By, BS = x3, y3, S3
                Cx, Cy, CS = x2, y2, S2
            if d12 < d23 and d23 < d13:          # 情况3
                Ax, Ay, AS = x2, y2, S2
                Bx, By, BS = x1, y1, S1
                Cx, Cy, CS = x3, y3, S3
            if d13 < d23 and d23 < d12:          # 情况4
                Ax, Ay, AS = x3, y3, S3
                Bx, By, BS = x1, y1, S1
                Cx, Cy, CS = x2, y2, S2
            if d23 < d13 and d13 < d12:          # 情况5
                Ax, Ay, AS = x3, y3, S3
                Bx, By, BS = x2, y2, S2
                Cx, Cy, CS = x1, y1, S1
            if d23 < d12 and d12 < d13:          # 情况6
                Ax, Ay, AS = x2, y2, S2
                Bx, By, BS = x3, y3, S3
                Cx, Cy, CS = x1, y1, S1

            # === LED 闪烁检测 & 数字解码 (所有情况通用, 不再局限于情况6) ===
            # 注: 原代码频率检测缩进在情况6内部 → 已修复, 移到此处对所有情况生效
            AS_changes = abs(AS - AS2) / AS if AS > 0 else 0
            BS_changes = abs(BS - BS2) / BS if BS > 0 else 0
            CS_changes = abs(CS - CS2) / CS if CS > 0 else 0
            AS2 = AS
            BS2 = BS
            CS2 = CS

            # 闪烁边沿检测: 面积变化 >15% 且从亮→暗 = 一次闪烁开始
            DARK_THRESH = 0.15
            STABLE_FRAMES = 15      # 连续稳定帧数, 判定闪烁结束

            # A灯边沿检测
            a_dark_now = (AS_changes > DARK_THRESH and AS < AS2)
            if a_dark_now and not blink_last_A:
                blink_count_A += 1          # 上升沿 (亮→暗), 计数+1
            blink_last_A = a_dark_now

            # B灯边沿检测
            b_dark_now = (BS_changes > DARK_THRESH and BS < BS2)
            if b_dark_now and not blink_last_B:
                blink_count_B += 1
            blink_last_B = b_dark_now

            # C灯边沿检测
            c_dark_now = (CS_changes > DARK_THRESH and CS < CS2)
            if c_dark_now and not blink_last_C:
                blink_count_C += 1
            blink_last_C = c_dark_now

            # 判断哪个灯在闪烁 (任一灯的计数在增长)
            any_blinking = (a_dark_now or b_dark_now or c_dark_now or
                            blink_last_A or blink_last_B or blink_last_C)

            if any_blinking:
                blink_stable_cnt = 0
                if blink_state == 0:
                    blink_state = 1     # 进入监听状态
            else:
                blink_stable_cnt += 1

            # 连续稳定 STABLE_FRAMES 帧 → 闪烁结束, 锁存解码结果
            if blink_state == 1 and blink_stable_cnt >= STABLE_FRAMES:
                blink_state = 2         # 解码完成

                if blink_count_A > 0:
                    decoded_num_A = blink_count_A
                if blink_count_B > 0:
                    decoded_num_B = blink_count_B
                if blink_count_C > 0:
                    decoded_num_C = blink_count_C

                # 解码结果通过 UART 发送 (CMD=0x02: 数字信息)
                dec_num = decoded_num_A if decoded_num_A >= 0 else (
                          decoded_num_B if decoded_num_B >= 0 else decoded_num_C)
                if dec_num >= 0:
                    uart_send_position(0x02, dec_num, 0, 0)

                # 重置计数器, 准备下一次接收
                blink_count_A = 0
                blink_count_B = 0
                blink_count_C = 0
                blink_last_A = False
                blink_last_B = False
                blink_last_C = False
                blink_state = 0

            # === 绘制闪烁检测和解码结果 ===
            tmp = img.draw_string(3, 140, "Blink A:%d B:%d C:%d" %
                                   (blink_count_A, blink_count_B, blink_count_C),
                                   color=(0, 255, 0), scale=2)
            if decoded_num_A >= 0:
                tmp = img.draw_string(3, 160, "NUM_A=%d" % decoded_num_A,
                                       color=(255, 255, 0), scale=2)
            if decoded_num_B >= 0:
                tmp = img.draw_string(3, 182, "NUM_B=%d" % decoded_num_B,
                                       color=(255, 255, 0), scale=2)
            if decoded_num_C >= 0:
                tmp = img.draw_string(3, 204, "NUM_C=%d" % decoded_num_C,
                                       color=(255, 255, 0), scale=2)

            # 绘制三灯十字标
            tmp = img.draw_cross(x1, y1, color=(255, 0, 0))
            tmp = img.draw_cross(x2, y2, color=(255, 0, 0))
            tmp = img.draw_cross(x3, y3, color=(255, 0, 0))

            # 绘制 A/B/C 标签
            tmp = img.draw_string(Ax, Ay, "A", color=(255, 0, 0), scale=2)
            tmp = img.draw_string(Bx, By, "B", color=(255, 0, 0), scale=2)
            tmp = img.draw_string(Cx, Cy, "C", color=(255, 0, 0), scale=2)

            # === B/C 中点偏移 + AB 向量角度 → 世界坐标 (XX, YY) ===
            x0 = (Cx + Bx) / 2
            y0 = (Cy + By) / 2

            xx = (x0 - 160) / 4.9
            yy = -(y0 - 120) / 4.9
            Xab = Bx - Ax
            Yab = By - Ay
            # 使用 atan2 替代 atan：自动处理所有象限和除零问题
            angle = -(math.atan2(Yab, Xab) * 180) / math.pi
            # 归一化到 [-180, 180]
            if angle > 180:
                angle = angle - 360
            if angle < -180:
                angle = angle + 360
            angle2 = angle * math.pi / 180

            tmp = img.draw_string(160, 30, "angle=%d" % angle,
                                   color=(0, 0, 255), scale=2)

            # B/C 中点十字
            tmp = img.draw_cross(round((Cx + Bx) / 2), round((Cy + By) / 2),
                                  color=(255, 255, 0))

            # 每 500ms 更新世界坐标计算
            if Time2 == 1:
                Time2 = 0
                R = math.sqrt(xx * xx + yy * yy)
                if R > 0.01:        # 避免除零 (替代旧 yy != 0 检查)
                    theta = math.atan2(xx, yy)   # atan2 自动处理所有象限
                    XX = -R * math.sin(angle2 + theta)
                    YY = -R * math.cos(angle2 + theta)

            # 根据 y0 位置分上下半屏显示坐标 (原 K210 逻辑)
            if y0 > 120:
                if YY >= 6:
                    tmp = img.draw_string(3, 3, "(%.1f,%.1f)" % (XX, YY + 3),
                                           color=(0, 0, 255), scale=2)
                    tmp = img.draw_string(3, 30, "area=%s" % Area(XX, YY + 3),
                                           color=(0, 0, 255), scale=2)
                else:
                    tmp = img.draw_string(3, 3, "(%.1f,%.1f)" % (XX, YY + 1),
                                           color=(0, 0, 255), scale=2)
                    tmp = img.draw_string(3, 30, "area=%s" % Area(XX, YY + 1),
                                           color=(0, 0, 255), scale=2)
            if y0 <= 120:
                if YY < -6:
                    tmp = img.draw_string(3, 3, "(%.1f,%.1f)" % (-XX, -YY + 3),
                                           color=(0, 0, 255), scale=2)
                    tmp = img.draw_string(3, 30, "area=%s" % Area(-XX, -YY + 3),
                                           color=(0, 0, 255), scale=2)
                else:
                    tmp = img.draw_string(3, 3, "(%.1f,%.1f)" % (-XX, -YY + 1),
                                           color=(0, 0, 255), scale=2)
                    tmp = img.draw_string(3, 30, "area=%s" % Area(-XX, -YY + 1),
                                           color=(0, 0, 255), scale=2)

            # UART 发送三灯世界坐标
            now = time.ticks_ms()
            if time.ticks_diff(now, last_send_ms) > 50:
                last_send_ms = now
                uart_send_position(0x01, XX, YY, 3)  # extra=灯数

        # === 帧尾: 重置列表 ===
        c = 0
        s = []
        ss = []
        x = []
        y = []

        # 屏幕中心十字
        tmp = img.draw_cross(160, 120, color=(255, 100, 0))

    else:
        # 无光斑: 发送目标丢失
        now = time.ticks_ms()
        if time.ticks_diff(now, last_send_ms) > 200:
            last_send_ms = now
            uart_send_position(0x04, 0, 0, 0)

    # === 显示: QVGA 帧居中到 ST7701 800x480 ===
    Display.show_image(img, x=OFFSET_X, y=OFFSET_Y)

    # === 周期性 GC (防内存泄漏和周期性卡顿) ===
    if fc % 300 == 0:
        gc.collect()
