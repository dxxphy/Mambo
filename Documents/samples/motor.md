# 电机样例

电机样例覆盖 DJI、DM、LA、RS、MI、VESC 执行链路。
文档仅描述可维护边界，不复述实现细节。

## 共同边界

- 所有样例默认启用 `CONFIG_MOTOR=y`，按驱动类型配置 `CONFIG_MOTOR_DJI/DM/LA_YINSHI/RS/MI/VESC`。
- 发送与反馈基于 CAN 或串口接口，需与目标板 DTS 节点一致。
- 代码使用 DTS 节点进行设备查找（`DT_NODELABEL`/`DT_INST`）。

## samples/motor/dji_m2006_verify

- 用途：M2006 三模式（角度/转矩/速度）行为验证，包含上电使能与反馈打印。
- 构建：
  - `west build -b dm_mc02 samples/motor/dji_m2006_verify`
  - `west flash`
- 适用 board：`dm_mc02`
- 硬件依赖：
  - `can1`（FDCAN）接口与收发器
  - `m2006_motor` 设备节点
  - `speed_pid_m2006` 与 `angle_pid_m2006`
- 维护规则：
  - `m2006_motor` 的 `id`、`tx_id`、`rx_id` 与 overlay 一致。
  - 默认行为为 `tx=0x200`、`rx=0x201`，需改动应同步 overlay 与日志预期。

## samples/motor/dji_m3508_demo

- 用途：Robomaster Board C 上 M3508 电机使能、速度测试、归零、角度测试。
- 构建：
  - `west build -b robomaster_board_c samples/motor/dji_m3508_demo`
  - `west flash`
- 适用 board：`robomaster_board_c`
- 硬件依赖：
  - `m3508_motor` 设备节点（`compatible = "dji,motor"`、`is_m3508`）
  - `can1`
  - 速度/角度 PID 节点
- 维护规则：
  - 样例依赖 `app.overlay` 定义节点，不在主逻辑中新增节点定义。
  - 保持 `DT_NODELABEL(m3508_motor)` 与 overlay 定义同步。

## samples/motor/dm_demo

- 用途：DM 电机基础速度控制验证。
- 构建：
  - `west build -b dm_mc02 samples/motor/dm_demo`
  - `west flash`
- 适用 board：`dm_mc02`
- 硬件依赖：
  - `dm_motor` 节点（`DT_PATH(motor, dm_motor)`）
  - `can1` 接口
- 维护规则：
  - 主要参数来自 `samples/motor/dm_demo/boards/dm_mc02.overlay`，不应将控制参数下沉到日志逻辑中。
  - 若替换电机参数，优先修改 overlay 与 `prj.conf` 中的 CAN 相关开关。

## samples/motor/la_yinshi_demo

- 用途：Yinshi LA 执行器开合、状态查询、故障清理链路验证。
- 构建：
  - `west build -b robomaster_board_c samples/motor/la_yinshi_demo`
  - `west flash`
- 适用 board：`robomaster_board_c`
- 硬件依赖：
  - `usart1`
  - `la_demo` 节点（`compatible = "yinshi,la"`）
- 维护规则：
  - 行程参数（`POS_OPEN`、`POS_GRASP`）改动需同步样例维护说明。
  - 若启用 RS485，采用 overlay 的 `de-gpios` 配置，不在逻辑中临时切换。

## samples/motor/rs_demo

- 用途：RS02 轮机电机 MIT 闭环示例。
- 构建：
  - `west build -b robomaster_board_c samples/motor/rs_demo`
  - `west flash`
- 适用 board：`robomaster_board_c`（依赖 `rs_motor` DTS）
- 硬件依赖：
  - `rs_motor` 兼容节点
  - CAN 总线
  - MIT 控制器参数
- 维护规则：
  - 代码使用 `DT_INST(0, rs_motor)`；新增/替换 RS 设备时保持 compatible 与兼容序列。
  - 运行日志与状态节拍用于判定回归时，尽量不改动主控制流程。

## samples/motor/test_mi

- 用途：MI 电机 MIT 控制最小化回归样例。
- 构建：
  - `west build -b robomaster_board_c samples/motor/test_mi`
  - `west flash`
- 适用 board：`robomaster_board_c`
- 硬件依赖：
  - `boards/robomaster_board_c.overlay` 中的 `mi_motor` 与 `controllers`
  - `can1` 和相关收发能力
- 维护规则：
  - 与代码一致的设备实例是 `DT_INST(0, mi_motor)`。
  - 线程优先级、反馈周期及状态日志为维护观察面，修改前需同步结果。

## samples/motor/vesc_demo

- 用途：VESC 电机速度控制验证与回环状态观察。
- 构建：
  - `west build -b robomaster_board_c samples/motor/vesc_demo`
  - `west flash`
- 适用 board：`robomaster_board_c`
- 硬件依赖：
  - `can1`
  - `vesc,motor` 节点（含 `id`、`kv`、`pole_pairs`、`p_max` 等参数）
- 维护规则：
  - 核心参数来源 overlay，避免在主逻辑中通过临时常量替换。

## 维护建议（motor 共用）

- 每次更改节点参数请先更新 overlay，再同步样例文档。
- 节点命名应稳定（`m2006_motor`、`motor0`、`motor_...` 等），避免出现同名混淆。
- 新增板级支持时优先补齐 `boards/<sample>/<board>.overlay` 与 `prj.conf` 约束。
