# VCAN 样例

VCAN 分为主端与从端演示，用于验证 SPI2 + 双 CAN 通道映射与队列吞吐。

## samples/vcan/vcan_host_demo

- 用途：主机侧与两路虚拟 CAN（`can_v0`/`can_v1`）进行双向映射与回传压测。
- 构建：
  - `west build -b robomaster_board_c samples/vcan/vcan_host_demo -d build/vcan_host`
  - `west flash -d build/vcan_host`
- 硬件依赖：
  - `spi2` + DMA（`MOSI/MISO/SCK/NSS`）
  - `can1`、`can2`
  - `PB8` 作为 `HOST_INT`（输入）
- 覆盖内容：
  - `app.overlay` 将 `virtual_can_host`、`can_v0`、`can_v1` 及采样率映射写入。
- 维护规则：
  - 映射固定：`can1 <-> can_v0`、`can2 <-> can_v1`。
  - 源码中 `TARGET_FPS_OVERRIDE=3000`，速率与队列行为请与该设置一致回归。
- 主端与从端的端口编号（0/1）与日志关键字不得随意改动。

## samples/vcan/vcan_slave_demo

- 用途：从机侧完成 SPI 从机转发队列行为和帧聚合，验证与主端通道语义一致。
- 构建：
  - `west build -b robomaster_board_c samples/vcan/vcan_slave_demo -d build/vcan_slave`
  - `west flash -d build/vcan_slave`
- 硬件依赖：
  - `spi2` + DMA
  - `can1`、`can2`
  - `PB8`（INT 输出）
- 维护规则：
  - 每次 SPI 同步帧固定为 `192` 字节，单次最大返回 4 帧（`SPI_CAN_SYNC_MAX_FRAMES = 4`）。
  - `PB8` 维持电平型 INT 行为，不建议替换为触发式实现。
- 若调整队列深度或目标速率，需同步主端 `TARGET_FPS_OVERRIDE` 与测试期望。

## 运行一致性（双端通用）

- 两端均以 `robomaster_board_c` 为常见运行板。
- 端口语义不对称替换会导致回测不可比，映射需保持统一：
  - `channel 0` 对应 `can1 / can_v0`
  - `channel 1` 对应 `can2 / can_v1`
- 若扩展到新板/新引脚，优先更新 `app.overlay` 并单独用独立 build 目录执行联调。
