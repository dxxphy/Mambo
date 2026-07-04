# 设备树参考

本文档汇总 Mambo 提供的 devicetree binding。Zephyr 标准属性如 `status`、`reg`、`label`
与总线 binding 的通用属性不重复展开。
## 电机设备

### `dji,motor`

文件：`dts/bindings/motor/dji,motor.yaml`

DJI CAN 电机节点。

| 属性 | 类型 | 必需 | 说明 |
| --- | --- | --- | --- |
| `id` | int | 是 | 电机编号。 |
| `tx_id` | int | 是 | 主控发送控制帧 ID。 |
| `rx_id` | int | 是 | 电机反馈帧 ID。 |
| `can_channel` | phandle | 是 | 所属 CAN 设备。 |
| `gear_ratio` | string | 是 | 减速比。 |
| `controllers` | phandles | 否 | `controller` 节点列表。 |
| `is_m3508` | boolean | 否 | 标记 M3508。 |
| `is_m2006` | boolean | 否 | 标记 M2006。 |
| `is_gm6020` | boolean | 否 | 标记 GM6020。 |
| `is_dm_motor` | boolean | 否 | 使用 DJI 兼容路径驱动的 DM 类电机。 |
| `dm_i_max` | string | 否 | DM 兼容路径电流上限。 |
| `dm_torque_ratio` | string | 否 | DM 兼容路径扭矩系数。 |
| `minor_arc` | boolean | 否 | 位置控制使用小弧路径。 |
| `inverse` | boolean | 否 | 反向角度或输出方向。 |
| `follow` | phandle | 否 | 跟随另一个电机设备。 |

### `dm,motor`

文件：`dts/bindings/motor/dm,motor.yaml`

达妙电机节点。

| 属性 | 类型 | 必需 | 说明 |
| --- | --- | --- | --- |
| `id` | int | 否 | 电机编号。 |
| `tx_id` | int | 是 | 主控发送帧 ID。 |
| `rx_id` | int | 是 | 电机回复帧 ID。 |
| `can_channel` | phandle | 是 | 所属 CAN 设备。 |
| `controllers` | phandles | 否 | `controller` 节点列表。 |
| `v_max` | string | 否 | 最大速度。 |
| `p_max` | string | 否 | 最大位置。 |
| `t_max` | string | 否 | 最大扭矩。 |
| `read_only` | boolean | 否 | 只接收反馈，不自动使能或发送控制帧。 |

控制请求频率由 Kconfig `CONFIG_MOTOR_DM_DEFAULT_FREQ_HZ` 设置。

### `mi,motor`

文件：`dts/bindings/motor/mi,motor.yaml`

小米电机节点。

| 属性 | 类型 | 必需 | 说明 |
| --- | --- | --- | --- |
| `id` | int | 否 | 电机编号。 |
| `tx_id` | int | 是 | 主控发送帧 ID。 |
| `rx_id` | int | 是 | 电机回复帧 ID。 |
| `can_channel` | phandle | 是 | 所属 CAN 设备。 |
| `gear_ratio` | string | 是 | 减速比。 |
| `controllers` | phandles | 否 | `controller` 节点列表。 |

### `rs,motor`

文件：`dts/bindings/motor/rs,motor.yaml`

Robstride 电机节点。

| 属性 | 类型 | 必需 | 说明 |
| --- | --- | --- | --- |
| `id` | int | 是 | 电机编号。 |
| `rx_id` | int | 是 | 电机回复帧 ID。 |
| `tx_id` | int | 是 | 主控发送帧 ID。 |
| `motor_type` | string | 是 | 电机型号。 |
| `can_channel` | phandle | 是 | 所属 CAN 设备。 |
| `controllers` | phandles | 否 | `controller` 节点列表。 |
| `auto_report` | boolean | 否 | 电机主动上报。 |
| `v_max` | string | 否 | 最大速度。 |
| `p_max` | string | 否 | 最大位置。 |
| `t_max` | string | 否 | 最大扭矩。 |

### `lk,motor`

文件：`dts/bindings/motor/lk,motor.yaml`

凌控电机节点。

