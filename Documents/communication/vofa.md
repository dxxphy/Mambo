# VOFA 支持

ARES 的 VOFA 支持模块当前实现的是一条 JustFloat 风格的串口数据输出通道，
用于快速把若干标量以 float 流形式送到上位机。

实现位于：

- 头文件：`include/ares/vofa/justfloat.h`
- 实现：`lib/ares/vofa/justfloat.c`

## 角色

该模块的定位是轻量调试与观测，而不是：

- 通用通信协议
- 带确认的控制链路
- 多客户端可仲裁输出

它直接面向一个 UART 设备和一组待观察变量。

## 核心对象

核心状态结构是 `struct JFData`，其中维护：

- `uart_dev`
- 每个通道的原始指针
- 每个通道的 float 缓冲
- 每个通道的类型
- 当前通道数

通道上限由 `aresMaxChannel` 固定为 24。

## 公开接口

- `struct JFData *jf_send_init(const struct device *uart_dev, int delay)`
- `void jf_channel_add(struct JFData *data, void *value, enum JF_Types type)`

支持的输入类型见 `enum JF_Types`，包括：

- `PTR_INT`
- `PTR_FLOAT`
- `PTR_DOUBLE`
- `PTR_INT8`
- `PTR_INT16`
- `PTR_UINT8`
- `PTR_UINT16`
- `PTR_UINT`
- `RAW`

## 初始化顺序

标准用法：

1. 调用 `jf_send_init()` 绑定一个 UART 设备并启动后台线程/定时器。
2. 逐个调用 `jf_channel_add()` 注册观测项。

`jf_send_init()` 会：

- 检查 UART 设备是否 ready
- 清零全局 `aresPlotData`
- 创建发送线程
- 启动周期定时器

它返回的是一个全局静态对象指针，而不是可重复实例化的独立上下文。

## 运行时行为

### 定时触发

定时器周期到达后，只做一件事：

- `k_sem_give(&jf_sem)`

后台线程被唤醒后：

1. 遍历已注册通道。
2. 按声明类型取值并转换为 `float`。
3. 在末尾追加固定 tail。
4. 通过 `uart_tx()` 发出连续 float 字节流。

### 尾标记

模块固定使用四字节 tail：

- `00 00 80 7f`

并在必要时避免通道值与 tail 模式冲突。

### RAW 通道

`RAW` 类型表示该通道直接使用当前 `float` 数据，不再通过指针回读。
这适合外部周期性覆写 `fdata[]` 的场景。

## 配置与线程

VOFA 模块当前没有独立 Kconfig，线程栈和节拍控制主要体现在源码常量与 `jf_send_init()` 的
`delay` 参数中。

运行对象包括：

- 一个后台线程
- 一个周期定时器
- 一个全局信号量

因此它更像单例服务，不是可并列部署的多路实例框架。

## 错误边界

- UART 设备未 ready：`jf_send_init()` 返回 `NULL`
- 通道数超过 24：新通道不会被正确纳入设计上限内的布局
- 输出使用全局静态对象，多个调用点重复初始化会互相覆盖
- 模块默认假设 `uart_tx()` 可直接承载该输出节拍，不做反压处理

维护者若要把它用于长期稳定采集，首先要解决的通常不是协议格式，而是实例隔离和发送仲裁。

## 适用场景

适合：

- 快速看姿态、角速度、控制量
- 在算法 bring-up 阶段临时导出几个标量

不适合：

- 多任务共享一条调试口
- 需要结构化字段或版本管理的上位机协议

## 最小入口

```c
struct JFData *jf = jf_send_init(uart_dev, 10);

jf_channel_add(jf, &roll, PTR_FLOAT);
jf_channel_add(jf, &pitch, PTR_FLOAT);
jf_channel_add(jf, &yaw, PTR_FLOAT);
```

初始化完成后，线程会按给定周期持续输出这组值。
