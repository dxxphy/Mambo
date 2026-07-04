# 通信协议与 ARES 库

本目录收录 Mambo 中面向上位机链路、板间通信、调试输出和 ARES 支撑库的维护文档。
代码仍位于 `include/ares/` 与 `lib/ares/`；文档入口使用“通信协议”命名，便于从目录上识别职责。

本文档面向维护者。重点说明模块职责、公开接口、初始化关系、配置入口和运行边界，
不重复实现细节，也不记录开发过程。

## 目录

- [接口层](./interface.md)
  - `AresInterface`
  - UART 接口
  - USB 批量传输接口
- [协议层](./protocol.md)
  - `AresProtocol`
  - 双向协议
  - 绘图协议
  - 历史 `usb_trans` 语义
  - 历史 UART 上下位机协议
- [算法层](./algorithms.md)
  - 四元数 EKF
  - Mahony AHRS
  - 数学工具
  - 矩阵存储
  - IMU 任务
- [板间通信](./interboard.md)
  - SPI 双板报文
  - 主从时序
  - 收发队列
- [板级支持](./board.md)
  - 电源使能
  - 状态 LED
  - 板级初始化
- [VOFA 支持](./vofa.md)
  - JustFloat 串口输出

## 结构概览

通信相关代码主要分布在两个层面：

- `include/ares/` 暴露对外可包含的头文件。
- `lib/ares/` 提供实现、Kconfig 和按功能拆分的子目录。

大致分层如下：

1. 接口层负责收发字节流，并向协议层提供缓冲分配与发送入口。
2. 协议层负责把字节流解释为帧，并将高层操作映射到接口发送。
3. 算法层独立于通信层，处理 IMU 采样、姿态估计和标定数据存储。
4. interboard 模块处理板间 SPI 报文。
5. 历史 UART 协议文档记录早期上下位机链路帧格式。
6. board 与 VOFA 模块分别提供板级公共外设和轻量级调试输出路径。

## 绑定模型

ARES 通信栈围绕两个基础结构工作：

- `struct AresInterface`
- `struct AresProtocol`

它们通过 `ares_bind_interface()` 绑定。绑定过程有固定顺序：

1. 检查 `interface` 与 `protocol` 非空。
2. 互相写入对方指针。
3. 先执行 `interface->api->init(interface)`。
4. 再执行 `protocol->api->init(protocol)`。

这一顺序很重要。协议初始化通常假定底层设备、线程或缓冲池已经可用。

## 配置入口

ARES 顶层配置位于 `lib/ares/Kconfig`，主要开关包括：

- `CONFIG_ARES_COMM_LIB`
- `CONFIG_UART_INTERFACE`
- `CONFIG_USB_BULK_INTERFACE`
- `CONFIG_DUAL_PROPOSE_PROTOCOL`
- `CONFIG_ARES_PLOTTER_PROTOCOL`
- `CONFIG_PLOTTER`
- `CONFIG_EKF_LIB`
- `CONFIG_MAHONY_LIB`
- `CONFIG_VOFA_LIB`
- `CONFIG_MASTER_BOARD`
- `CONFIG_SLAVE_BOARD`
- `CONFIG_ARES_BOARD_STATUS_LED`

`lib/ares/CMakeLists.txt` 会按这些配置裁剪子目录，维护者排查“代码存在但未编译”
的问题时应先看这里。

## 维护边界

维护通信与 ARES 支撑库时应优先保持以下约束：

- 接口层只处理收发、线程和缓冲，不引入协议语义。
- 协议层只依赖 `AresInterfaceAPI`，不直接侵入具体 UART/USB 细节。
- 算法层不依赖通信层；调试与可视化通过外部回调或 VOFA/plotter 接出。
- 板级代码按板型提供同名实现，公共调用点只依赖统一函数名。

## 典型入口

最小通信入口通常长这样：

```c
#include <ares/ares_comm.h>
#include <ares/interface/uart/uart.h>
#include <ares/protocol/dual/dual_protocol.h>
```

随后：

1. 定义接口实例和协议实例。
2. 先把设备句柄灌入接口私有数据。
3. 调用 `ares_bind_interface()`。
4. 再注册功能表、同步表或协议私有回调。

具体顺序和注意事项见各分册。
