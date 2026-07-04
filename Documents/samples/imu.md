# IMU 样例

IMU 类样例分为姿态输出与加速度计标定，两类场景共享传感器驱动约束，差异在于是否使用持久化存储。

## 共同边界

- 约定使用 `DT_NODELABEL(bmi08x_accel)` 与 `DT_NODELABEL(bmi08x_gyro)`。
- `samples/IMU/IMU` 使用 `CONFIG_EKF_LIB` + `CONFIG_ARES`，并开启 CMSIS-DSP 相关组件。
- `samples/IMU/IMU_Calibrate` 在标定路径中同时开启 `CONFIG_FLASH` 与 `CONFIG_NVS`。

## samples/IMU/IMU

- 用途：读取 BMI08x 姿态数据并输出四元数/欧拉角，支持 QEKF 姿态链路。
- 构建：
  - `west build -b robomaster_board_c samples/IMU/IMU`
  - `west flash`
- 适用 board：
  - `robomaster_board_c`
  - `dm_mc02`
- 硬件依赖：
  - `bmi08x_accel` 与 `bmi08x_gyro` 节点
  - `CONFIG_SENSOR=y` 与异步触发配置
  - 串口用于姿态日志输出
- 配置与维护：
  - `sample.yaml` 过滤条件要求：
    - `depends_on: spi sensor`
    - `filter: dt_nodelabel_enabled("bmi08x_accel") and dt_nodelabel_enabled("bmi08x_gyro")`
  - `samples/IMU/IMU/boards/robomaster_board_c.overlay` 提供 `robomaster_board_c` 的传感器兼容映射。
  - `dm_mc02` 依赖板级 DTS 提供 `bmi08x_*` 节点，需满足 `sample.yaml` 的 `filter` 条件。
  - `sample.yaml` 的 `integration_platforms` 与 `filter` 是该样例的 CI 入口约束。

## samples/IMU/IMU_Calibrate

- 用途：加速度计六参数标定（偏置 + 尺度）与参数持久化。
- 构建：
  - `west build -b robomaster_board_c samples/IMU/IMU_Calibrate`
  - `west flash`
- 适用 board：`robomaster_board_c`
- 硬件依赖：
  - `bmi08x_accel`、`bmi08x_gyro`
  - 可按需求使用按钮事件（`DT_CHOSEN(zephyr_button)`）
  - Flash/NVS 分区（`CONFIG_FLASH_MAP`）
- 维护规则：
  - 当前样例固定提供 `storage_partition`，`reg = <0x00C0000 0x00040000>`。
  - 标定参数变更需同步 `CONFIG_NVS_DATA_CRC` 与存储生命周期，避免覆盖非标定分区。
  - 标定流程依赖 `sample.yaml` 中 `sensor` 标签与 CI regex；改动输出时同步更新验收规则。

## 维护建议（IMU 共用）

- 新增 IMU 类型时，先确认 devicetree compatible 与节点别名能满足 `filter` 条件。
- 标定样例涉及可持久化参数，发布前应验证 `CONFIG_FLASH` 与存储分区策略。
- 输出语义以 `main.c` 打印项为准，避免以示例脚本替代传感器原始语义说明。
