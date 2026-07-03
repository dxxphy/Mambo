# PID

## 概览

本仓库中的 PID 模块是轻量级控制工具层，而非完整的独立 Zephyr 驱动族。其公开入口为 `include/zephyr/drivers/pid.h`，`drivers/pid/` 主要提供构建归属与 devicetree 接入点。

相关文件：

- `include/zephyr/drivers/pid.h`
- `drivers/pid/`
- `dts/bindings/pid/pid,single.yaml`
- `dts/bindings/pid/pid.mit.yaml`

当前仓内使用方包括：

- `drivers/chassis/`
- `lib/ares/` 下的 IMU 温控代码
- 原生仿真测试

## 设备模型

PID 实例由 `struct pid_data` 表示，包含：

- 指向参考值与反馈值的指针
- 可选的微分参考/反馈指针
- 积分与微分累积器
- 时间戳指针
- 输出指针
- 所属 Zephyr 设备指针

静态配置为 `struct pid_config`：

- `k_p`
- `k_i`
- `k_d`
- `integral_limit`
- `output_limit`
- `output_offset`
- `detri_lpf`
- `mit`

公共宏 `PID_NEW_INSTANCE()` 与 `PID_DEVICE_DT_DEFINE()` 支持从 devicetree 节点静态实例化。

当前仓内，底盘通过 `PID_NEW_INSTANCE()` 实例化一个每底盘 PID 数据对象，并绑定到 `angle-pid` 引用的 devicetree 节点。

## 公共接口

全部有效公共入口位于 `include/zephyr/drivers/pid.h`。

### 计算函数

- `pid_calc(data)`
- `pid_calc_in(data, error, deltaT_us)`
- `pid_calc_mit(data, pos_error, vel_error, deltaT_us)`

`pid_calc()` 基于已在 `struct pid_data` 中注册的指针计算结果。

`pid_calc_in()` 接收已计算好的标量误差以及微秒级时间步。

`pid_calc_mit()` 使用位置误差参与比例与积分项，使用独立的速度误差参与微分项。

### 注册与参数辅助函数

- `pid_reg_input(data, curr, ref)`
- `pid_reg_output(data, output)`
- `pid_reg_time(data, curr_cyc, prev_cyc)`
- `mit_reg_detri_input(data, detri_curr, detri_ref)`
- `pid_get_params(data, config)`
- `pid_set_params(dev, config)`

这些函数在头文件内为 `static` 辅助实现，而非系统调用。然而逻辑集中在头文件内，因此它们仍属于实际集成接口的一部分。

## Devicetree 绑定

绑定文件：

- `dts/bindings/pid/pid,single.yaml`
- `dts/bindings/pid/pid.mit.yaml`

兼容字符串：

- `pid,single`
- `pid,mit`

通用属性：

- `k_p`（必需）
- `k_i`
- `k_d`
- `i_max`
- `out_max`
- `offset`
- `detri_lpf`

这些绑定描述了填充 `struct pid_config` 所需的数据。

当前公共头文件字段名为：

- `integral_limit`
- `output_limit`
- `output_offset`

而绑定属性名为：

- `i_max`
- `out_max`
- `offset`

该映射由设备侧实例化代码完成，应用侧无需手工对齐。

## 构建与配置入口

Kconfig：

- `drivers/Kconfig`
- `drivers/pid/Kconfig`

相关符号：

- `CONFIG_PID_MUNU`
- `CONFIG_PID`

构建入口：

- `drivers/CMakeLists.txt`
- `drivers/pid/CMakeLists.txt`

`drivers/pid/CMakeLists.txt` 会添加：

- 当 `CONFIG_PID=y` 时添加 `pid_mit.c`
- 当 `CONFIG_PID=y` 时添加 `pid_single.c`

头文件承载实际运算逻辑，源码文件主要用于驱动树集成与实例归属。

## 运行时行为

### 时间基准

`pid_calc()` 从 `k_cycle_get_32()` 时间戳推导 `deltaT`，并用 `k_cyc_to_us_near32()` 转为微秒。

`pid_calc_in()` 与 `pid_calc_mit()` 直接接收 `deltaT_us`。

三类路径都在时间步为 0 时立即返回。

### 积分项

当 `k_i` 非 `NaN` 且非零时：

- 按 `error * deltaT / k_i` 更新积分累积
- 当 `integral_limit` 非零时进行限幅

### 微分项

当 `k_d` 非 `NaN` 时：

- 微分输入来自 `detri_ref - detri_curr`，或在 MIT 模式来自速度误差
- 当 `detri_lpf` 为 `NaN` 时，直接使用微分值
- 否则对先前微分值进行低通滤波

### 输出

输出表达式为：

- `k_p * (error + integral + derivative) + output_offset`

当 `output_limit` 非零时，最终输出按对称区间限幅。

当 `data->output` 非空时，计算结果也会写回该存储位置。

## 错误处理与边界条件

- 所有辅助函数都允许 `struct pid_data *` 为 `NULL`
- 当未绑定设备时，`pid_calc*()` 返回 `0`
- 当 `data->curr` 为 `NULL` 时，`pid_calc()` 在不更新输出的前提下直接返回
- 零时间步会抑制状态推进并返回 `0`
- `pid_get_params()` 与 `pid_set_params()` 在设备或配置存储不可用时返回 `-ENOSYS`

代码使用 `NaN` 作为 `k_i`、`k_d`、`detri_lpf` 可选功能开关的一部分，这是可观测行为，后续重构应保留。

## 示例与测试

原生测试：

- `tests/native_sim/pid/src/main.c`

覆盖的行为包括：

- 输出限幅
- 积分限幅
- 零时间步处理
- MIT 模式微分行为

其他使用示例：

- `samples/chassis/boards/robomaster_board_c.overlay`
- `samples/IMU/IMU_Calibrate/src/main.c`
- `lib/ares/ekf/imu_task.c`
- `lib/ares/mahony/imu_task.c`

这些示例展示了 devicetree 绑定的 PID 节点以及上层控制代码对 `pid_calc_in()` 的直接调用。
