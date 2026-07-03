# 图形

## 概览

`drivers/graph/` 当前保存的是早期的 INSLink/RTT 图形支持代码，而非完整的 Zephyr 驱动子系统。该代码已存在于仓库中，但未按照与 `chassis`、`wheel`、`SBUS`、`PID`、`sensor` 相同方式接入常规 `drivers/` 构建链路与公共 API 布局。

相关文件：

- `drivers/graph/graph.h`
- `drivers/graph/inslink.h`
- `drivers/graph/inslink.c`

## 当前代码结构

### `graph.h`

该头文件定义了当前可见的少量公共类型：

- `enum VarType`，包含 `STRING`、`INTEGER`、`FLOAT`、`DOUBLE`、`RAW`
- `feedbackVar`，包含类型、大小和数据指针
- `DT_GRAPH_DEFINE(node_id)` 宏

本子系统在 `include/zephyr/drivers/` 下没有公开的 Zephyr 头文件。

### `inslink.h`

本文件声明了 RTT 图形相关的私有结构：

- `struct rtt_graph_data`
- `struct rtt_graph_cfg`

并前向声明：

- `rtt_graph_init()`
- `rtt_graph_add()`
- `rtt_graph_stop()`

同时定义了一组实例宏，基于：

- `DT_DRV_COMPAT ares_rttgraph`

配置结构期望来自 devicetree 的：

- `name`
- `phy`
- `time_gap_ms`

### `inslink.c`

当前源码：

- 引入 `graph.h`
- 注册日志模块 `insLink`
- 展开 `DT_INST_FOREACH_STATUS_OKAY(RTT_GRAPH_INST)`

但在当前仓内，`rtt_graph_init()`、`rtt_graph_add()`、`rtt_graph_stop()` 的函数体均未提供。

## 设备模型

预期形态似乎是按 `ares_rttgraph` 实例创建 Zephyr 设备，包含：

- `struct rtt_graph_data` 最多 16 个 `feedbackVar` 注册
- 在 `struct rtt_graph_cfg.dev` 中绑定的物理设备
- `time_gap_ms` 的周期采样间隔

该设备模型当前仅部分落地。运行时方法已声明，但在可见源码中尚未实现。

## Devicetree 与公开 API 状态

当前未在 `dts/bindings/` 下提供 `ares_rttgraph` 的匹配绑定。

`include/zephyr/drivers/` 下也没有对应公开 API 头文件。

因此：

- 应用无法获得稳定的文档化接口
- `ares_rttgraph` 节点的 devicetree 校验缺失
- `inslink.h` 引用的属性名缺少模式约束

## 构建与配置状态

`drivers/CMakeLists.txt` 未加入 `drivers/graph/`。

以下文件中也未出现 graph 特定条目：

- `drivers/Kconfig`
- `drivers/CMakeLists.txt`

源码还引用了如下配置项：

- `CONFIG_INS_LINK_LOG_LEVEL`
- `CONFIG_RTT_GRAPH_INIT_PRIO`

这些符号在本文件范围内未被定义。

因此，在实践中 `drivers/graph/` 应视为仓内持续开发中的过渡代码，不应按支持级驱动子系统对待。

## 维护指引

在该子系统完成前，维护应保持保守：

- 避免文档化或依赖未导出的应用侧 API
- 不要新增默认假设这些实例宏已完整可用的用户代码
- 若继续扩展，需先补齐绑定、Kconfig、构建胶水与公共头文件，再把它当作常规驱动使用

## 验证

当前仓内无样例或测试直接覆盖 `drivers/graph/`。

后续启用工作至少应补齐：

- `dts/bindings/` 下的绑定
- `drivers/CMakeLists.txt` 下的构建入口
- 至少一个展示实例创建与变量注册的示例
