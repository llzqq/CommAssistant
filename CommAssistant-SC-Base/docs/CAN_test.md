# CAN 测试步骤

目标：让没有接触过 CAN 的人，也能把 Windows 和树莓派的 CAN 硬件接好，并完成 `HELLO` / `ACK` 的最小闭环测试。

## 1. 先认识 CAN

CAN 总线最重要的是两根信号线：

- `CAN_H`
- `CAN_L`

两端设备必须共用同一条 CAN 总线：

- 所有设备的 `CAN_H` 接在一起
- 所有设备的 `CAN_L` 接在一起
- 必要时连接 `GND`

CAN 不是一端 TX、一端 RX 的交叉连接。`CAN_H` 接 `CAN_H`，`CAN_L` 接 `CAN_L`。

## 2. 需要哪些硬件

如果你的场景是 `Windows 电脑 <-> 树莓派`，最小硬件组合是：

- Windows 侧 `USB 转 CAN 适配器` 1 个
- 树莓派侧 `USB 转 CAN 适配器` 1 个
- `CAN_H / CAN_L` 接线
- `120 欧姆` 终端电阻 x2
- 建议再准备一根 `GND` 线

这里按你的最终方案，不使用 CAN HAT。

## 2.1 这些名词是什么意思

- `CAN_H` / `CAN_L`：CAN 总线的两根差分信号线
- `GND`：共地线，建议接上以提高稳定性
- `SLCAN`：把 CAN 帧封装成串口文本命令来传输的一种方式
- `SocketCAN`：Linux 下把 CAN 当成网络接口来用的一种方式

你可以把它理解成：

- Windows 侧的 USB-CAN 适配器，通常通过 `SLCAN` 方式被软件控制
- 树莓派侧的 USB-CAN 适配器，通常在 Linux 下表现为一个串口设备，再通过对应驱动或工具转成可用的 CAN 接口

## 3. 终端电阻怎么接

CAN 总线需要在总线两端各接一个 `120 欧姆` 电阻。

最小两节点测试时：

- 一个终端电阻接在 Windows 侧这一端的 `CAN_H` 和 `CAN_L` 之间
- 另一个终端电阻接在树莓派侧这一端的 `CAN_H` 和 `CAN_L` 之间

如果适配器自带终端电阻开关，先确认是否已经打开。不要重复接太多终端电阻。

## 4. Windows 连接树莓派时，怎么连

最直观的连法是把两边放在同一条 CAN 总线上：

- Windows 适配器 `CAN_H` 接树莓派适配器 `CAN_H`
- Windows 适配器 `CAN_L` 接树莓派适配器 `CAN_L`
- 两边 `GND` 建议相连
- 总线两端各放一个 `120 欧姆` 终端电阻

如果你把它画成图，大概就是：

```text
Windows USB-CAN  ---- CAN_H ---------------- CAN_H ----  树莓派 USB-CAN
Windows USB-CAN  ---- CAN_L ---------------- CAN_L ----  树莓派 USB-CAN
Windows USB-CAN  ---- GND  ----------------- GND  ----  树莓派 USB-CAN
```

注意：

- CAN 不是交叉接线
- `CAN_H` 接 `CAN_H`
- `CAN_L` 接 `CAN_L`
- 只有一端有终端电阻，通常是不够稳定的
- 线先短一点，最容易排查问题

## 5. 软件参数

两端先用同一组 CAN 参数：

- 波特率：`500000`
- 帧类型：标准帧
- 测试 ID：`123`

两端的波特率必须一致。ID 不一致时，也可能出现一端发了但另一端不按预期处理。

## 5.1 两端 Driver 怎么选

Windows 侧：

- 选择 `SLCAN`
- `Channel` 填 USB-CAN 在 Windows 里显示的串口号，例如 `COM4`

树莓派侧：

- 如果 USB-CAN 在树莓派上显示为串口设备，例如 `/dev/ttyUSB0`，选择 `SLCAN`
- 如果 USB-CAN 已经被系统配置成 SocketCAN 接口，例如 `can0`，选择 `SocketCAN`

当前这套“树莓派也使用 USB 转 CAN 适配器”的方案，最常见的是树莓派侧也选 `SLCAN`，通道填 `/dev/ttyUSB0` 或实际识别到的串口设备。

## 6. 测试步骤

1. 接好 `CAN_H`、`CAN_L`、`GND` 和两个 `120 欧姆` 终端电阻
2. 两端都启动 `Comm Assistant`
3. 两端都切到 `CAN` 模式
4. Windows 侧选择 `SLCAN`，通道填 `COMx`
5. 树莓派侧优先选择 `SLCAN`，通道填 `/dev/ttyUSBx`
6. 两端设置相同波特率
7. 两端设置相同测试 ID
8. 两端分别点 `Apply`
9. 任意一端点 `Start Test`
10. 观察是否收到 `ACK`，并显示 `Handshake success`

## 7. 成功时看什么

- 发起端日志出现 `TX ID=0x123 ... HELLO`
- 对端日志出现 `RX ID=0x123 ... HELLO`
- 对端回包日志出现 `TX ... ACK`
- 发起端日志出现 `RX ... ACK`
- 界面状态显示握手成功

## 8. 常见问题

- `Apply` 失败：检查 CAN 适配器驱动、设备通道、是否被其他软件占用
- 没有任何接收：检查 `CAN_H / CAN_L` 是否接反
- 能发送但对端没反应：检查波特率和测试 ID 是否一致
- 通信不稳定：检查终端电阻是否正确，线是否太长
- 树莓派侧不通：先确认树莓派的 USB-CAN 适配器已经被系统识别，例如出现 `/dev/ttyUSB0`
- 树莓派侧选择 `SocketCAN` 失败：先确认系统里是否真的已经有 `can0` 这类接口

## 9. 对新手的建议

先把两只 USB-CAN 适配器直接连起来做闭环。这个场景变量最少，最容易判断是软件问题还是接线问题。

等双 USB-CAN 测通后，再接树莓派侧真实程序去回 `ACK`。
