# VCAN 实现说明

本文记录 `drivers/vcan` 的实现边界、同步协议和维护注意事项。使用方法见
[readme.md](readme.md)。

## 文件职责

### `spi_can_mfd.c`

父设备。每个实例对应一个 SPI-CAN 桥。

职责：

- 管理 SPI 事务。
- 维护两个通道的 TX 队列和状态缓存。
- 构造 Host 同步帧。
- 解析 Slave 同步帧。
- 调度轮询线程和短聚合定时器。
- 将 Slave 回包中的 CAN 帧分发给子设备。

### `spi_can_node.c`

子设备。每个实例对应一个逻辑 CAN 通道。

职责：

- 实现 Zephyr `can_driver_api`。
- 维护本地 RX filter。
- 维护 CAN 状态缓存和状态变化回调。
- 将 `can_send()`、`can_set_timing()` 等请求转给父设备。

### `spi_can_mfd.h`

父子设备内部接口。该头文件不是应用层 API。

## 数据结构

### 父设备配置

`struct spi_can_mfd_config` 来自 devicetree：

- `bus`：SPI 目标设备。
- `int_gpio`：Slave 到 Host 的提示线。
- `cs_gpio`：手动片选。
- `can_core_clock`：远端 CAN 控制器核心时钟。
- `channels[]`：两个子设备指针。

### 父设备运行状态

`struct spi_can_mfd_data` 保存：

- `tx_frame` / `rx_frame`：固定长度 SPI 同步缓冲区。
- `bus_lock`：SPI 事务锁。
- `service_sem`：服务线程唤醒信号。
- `service_timer`：本地发送短聚合窗口。
- `poll_thread`：同步服务线程。
- `channel_data[]`：每个通道的 TX 队列、bitrate 和状态缓存。
- `rr_channel`：同步帧打包时的通道轮转起点。

### 子设备运行状态

`struct spi_can_node_data` 保存：

- `common`：Zephyr CAN 公共状态。
- `filters[]`：本地软件 RX filter。
- `cached_state` / `cached_err_cnt`：最近一次已知状态。
- `bitrate`：当前目标 bitrate。

## 同步帧

Host 和 Slave 每次 SPI 事务交换 `192` 字节。

### Host 到 Slave

字段：

- `magic`
- `version`
- `seq`
- `tx_count`
- `bitrate_mask`
- `bitrates[2]`
- `tx_entries[4]`

语义：

- `bitrate_mask` 指示哪些通道携带有效 bitrate。
- `tx_entries` 携带待发送到 Slave 物理 CAN 的帧。
- 每个 CAN entry 包含 `id`、`dlc`、`flags`、`channel` 和 `data[8]`。

### Slave 到 Host

字段：

- `magic`
- `version`
- `seq`
- `rx_count`
- `more_rx_mask`
- `state_mask`
- `states[2]`
- `rx_entries[4]`

语义：

- `state_mask` 指示哪些通道携带状态更新。
- `more_rx_mask` 指示 Slave 侧仍有待上传 RX 帧。
- `rx_entries` 携带 Slave 物理 CAN 收到的帧。

Host 和 Slave 两侧均使用 `BUILD_ASSERT` 校验结构体大小。

## Host 发送路径

1. 应用调用 `can_send(can_v0/can_v1, ...)`。
2. `spi_can_node_send()` 检查 DLC、flags 和启动状态。
3. 未启动的子设备先调用 `spi_can_node_start()`，同步目标 bitrate。
4. `spi_can_mfd_send()` 将 CAN 帧写入对应通道 `k_msgq`。
5. 父设备启动短聚合窗口并唤醒服务线程。
6. 服务线程构造 Host 同步帧。
7. SPI 事务成功后，父设备提交本次已发送队列项。

发送队列采用先 peek、成功后 get 的方式。SPI 事务失败时，已计划发送的帧仍保留在队列中。

## Host 接收路径

1. `spi_can_parse_slave_frame_locked()` 校验 Slave 回包。
2. 回包错位时尝试按 magic 重新对齐。
3. 根据 `state_mask` 更新通道状态缓存。
4. 根据 `rx_count` 解包 CAN entry。
5. 根据 entry 内的 `channel` 选择子设备。
6. `spi_can_node_handle_rx_frame()` 执行本地 RX filter。
7. 匹配的 callback 在释放子设备锁后调用。

