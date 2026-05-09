# CANgaroo 参考索引

本目录只存放参考映射，不混编 CANgaroo 源码。

## 参考来源
- 本地源码：`E:/projects/8院_SAST_地月空间低轨返回气动辅助变轨技术/一期通讯助手/CANgaroo-master/CANgaroo-master`

## 关键对照点
- 驱动初始化组织：
  - `src/mainwindow.cpp` 中 `initDrivers()`
- SLCAN 驱动参考：
  - `src/driver/SLCANDriver/`
- SocketCAN 驱动参考：
  - `src/driver/SocketCanDriver/`

## 本项目采用策略
- 只复用设计思路：`驱动探测/注册 -> 统一收发回调 -> 上抛错误`。
- MVP 不移植 CANgaroo 复杂功能（图表、DBC/LDF、回放、脚本引擎）。
