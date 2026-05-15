# Linux ARM64 使用说明

本文说明如何在树莓派 64 位 Linux 系统上使用 `CommAssistant-linux-arm64.tar.gz`。

适用对象：

- 树莓派 64 位系统
- 其他 `ARM64 / aarch64` Linux 图形环境

本文默认你已经从 GitHub Actions artifact 或 GitHub Release 中拿到了：

```text
CommAssistant-linux-arm64.tar.gz
```

## 1. 先准备目标环境

建议目标机满足以下条件：

- 64 位 Linux
- 有桌面图形环境
- 能打开终端
- 已连接显示器，或者能通过远程桌面看到图形界面

注意：

- 这个包已经自带 Qt 和程序资源。
- 但系统基础库仍然需要目标机自己提供。
- 如果系统过旧，程序可能无法启动。

## 2. 把压缩包传到 Linux 设备

你可以用任意方式把文件传到树莓派，例如：

- U 盘
- 浏览器下载
- `scp`
- WinSCP
- Samba 共享

假设你最终把文件放到了：

```bash
~/Downloads/CommAssistant-linux-arm64.tar.gz
```

## 3. 解压

打开终端，执行：

```bash
cd ~/Downloads
tar -xzf CommAssistant-linux-arm64.tar.gz
```

解压后会得到一个目录：

```text
CommAssistant-linux-arm64
```

进入目录：

```bash
cd CommAssistant-linux-arm64
```

## 4. 给启动脚本加执行权限

首次使用前，执行：

```bash
chmod +x CommAssistant
chmod +x DeleteFolder
```

## 5. 启动程序

在当前目录执行：

```bash
./CommAssistant
```

正常情况下会弹出主界面。

## 6. 首次运行后你会看到什么

当前版本的主界面特点：

- 顶部工具栏只有 `Comm Assistant`、`Clear`、`Quit`
- 右侧只保留 `Send history`、`Receive history`
- 主界面有发送区和接收区

第一次进入时：

- 发送框应为空
- 接收区应为空
- 收发历史应为空

但 `Comm Assistant` 内的配置会保留。

## 7. 如何打开通讯配置窗口

点击顶部工具栏里的：

```text
Comm Assistant
```

然后按你的通讯方式选择：

- `RS422`
- `CAN`
- `Ethernet`

## 8. Linux 下的默认值

为了更适合树莓派，Linux 默认值已经改成：

### RS422

- `Port`：`/dev/ttyUSB0`

### CAN

- `Driver`：`SocketCAN`
- `Channel`：`can0`

如果你的设备名不同，需要改成自己的实际值。

## 9. 三种典型使用方式

### Ethernet

如果你要测试网口通信，建议配合文档：

- [Ethernet_test.md](./Ethernet_test.md)

### RS422

如果你要测试串口通信，建议配合文档：

- [RS422_test.md](./RS422_test.md)

在 Linux 下常见串口名例如：

- `/dev/ttyUSB0`
- `/dev/ttyUSB1`
- `/dev/ttyS0`
- `/dev/ttyAMA0`

### CAN

如果你要测试 CAN，建议配合文档：

- [CAN_test.md](./CAN_test.md)

在 Linux 下推荐优先使用：

- `SocketCAN`
- 接口名如 `can0`

## 10. 常用命令

### 查看串口设备

```bash
ls /dev/ttyUSB*
ls /dev/ttyAMA*
ls /dev/ttyS*
```

### 查看 CAN 接口

```bash
ip link show
```

如果系统里已经启用了 `can0`，通常能在输出里看到它。

### 重新进入程序目录

```bash
cd ~/Downloads/CommAssistant-linux-arm64
./CommAssistant
```

## 11. 常见问题

### 双击没反应

优先改用终端启动：

```bash
./CommAssistant
```

这样可以直接看到错误输出。

### 提示没有执行权限

重新执行：

```bash
chmod +x CommAssistant
chmod +x DeleteFolder
```

### 提示缺少共享库

说明目标系统缺少基础运行库，而不是程序包内部缺少文件。

常见原因：

- 系统太旧
- 图形环境相关库不完整
- `fontconfig`、`dbus`、`xkbcommon`、X11/Wayland 相关库不完整

建议先更新系统，再重试。

### 串口打不开

常见原因：

- 端口名填错
- 串口已被其他程序占用
- 当前用户没有串口权限

可以先查看设备名：

```bash
ls /dev/ttyUSB*
```

如果是权限问题，很多系统可以把当前用户加入串口组后重新登录：

```bash
sudo usermod -aG dialout $USER
```

执行后通常需要注销并重新登录。

### CAN 打不开

常见原因：

- `can0` 还没启用
- 波特率没配好
- 驱动没准备好

先检查：

```bash
ip link show
```

如果没有 `can0`，需要先在系统层把 CAN 设备准备好，再回到软件里连接。

### 程序能打开，但点 Apply 失败

说明主界面本身能启动，问题通常在接口配置或系统权限：

- Ethernet：检查 IP 和端口
- RS422：检查设备名和权限
- CAN：检查 `SocketCAN` 和 `can0`

## 12. 推荐给新手的最短步骤

如果你只是想先把程序跑起来，按下面做：

```bash
cd ~/Downloads
tar -xzf CommAssistant-linux-arm64.tar.gz
cd CommAssistant-linux-arm64
chmod +x CommAssistant DeleteFolder
./CommAssistant
```

打开后再按你的通信方式进入 `Comm Assistant` 配置即可。
