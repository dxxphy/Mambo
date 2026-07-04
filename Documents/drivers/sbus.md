# SBUS

## 概览

SBUS 子系统通过异步 UART 后端提供遥控接收器输入。其公开的是一组窄 API，可读取原始数字计数或归一化百分比。

相关文件：

- `include/zephyr/drivers/sbus.h`
- `drivers/transfer/ares_sbus.c`
- `drivers/transfer/ares_sbus.h`
- `dts/bindings/transfer/ares,sbus.yaml`

## 设备模型

公共设备 API 为 `struct sbus_driver_api`，包含两个操作：

- 百分比读取
- 数字/原始通道读取

私有配置（`struct sbus_driver_config`）包含：

- `frameSize`
- `inputChannels`
- `startByte`
- `endByte`

私有运行时状态（`struct sbus_driver_data`）包含：

- 最近一次 25 字节帧
- 16 个已解析通道值
- 丢帧与 `failSafe` 位
- 两个数字通道
- 上次接收时间戳

实现上每个 devicetree 节点对应一个 Zephyr 设备实例，但当前代码仍通过
`DT_CHOSEN(sbus_uart)` 使用单个全局 UART 设备。

## 公共接口

定义于 `include/zephyr/drivers/sbus.h`：

- `sbus_get_percent(dev, channelid)`
- `sbus_get_digit(dev, channelid)`

### 语义

`sbus_get_percent()`：

- 解析指定模拟通道
- 将 SBUS 区间 `[SBUS_MIN, SBUS_MAX]` 映射到约 `[-1.0, 1.0]`
- 以 `1024` 为中位输出

`sbus_get_digit()` 直接返回解析后的通道整数值。

公共头文件未增加方法空实现检查。若作为 SBUS 设备暴露，预期其应实现这两个操作。

## Devicetree 绑定

绑定：

- `dts/bindings/transfer/ares,sbus.yaml`

兼容：

- `ares,sbus`

属性：

- `frame_size`（默认 `25`）
- `input_channels`（默认 `16`）
- `start_byte`（默认 `0x0F`）
- `end_byte`（默认 `0x00`）

除 SBUS 节点本身外，代码还依赖：

- `chosen { sbus_uart = ...; }`

该 `chosen UART` 未在绑定中声明，但当前驱动依赖该项，因为 `ares_sbus.c` 中通过
`DEVICE_DT_GET(DT_CHOSEN(sbus_uart))` 固定引用了 `uart_dev`。

## 构建与配置入口

Kconfig：

- `drivers/Kconfig`
- `drivers/transfer/Kconfig`

相关符号：

- `CONFIG_ARES_SBUS`
- `CONFIG_SBUS_INIT_PRIORITY`
- `CONFIG_SBUS_LOG_LEVEL`

构建入口：

- `drivers/CMakeLists.txt`
- `drivers/transfer/CMakeLists.txt`

当 `CONFIG_ARES_SBUS=y` 时会编译 SBUS 源码。

## 运行时行为

### 初始化

`sbus_init()`：

- 校验 chosen UART 设备就绪
- 将所有通道初始化为中位值 `1024`
- 安装 UART 异步回调
- 从 slab 内存池分配 RX 缓冲
- 按 SBUS 帧参数配置 UART：
  - 波特率 `100000`
  - 偶校验
  - 两个停止位
  - 八位数据位
- 禁用并重新使能 RX

### 接收路径

驱动依赖 Zephyr UART 异步事件：

- `UART_RX_RDY`
- `UART_RX_BUF_REQUEST`
- `UART_RX_BUF_RELEASED`
- `UART_RX_STOPPED`

输入数据进入后，通过 `find_begin()` 扫描有效 25 字节帧，该函数向后回溯查找 `0x0F ... 0x00`。

定位到有效帧后：

- 将 25 字节拷贝到 `data->data`
- 更新 `recv_time`

通道提取在公共 API 被调用时按需由 `sbus_parseframe_chan()` 执行。

### 超时与陈旧性

当距离上次接收超过 100 ms 时，`sbus_parseframe_chan()` 会返回中位值 `1024`。

这意味着输入过期时对上层表现为“摇杆居中”，而不是错误码。

## 错误处理与边界条件

- 若 UART 设备未就绪，初始化返回 `-ENODEV`
- UART 回调安装失败与 RX 使能失败会直接返回给调用方
- 内存 slab 池分配失败在初始化阶段记录日志并返回；在 RX 停止处理中会记录日志并尝试重试
- 非法通道 ID 返回 `1024`
- 过期数据也返回 `1024`

该设计保持了读取 API 的简单性，但同时使调用方无法区分：

- 无效通道号
- 无输入/数据过期
- 真正的中位摇杆值

驱动会解析 `frameLost`、`failSafe` 与两个数字通道到私有状态，但当前无公开 API 可读取这些字段。

## 示例与使用入口

主要使用示例：

- `samples/chassis/src/main.c`

典型覆盖文件：

- `samples/chassis/boards/robomaster_board_c.overlay`

该示例读取：

- 通道 `3` 作为底盘 X 指令
- 通道 `1` 作为底盘 Y 指令
- 通道 `0` 作为偏航指令

仓内当前没有 SBUS 专用测试。
