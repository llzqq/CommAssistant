# 通讯助手架构（MVP）

## 目标
- 基于 `ScriptCommunicator` 保留原有串口/TCP/UDP能力。
- 新增一个独立 `Comm Assistant` 对话框，统一 `RS422/CAN/Ethernet` 的配置和测试入口。
- 首版实现一键配置 + 一键测试（`HELLO/ACK`）+ 统一状态与日志。

## 模块划分
- `mainwindow`：只负责入口，菜单新增 `Comm Assistant`。
- `commassistant/commassistantdialog.*`：通讯助手主 UI 与业务流程。
- `commassistant/icantransport.h`：CAN 抽象接口。
- `commassistant/slcanctransport.*`：Windows 优先的 SLCAN 实现。
- `commassistant/socketcantransport.*`：树莓派/Linux 的 SocketCAN 实现（基于 Qt SerialBus）。

## 数据流
1. 用户选择模式并填写参数。
2. 点击 `Apply`：
   - RS422：打开串口。
   - CAN：初始化 `ICanTransport` 并打开通道。
   - Ethernet：按 TCP/UDP + Client/Server 建链。
3. 点击 `Start Test`：发送 `HELLO`，启动握手超时计时器。
4. 收到数据若包含 `ACK`：握手成功；超时则失败。

## CAN 设计约束
- 仅覆盖最小收发闭环：标准帧/扩展帧 + 文本日志。
- 不引入 CANgaroo 的图表、DBC/LDF、回放等高级视图。
- 通过 `ICanTransport` 预留后续扩展（PCAN/Kvaser/Vector）。
