# Comm Assistant 测试流程

目标：先验证 `Comm Assistant` 三种通讯方式能否完成最小闭环，即 `Apply` 成功、`Start Test` 发送 `HELLO`、对端返回 `ACK`、界面显示 `Handshake success`。

## 通用步骤
- 启动程序，打开 `Config -> Comm Assistant`。
- 选择通讯模式，填写参数。
- 点击 `Apply`，确认 `Connection` 显示为 `Connected`。
- 点击 `Start Test`，确认日志出现 `HELLO` 发送记录。
- 对端返回 `ACK` 后，确认 `Handshake` 显示为 `Handshake success`。

## RS422
- 建议先用两路串口互连，或一端用 RS422 设备、另一端用串口工具模拟对端。
- 参数建议：`115200 / 8 / None / 1`。
- 对端行为：收到 `HELLO` 后回发 `ACK`。
- 判定通过：
  - `Apply` 后串口成功打开。
  - `Last TX` 显示 `HELLO`。
  - `Last RX` 显示 `ACK`。

## Ethernet
- `TCP`：
  - 一端设为 `Server`，监听本地端口。
  - 另一端设为 `Client`，连接该端口。
- `UDP`：
  - 本地和远端 IP/端口填对即可，无需先建连接。
- 对端行为：收到 `HELLO` 后回发 `ACK`。
- 判定通过：
  - `TCP` 模式下 `Apply` 后服务端开始监听，客户端能连接。
  - `UDP` 模式下 `Apply` 后可直接发包。
  - `Handshake` 变为 `Handshake success`。

## CAN
- Windows 侧优先用 `SLCAN`。
- 树莓派/Linux 侧可用 `SocketCAN`。
- 两端波特率一致，首测建议 `500000`。
- 测试 ID 先统一为 `123`，标准帧。
- 对端行为：收到数据区 `HELLO` 的 CAN 帧后，回发数据区为 `ACK` 的 CAN 帧。
- 判定通过：
  - `Apply` 后通道成功打开。
  - 日志区出现 `TX ID=0x123 DLC=5 DATA=48 45 4C 4C 4F`。
  - 收到 `ACK` 后界面显示 `Handshake success`。

## 当前版本注意点
- `HELLO/ACK` 为纯 ASCII 文本，不是正式业务协议。
- `CAN` 当前只覆盖最小收发闭环，不含 DBC/LDF、回放和高级分析。
- `ScriptSound` 已降为占位实现，不影响通讯测试。