| 属性 | 类型 | 必需 | 说明 |
| --- | --- | --- | --- |
| `id` | int | 否 | 电机编号。 |
| `tx_id` | int | 是 | 主控发送帧 ID。 |
| `rx_id` | int | 是 | 电机回复帧 ID。 |
| `can_channel` | phandle | 是 | 所属 CAN 设备。 |
| `controllers` | phandles | 否 | `controller` 节点列表。 |

### `vesc,motor`

文件：`dts/bindings/motor/vesc,motor.yaml`

VESC CAN 电机节点。

| 属性 | 类型 | 必需 | 说明 |
| --- | --- | --- | --- |
| `id` | int | 是 | VESC motor ID。 |
| `rx_id` | int | 否 | 本机 host ID，用于发送 PING 与接收 PONG。 |
| `can_channel` | phandle | 是 | 所属 CAN 设备。 |
| `gear_ratio` | string | 是 | 减速比。 |
| `controllers` | phandles | 否 | `controller` 节点列表。 |
| `kv` | string | 是 | 电机 KV。 |
| `pole_pairs` | string | 是 | 极对数。 |
| `v_max` | string | 是 | 最大速度或速度换算上限。 |
| `p_max` | string | 是 | 最大位置。 |
| `t_max` | string | 是 | 最大扭矩。 |
| `i_max` | string | 是 | 最大电流。 |
| `freq` | int | 否 | PING 频率，用于在线检测。 |

控制帧频率由 Kconfig `CONFIG_MOTOR_VESC_CONTROL_FREQ_HZ` 设置。

### `yinshi,la`

文件：`dts/bindings/motor/yinshi,la.yaml`

银石线性执行器节点。

| 属性 | 类型 | 必需 | 说明 |
| --- | --- | --- | --- |
| `uart` | phandle | 是 | 通信 UART。 |
| `id` | int | 否 | 执行器 ID，默认 `1`。 |
| `de-gpios` | phandle-array | 否 | RS485 DE 引脚。 |
| `max-retries` | int | 否 | 位置命令最大重试次数。 |
| `reply-timeout-ms` | int | 否 | 回复超时时间。 |

## 电机控制器

### `motor-controller,mit`

MIT controller 参数节点。

| 属性 | 类型 | 说明 |
| --- | --- | --- |
| `k_p` | string | 位置比例参数。 |
| `k_i` | string | 积分参数。 |
| `k_d` | string | 速度或微分参数。 |
| `i_max` | string | 积分限幅。 |
| `out_max` | string | 输出限幅。 |
| `offset` | string | 输出偏置。 |
| `detri_lpf` | string | 微分低通系数。 |

### `motor-controller,pv`

位置-速度串级 controller 参数节点。

| 属性 | 类型 | 说明 |
| --- | --- | --- |
| `pos_k_p`、`pos_k_i`、`pos_k_d` | string | 位置环参数。 |
| `pos_i_max`、`pos_out_max` | string | 位置环积分和输出限幅。 |
| `pos_offset`、`pos_detri_lpf` | string | 位置环偏置和微分低通。 |
| `vel_k_p`、`vel_k_i`、`vel_k_d` | string | 速度环参数。 |
| `vel_i_max`、`vel_out_max` | string | 速度环积分和输出限幅。 |
| `vel_offset`、`vel_detri_lpf` | string | 速度环偏置和微分低通。 |
| `i_max`、`out_max`、`offset`、`detri_lpf` | string | 分环参数缺失时的通用回退参数。 |

### `motor-controller,vo`

单目标 controller 参数节点。

| 属性 | 类型 | 说明 |
| --- | --- | --- |
| `target` | string | `"speed"` 或 `"torque"`，默认 `"speed"`。 |
| `k_p`、`k_i`、`k_d` | string | 控制参数。 |
| `i_max`、`out_max` | string | 积分和输出限幅。 |
| `offset` | string | 输出偏置。 |
| `detri_lpf` | string | 微分低通系数。 |

## 底盘与轮组

### `ares,chassis`

文件：`dts/bindings/chassis/ares,chassis.yaml`

| 属性 | 类型 | 必需 | 说明 |
| --- | --- | --- | --- |
| `wheels` | phandle-array | 是 | 轮组（wheel）节点列表，`specifier` 为 `offset_x`、`offset_y`。 |
| `angle-pid` | phandle | 是 | 底盘角度 PID。 |
| `speed-limit` | string | 否 | 速度限制。 |
| `track_angle` | boolean | 否 | 启用角度跟踪。 |
| `max_gyro` | string | 否 | 最大角速度。 |
| `max_linear_accel` | string | 否 | 最大线加速度。 |

