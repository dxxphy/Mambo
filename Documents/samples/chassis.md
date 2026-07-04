# Chassis 样例

本样例将 SBUS 遥控输入映射为底盘速度与转向指令，并接入 IMU 反馈通道。

## 用途

- 验证 Chassis 驱动链路的初始化与运行闭环。
- 验证 `sbus` 通道值映射为底盘速度 / 角速度指令。

## 构建与烧录

- `west build -b robomaster_board_c samples/chassis`
- `west flash`

## 适用 board

- 默认支持 `robomaster_board_c`。
- `samples/chassis/sample.yaml` 当前 `integration_platforms` 为 `native_sim`，用于 Twister 入口验证；硬件上仍以真实 board 构建验证。

## 硬件依赖

- `sbus0` 节点（兼容 `ares,sbus`）
- `can1` / `can2`、底盘电机节点
- IMU 节点用于姿态补偿更新
- 轮组与转向电机节点：`rm_motor` 下的 `motor_wheel*`、`motor_steer*`
- PID 参数节点（`wheel_*_pid`、`steer_*_pid`、`chassis_roll_angle_pid`）

## overlay 与维护边界

- `samples/chassis/boards/robomaster_board_c.overlay` 提供完整底盘拓扑：
  - `chassis` 实例及 `wheels` 定义
  - `steerwheel*`、`motor_wheel*`、`motor_steer*`、各 PID 节点
  - `sbus0` 兼容定义
- `prj.conf` 决定 ARES/SBUS/串口、日志与 DSP 能力。
- `samples/chassis/src/main.c` 中执行 `chassis_set_speed()` 与 `chassis_set_gyro()` 的调用顺序是行为核心，不建议改变。

## 维护规则

- 若变更底盘类型（Mecanum/Omni/Steer）或轮组序号，需同步 overlay 中 `wheels` 与 `steerwheel` 的映射关系。
- 新增 IMU 或 SBUS 实现时，先调整 DTS compatible 与 `can_channel` 分配，再改应用初始化流程。
