# 握手协议（MVP）

## 协议定义
- 请求：`HELLO`
- 应答：`ACK`
- 编码：ASCII
- 超时：1500 ms（默认）

## 各模式映射
- RS422：串口字节流直接发送 `HELLO`，接收中检测 `ACK`。
- Ethernet TCP/UDP：负载发送 `HELLO`，接收中检测 `ACK`。
- CAN：数据区发送 `HELLO`（最多 5 字节），接收帧数据区检测 `ACK`。

## 判定规则
- `Start Test` 发送成功后进入 `Waiting ACK`。
- 在超时时间内，任意一帧/报文包含 `ACK` 即判定 `Handshake success`。
- 超时未命中则判定 `Handshake timeout`。