RX filter 不同步到 Slave。Slave 上传原始帧，Host 本地完成过滤。

## Slave 参考实现

Slave 样例位于 `samples/vcan/vcan_slave_demo/src/main.c`。它是协议参考实现，不是 Zephyr
CAN 驱动。

每个 Slave 通道维护：

- 物理 CAN 设备指针。
- RX 队列。
- 当前 bitrate、timing 和 started 状态。
- 最近 CAN state 与 error counter。
- `state_dirty` 标记。

Slave 处理流程：

1. 物理 CAN RX callback 将帧放入通道 RX 队列。
2. RX 队列非空或状态变化时拉高 `INT`。
3. 主循环处理 Host 同步帧。
4. `bitrate_mask` 触发本地 `can_calc_timing()`、`can_set_timing()` 和 `can_start()`。
5. Host 下发的 CAN 帧通过 `can_send()` 发到物理 CAN。
6. 回包最多上传 4 个 RX 帧，并写入状态快照。

`INT` 是电平语义。只要有待上传 RX 或状态变化，Slave 应保持 `INT=1`。

## 调度

Host 同步线程同时支持轮询和事件唤醒。

固定参数：

- 兜底轮询周期：`500 us`
- 本地发送聚合窗口：`50 us`
- 同步帧容量：`4` 个 CAN 帧
- 通道数：`2`

触发源：

- `service_sem`：本地发送或 bitrate 变化。
- `service_timer`：聚合窗口结束。
- 轮询周期：检查 `INT` 与本地 pending 状态。
- 强制同步：`spi_can_mfd_get_state()` 查询状态。

服务线程每轮最多处理 `CONFIG_VCAN_MAX_SERVICE_BATCH` 次连续事务，避免长时间占用 CPU。

## 并发边界

- SPI 事务和父设备共享缓冲区由 `bus_lock` 保护。
- TX/RX 帧使用 `k_msgq` 作为有界缓冲。
- 定时器只负责唤醒服务线程，不执行 SPI 事务。
- 用户 RX callback 和状态 callback 不在子设备锁内调用。
- 入队路径使用非阻塞语义；队列满时返回错误或丢帧。

## 配置项

`drivers/vcan/Kconfig` 提供：

- `CONFIG_VCAN_MAX_RX_FILTER`
- `CONFIG_VCAN_TX_QUEUE_DEPTH`
- `CONFIG_VCAN_MAX_SERVICE_BATCH`
- `CONFIG_VCAN_WORKQ_STACK_SIZE`
- `CONFIG_VCAN_WORKQ_PRIORITY`
- `CONFIG_VCAN_INIT_PRIORITY`
- `CONFIG_VCAN_NODE_INIT_PRIORITY`

修改队列深度、同步批量或线程优先级时，应同时验证 Host demo 的四条路径。

## 维护注意事项

- Host 和 Slave 当前各自定义同步帧结构。修改协议字段时必须双边同步。
- 修改同步帧大小时必须更新 `SPI_CAN_SYNC_FRAME_SIZE` 和 `BUILD_ASSERT`。
- 修改 `SPI_CAN_SYNC_MAX_FRAMES` 时需要重新计算保留字段和 demo 压测上限。
- 新增 CAN mode、recover 或完整 timing 同步前，需要先定义协议兼容策略。
- Slave 样例并发模型较轻，不应按生产驱动假设其锁语义。

## 故障排查

- Host 没有收到远端帧：
  - 检查 `INT` 是否拉高。
  - 检查 SPI MISO/MOSI 是否交叉正确。
  - 检查 Slave 物理 CAN 是否已启动。
- Host 发送返回队列满：
  - 检查 `CONFIG_VCAN_TX_QUEUE_DEPTH`。
  - 检查 SPI 同步是否持续失败。
- 状态查询长期不更新：
  - 检查 Slave 是否设置 `state_mask`。
  - 检查 `int-gpios` 和强制同步路径。
- 两个通道吞吐差异明显：
  - 检查 `rr_channel` 轮转逻辑。
  - 检查某通道是否持续填满单次 `4` 帧额度。
