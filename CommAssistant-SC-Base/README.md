# Comm Assistant

`Comm Assistant` 是基于 `ScriptCommunicator` 的轻量通讯工具，面向 `RS422`、`CAN`、`Ethernet` 三类最小通讯场景。

## 项目特点

- 统一的 `Comm Assistant` 连接入口
- 主界面保留最小可用收发区
- 支持发送历史与接收历史
- 保留 `TCP / UDP / CAN / 串口` 相关底层链路
- 提供 `HELLO / ACK` 最小闭环测试

## 当前界面

- 顶部工具栏：`Comm Assistant`、`Clear`、`Quit`
- 右侧栏：`Send history`、`Receive history`
- 主界面：发送区、接收区、状态栏

## 使用方式

1. 打开 `Comm Assistant`
2. 选择通讯方式并配置参数
3. 点击 `Apply` 建立连接
4. 点击 `Start Test` 验证链路
5. 使用主界面发送区进行实际收发

## 构建

环境和命令见 [docs/build_from_source.md](docs/build_from_source.md)。

## 单文件交付

当前工程支持生成一个可直接交付的单文件启动包，脚本见 `scripts/package_single_exe.ps1`。

## 文档

- [架构说明](docs/architecture.md)
- [测试流程](docs/comm_assistant_test_flow.md)
- [Ethernet 测试](docs/Ethernet_test.md)
- [RS422 测试](docs/RS422_test.md)
- [CAN 测试](docs/CAN_test.md)
- [构建说明](docs/build_from_source.md)
