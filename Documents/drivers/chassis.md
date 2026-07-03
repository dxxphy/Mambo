# 底盘

## 概览

底盘子系统是在 `wheel` API 之上实现的整车体运动层。它接收平面速度、偏航角速度目标或目标偏航角，再将这些请求解析为每个轮子的转速与转向角命令。

实现位置：

- `include/zephyr/drivers/chassis.h`
- `drivers/chassis/chassis.c`
- `drivers/chassis/chassis.h`
- `dts/bindings/chassis/ares,chassis.yaml`

当前底盘驱动本身不直接处理 CAN、UART 或任何电机协议。它将所有执行输出委托给 `wheel` 设备。

## 设备模型

公共类型为 `struct chassis_driver_api`，对外暴露为一个 Zephyr 设备。

每个实例的运行时状态保存在 `chassis_data_t` 中：

- 当前与上一次时间戳
- 目标状态请求
- 斜坡限制后的目标状态
- 上报的底盘状态
- 从 devicetree 派生的轮子几何参数
- 传感器快照（`struct pos_data`）
- 使能、角度控制、跟踪角度和静态角度标志

静态配置保存在 `chassis_cfg_t` 中：

- `angle_pid`
- `wheels[]`
- 轮子位置偏移
- `max_lin_accel`
- `max_gyro`

`CHASSIS_WHEEL_COUNT` 来自实例 0 的 `wheels` 属性长度。当前实现实际仅假设存在单个底盘实例。

## 公共接口

导出的 API 声明在 `include/zephyr/drivers/chassis.h`。

### 命令接口

- `chassis_set_speed(dev, speedX, speedY)`
- `chassis_set_angle(dev, angle)`
- `chassis_set_gyro(dev, gyro)`
- `chassis_set_static(dev, static_angle)`

`chassis_set_speed()` 使用应用侧惯例的机体坐标系：

- `+X` 向前
- `+Y` 向左

`drivers/chassis/chassis.c` 中的求解器会在进入轮速解析前，将该坐标系内部旋转到自身定义的前进/右向坐标系。

`chassis_set_angle()` 启用基于配置 PID 的闭环偏航控制。  
`chassis_set_gyro()` 关闭角度控制并直接下发角速度。

`chassis_set_static()` 请求静态轮对齐。其会清除平移目标，并在内部设定速度未降到硬编码阈值前返回 `-EBUSY`。

### 状态接口

- `chassis_get_status(dev)`

返回的是设备持有的 `chassis_status_t` 指针。调用方应将其视为瞬时快照，而非持久存储。

### 公共头文件中的辅助例程

该头文件还对仓内应用暴露了两个静态助手函数：

- `chassis_update_sensor(dev, pos_data)`
- `chassis_set_enabled(dev, enabled)`

它们不是系统调用接口，但因现有应用直接调用，属于实际集成面的一部分。

`chassis_update_sensor()` 会将偏航、加速度和陀螺数据写入设备状态，并通过加 360 度的方式规范化负偏航值。

`chassis_set_enabled()` 用于切换输出生成。关闭底盘时，会对每个已配置轮子调用一次 `wheel_disable()`。

## Devicetree 绑定

绑定文件：`dts/bindings/chassis/ares,chassis.yaml`

兼容字符串：

- `ares,chassis`

属性：

- `wheels`（必需，`phandle-array`）
- `angle-pid`（必需，`phandle`）
- `speed-limit`（可选，`string`）
- `track_angle`（可选，`boolean`）
- `max_gyro`（可选，`string`）
- `max_linear_accel`（可选，`string`）

`wheels` 属性使用轮子绑定定义的 `wheel-cells`：

- `offset_x`
- `offset_y`

实现将这些偏移量按 `10000.0f` 除法转换为“米级”浮点单位。

当前代码消费的字段：

- `wheels`
- `angle-pid`
- `track_angle`
- `max_gyro`
- `max_linear_accel`

`speed-limit` 在绑定中定义，但当前驱动未使用。

## 构建与配置入口

Kconfig：

- `drivers/Kconfig`
- `drivers/chassis/Kconfig`

相关符号：

- `CONFIG_CHASSIS`
- `CONFIG_CHASSIS_INIT_PRIORITY`
- `CONFIG_CHASSIS_LOG_LEVEL`

`CONFIG_CHASSIS` 会选择 `CONFIG_WHEEL`。

构建入口：

- `drivers/CMakeLists.txt`
- `drivers/chassis/CMakeLists.txt`

当 `CONFIG_CHASSIS=y` 时，`drivers/chassis/` 会参与编译。

## 运行时行为

### 初始化

`cchassis_init()`：

- 计算每个轮子相对底盘中心的角度
- 计算每个轮子到底盘中心的距离
- 初始化时间戳
- 启动初始延时 100 ms、周期 8 ms 的全局定时器

定时器仅在底盘使能时才向信号量发布。

### 工作线程

求解器运行在静态定义的 `chassis_thread` 中。

当前实现要点如下：

- 线程按 `DT_DRV_INST(0)` 定义
- 在 `enabled` 变为 `true` 前持续旋转等待
- 禁用时重复发送 `wheel_set_static(..., 90.0f)`
- 激活后等待 `chassis_sem`

每个周期执行：

1. 更新时间戳
2. 若角度控制开启且传感器偏航有效，计算偏航误差
3. 按 `max_lin_accel` 进行平移设定值斜坡限制
4. 直接计算或通过 `pid_calc_in()` 计算偏航速率设定值
5. 将偏航速率限制到 `max_gyro`
6. 使用 `cchassis_resolve()` 解析轮子命令

### 轮子解析

对每个轮子求解时，流程为：

- 根据轮子偏移与底盘陀螺输入计算旋转速度分量
- 当开启 `track_angle` 时，可选地将平移指令按当前偏航旋转
- 计算速度幅值与转向角
- 调用 `wheel_set_speed()`

在开启 `static_angle` 且平移速度接近 0 时，先尝试 `wheel_set_static()`。若返回错误，则回退到 `wheel_set_speed(..., 0, angle_to_center)`。

## 错误处理与边界条件

- 若驱动未实现，`chassis_get_status()` 返回 `NULL`。
- 若驱动 API 缺失，`chassis_set_static()` 返回 `-ENOSYS`。
- `cchassis_set_static()` 若底盘未足够减速返回 `-EBUSY`。
- 当传感器偏航为 `NaN` 时跳过偏航控制。
- 偏航误差低于 `0.8` 度时按 0 处理。
- 平移动作包含加速度限制；当 `track_angle` 关闭时会叠加向心项。
- 实现采用单一全局定时器与信号量，这是对多实例扩展的现实约束。

## 集成点

- 轮子设备必须实现 `wheel_set_speed()`
- 当轮子驱动实现 `wheel_set_static()` 时，静态定位效果更好
- 偏航控制要求有效 PID 实例，并通过 `chassis_update_sensor()` 提供稳定的传感器更新

## 示例与验证

主要示例：

- `samples/chassis`

关键文件：

- `samples/chassis/src/main.c`
- `samples/chassis/boards/robomaster_board_c.overlay`

示例展示内容：

- 使用 `chassis_set_speed()` 映射 SBUS 输入
- 使用 `chassis_set_gyro()` 映射 SBUS 偏航命令
- 使用 `chassis_set_enabled()` 显式使能/去使能
- 使用 `chassis_set_angle()` 配置偏航目标

当前仓内未提供底盘行为的独立原生测试。验证主要来自示例和硬件运行。