### `ares,mecanum`

文件：`dts/bindings/steerwheel/ares,mecanum.yaml`

| 属性 | 类型 | 必需 | 说明 |
| --- | --- | --- | --- |
| `wheel-motor` | phandle | 是 | 驱动车轮的电机。 |
| `wheel-radius` | string | 是 | 轮半径。 |
| `angle-offset` | string | 是 | 安装角偏置。 |
| `inverse-wheel` | boolean | 否 | 反向轮输出。 |
| `free-angle` | string | 否 | 自由角。 |

### `ares,steerwheel`

文件：`dts/bindings/steerwheel/ares,steerwheel.yaml`

| 属性 | 类型 | 必需 | 说明 |
| --- | --- | --- | --- |
| `steer-motor` | phandle | 是 | 转向电机。 |
| `wheel-motor` | phandle | 是 | 驱动电机。 |
| `wheel-radius` | string | 是 | 轮半径。 |
| `angle-offset` | string | 是 | 安装角偏置。 |
| `inverse-steer` | boolean | 否 | 反向转向输出。 |
| `inverse-wheel` | boolean | 否 | 反向驱动输出。 |
| `multi_turn` | boolean | 否 | 使用多圈角度。 |

## 传输与通信

### `ares,sbus`

文件：`dts/bindings/transfer/ares,sbus.yaml`

| 属性 | 类型 | 默认 | 说明 |
| --- | --- | --- | --- |
| `frame_size` | int | `25` | SBUS 帧长度。 |
| `input_channels` | int | `16` | 输入通道数。 |
| `start_byte` | int | `0x0F` | 帧头。 |
| `end_byte` | int | `0x00` | 帧尾。 |

### `vnd,canbus`

文件：`dts/bindings/canbus/vnd,canbus.yaml`

| 属性 | 类型 | 必需 | 说明 |
| --- | --- | --- | --- |
| `can_device` | phandle | 是 | 物理 CAN 设备。 |

## VCAN

### `custom,spi-can-mfd`

文件：`dts/bindings/can/custom,spi-can-mfd.yaml`

SPI 虚拟 CAN 桥父节点。

| 属性 | 类型 | 必需 | 说明 |
| --- | --- | --- | --- |
| `can-core-clock` | int | 是 | 远端 CAN 控制器核心时钟。 |
| `int-gpios` | phandle-array | 是 | 远端提示主机有 RX 或状态变化的 GPIO。 |
| `host-int-gpios` | phandle-array | 否 | 保留兼容属性。 |

子节点属性：

| 属性 | 类型 | 必需 | 说明 |
| --- | --- | --- | --- |
| `can-channel` | int | 是 | 远端 CAN 通道，`0` 或 `1`。 |

### `custom,spi-can-node`

文件：`dts/bindings/can/custom,spi-can-node.yaml`

独立声明的托管 CAN 通道。

| 属性 | 类型 | 必需 | 说明 |
| --- | --- | --- | --- |
| `can-channel` | int | 是 | 远端 CAN 通道，`0` 或 `1`。 |

### `ares,test-can`

文件：`dts/bindings/can/ares,test-can.yaml`

Native sim motor 测试用 CAN 控制器。该绑定仅用于测试，应用不应依赖该 compatible。

## 传感器与 IMU

### `pixart,paw3395`

文件：`dts/bindings/sensor/pixart,paw3395.yaml`

| 属性 | 类型 | 必需 | 说明 |
| --- | --- | --- | --- |
| `cs-gpios` | phandle-array | 是 | SPI CS 引脚。 |
| `ripple` | boolean | 否 | 启用 `ripple` 模式。 |
| `dpi` | int | 否 | DPI，默认 `20000`。 |

### `gyro-bias`

文件：`dts/bindings/imu/gyro-bias.yaml`

| 属性 | 类型 | 必需 | 说明 |
| --- | --- | --- | --- |
| `gyro-bias` | string-array | 是 | 陀螺仪偏置。 |

## PID

PID binding 位于 `dts/bindings/pid/`，用于旧式 PID 设备。Motor 新 controller
使用 `motor-controller,*` binding。
