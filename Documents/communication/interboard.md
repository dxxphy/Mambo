# 板间通信

interboard 模块提供一条面向双板系统的 SPI 报文通道。它不是 ARES 通信抽象的一部分，
也不走 `AresInterface` / `AresProtocol` 绑定链，而是单独维护自己的帧格式、队列和工作队列。

实现位于：

- 头文件：`include/ares/interboard/interboard.h`
- 实现：`lib/ares/interboard/interboard.c`

## 角色

当前代码把 interboard 设计为：

- 固定长度报文
- 主从分工明确
- 以消息队列衔接业务线程
- 以异步 SPI 回调衔接底层设备

适合板间少量控制与状态交换，不适合承载大块可变长数据流。

## 报文格式

核心结构是 `Interboard_Frame`：

- `Frame_Header`
  - `sync`
  - `ctrl`
  - `msg_type`
  - `ack`
- `datas[16]`
- `crc16`
- `end_mark`

固定参数：

- 同步字节：`0x5A`
- 数据长度：16 字节
- 帧尾：`0x0D0A`
- 总缓冲长度：24 字节

## 消息类型

当前定义的消息类型只有两类：

- `INTERBOARD_MSG_CAN`
- `INTERBOARD_MSG_IMU`

接收后按类型分别进入不同消息队列。

## 公开接口

- `calculate_crc16()`
- `build_txbuf()`
- `process_rxbuf()`
- `interboard_init(struct device *spi_dev)`
- `interboard_transceive(Frame_Data *tx_data, Frame_Data *rx_data)`

其余回调和 work handler 虽在头文件中可见，但本质上属于模块内部调度路径。

## 初始化顺序

模块初始化入口是 `interboard_init()`。

它内部执行：

1. 保存 SPI 设备指针。
2. 检查 `device_is_ready()`。
3. 初始化并启动专用 work queue。
4. 启动 1 ms 周期定时器。
5. 从板模式下额外延时后主动提交一次发送 work。

启用条件来自顶层构建：

- `CONFIG_MASTER_BOARD`
- `CONFIG_SLAVE_BOARD`

`lib/ares/CMakeLists.txt` 会在这两个配置任一打开时编译 interboard。

## 主从模型

SPI 配置通过条件编译在同一份源码中切换：

- `CONFIG_MASTER_BOARD`: `SPI_OP_MODE_MASTER`
- `CONFIG_SLAVE_BOARD`: `SPI_OP_MODE_SLAVE`

同时：

- 主板在定时器回调中周期性提交发送 work。
- 从板在收到回调后再回提发送 work。

这表示当前时序更接近“主板驱动轮询、从板跟随响应”的链路。

## 发送与接收路径

### 发送

业务侧把 `Frame_Data` 放入 `interboardtx_msgq`。

发送 work 执行时：

1. 非阻塞尝试从 TX 队列取一条消息。
2. 若取到，则用 `build_txbuf()` 组一帧。
3. 调用 `spi_transceive_cb()` 发起异步收发。

### 接收

SPI 回调成功后：

1. 提交 RX 处理 work。
2. 从板模式下再额外提交 TX work。

RX 处理 work 内：

1. 用 `process_rxbuf()` 校验和拆帧。
2. 按 `msg_type` 投递到 CAN 或 IMU 消息队列。

### 同步调用接口

`interboard_transceive()` 对业务呈现为一个同步调用：

1. 把待发消息放入 TX 队列。
2. 按消息类型阻塞等待对应 RX 队列。
3. 收到响应后返回。

因此它依赖底层周期性收发机制已经在后台运行。

## CRC 与校验

CRC 使用 `zephyr/sys/crc.h` 的 `crc16()`：

- poly: `0x8005`
- init: `0xFFFF`

校验覆盖范围是帧头加 16 字节数据区，不包括 CRC 字段和帧尾。

## 错误边界

当前实现的边界特征如下：

- `process_rxbuf()` 检查同步字节、CRC 和帧尾，不通过时直接返回。
- `interboard_init()` 若 SPI 未 ready，只记录错误并返回，不提供恢复。
- `interboard_transceive()` 默认阻塞等待对应 RX 队列，没有超时保护。
- TX/RX 消息队列写入多为 `K_NO_WAIT`，队列满时调用者拿不到显式恢复逻辑。

维护者尤其要注意最后两点：当前模块默认假定链路稳定、收发节拍稳定。

## 配置与环境要求

使用 interboard 至少需要：

- 一个可用 SPI 设备
- 主板或从板二选一配置
- 能容纳 24 字节帧的 SPI 收发缓冲

如果未来扩展消息类型或负载长度，应优先保持固定帧思路，除非同步修改所有校验、队列和主从节拍逻辑。

## 最小入口

```c
interboard_init((struct device *)spi_dev);

Frame_Data tx = {
	.msg_type = INTERBOARD_MSG_IMU,
};
Frame_Data rx;

interboard_transceive(&tx, &rx);
```

在进入该入口前，系统应已经完成 SPI pinmux、时钟和对应 devicetree 配置。
