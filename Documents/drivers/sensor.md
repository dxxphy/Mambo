# 传感器

## 概览

本仓库当前在 `drivers/sensor/` 下提供两类传感器驱动：

- `pixart,paw3395`
- `wit,hwt906`

两者均接入 Zephyr 的 sensor 子系统，并导出标准 `struct sensor_driver_api`，其包含 `sample_fetch()` 与 `channel_get()`。

相关目录：

- `drivers/sensor/`
- `dts/bindings/sensor/`
- `drivers/sensor/WIT_HWT906/`

## 构建与配置入口

顶层传感器入口：

- `drivers/sensor/Kconfig`
- `drivers/sensor/CMakeLists.txt`

驱动专有 Kconfig：

- `drivers/sensor/paw3395/Kconfig`
- `drivers/sensor/WIT_HWT906/Kconfig`

驱动专有构建文件：

- `drivers/sensor/paw3395/CMakeLists.txt`
- `drivers/sensor/WIT_HWT906/CMakeLists.txt`

相关符号：

- `CONFIG_PAW3395`
- `CONFIG_PAW3395_TRIGGER`
- `CONFIG_WIT_HWT906`
- `CONFIG_WIT_HWT906_THREAD_STACK_SIZE`
- `CONFIG_WIT_HWT906_THREAD_PRIORITY`

两个驱动都放在标准 Zephyr sensor 子系统下，并使用 `CONFIG_SENSOR_INIT_PRIORITY` 进行设备初始化。

## Zephyr Sensor API 契约

两类驱动均实现：

- `sensor_sample_fetch(dev, chan)`
- `sensor_channel_get(dev, chan, val)`

应用侧通过常规 sensor 辅助函数消费它们，而非仓内特定包装 API。

## PAW3395

### 范围

文件：

- `drivers/sensor/paw3395/paw3395.c`
- `drivers/sensor/paw3395/paw3395.h`
- `dts/bindings/sensor/pixart,paw3395.yaml`

该驱动将 PixArt PAW3395 光学传感器以 Zephyr 设备形式暴露在 SPI 总线上。

### Devicetree 绑定

绑定文件：

- `dts/bindings/sensor/pixart,paw3395.yaml`

兼容字符串：

- `pixart,paw3395`

包含的模式定义：

- `spi-device.yaml`
- `sensor-device.yaml`

属性：

- `cs-gpios`（必需）
- `ripple`（可选，boolean）
- `dpi`（可选，int，默认 `20000`）

### 设备模型

静态配置：

- `struct spi_dt_spec bus`
- `struct gpio_dt_spec cs_gpio`
- `dpi`
- `ripple_control`

运行时数据：

- 每次抓取的 `dx`、`dy`
- 累计 `sum_x`、`sum_y`
- 连续两次抓取间的 `sum_x_last`、`sum_y_last`
- 运动标志

### 运行时行为

`paw3395_init()`：

- 配置 CS GPIO
- 检查 SPI 总线就绪状态
- 执行上电流程
- 下发固定的“游戏键鼠风格”配置序列
- 从 devicetree 设置 DPI
- 设置断开行为
- 可选使能 ripple 模式
- 读取并记录 product ID

`paw3395_sample_fetch()`：

- 读取运动寄存器
- 若检测到运动，读取 Delta X 与 Delta Y 寄存器
- 同时累加总量与“自上次读取”增量
- 当无运动时清除瞬时位移量

`paw3395_channel_get()` 支持：

- `SENSOR_CHAN_POS_DX`
- `SENSOR_CHAN_POS_DY`

上报数值按配置 DPI 从计数转换为距离。每次成功读取通道时，对应的 `sum_*_last` 累加器会清零。

### 错误处理与边界条件

- 不支持的通道返回 `-ENOTSUP`
- 当 CS GPIO 或 SPI 总线不可用时，初始化返回 `-ENODEV`
- product ID 不匹配会记录日志，但当前不会失败初始化
- 即便未检测到运动，抓取流程仍返回成功

实现中 `paw3395_powerup()` 与相关辅助函数包含较长的寄存器配置序列。后续维护应与已验证可工作的传感器上电流程保持一致，不应在未经过硬件验证的情况下擅自简化。

当前没有 PAW3395 的仓内示例或专有测试。

## WIT HWT906

### 范围

文件：

- `drivers/sensor/WIT_HWT906/wit_hwt906.c`
- `drivers/sensor/WIT_HWT906/wit,hwt906.yaml`

该驱动通过 UART 暴露 WIT HWT906 9 轴 IMU。

### Devicetree 绑定

绑定：

- `drivers/sensor/WIT_HWT906/wit,hwt906.yaml`

兼容：

- `wit,hwt906`

绑定特征：

- 包含 `sensor-device.yaml`
- 声明 `bus: uart`
- 目前无驱动特定属性

UART 总线设备通过 `DT_INST_BUS(inst)` 获取。

### 设备模型

静态配置：

- UART 设备指针

运行时数据：

- RX 环形缓冲区
- 解码后报文字段缓存：时间、加速度、陀螺、欧拉角、磁场、温度、电压、版本
- 溢出计数

### 运行时行为

`wit_hwt906_init()`：

- 初始化环形缓冲
- 检查 UART 就绪
- 安装带用户数据的 IRQ 回调
- 使能 UART RX 中断

IRQ 回调将到达字节写入缓冲；当缓冲空间不足时会丢弃最旧字节，并更新溢出计数。

`wit_hwt906_sample_fetch()`：

- 消费缓冲中的 11 字节报文
- 按包头 `0x55` 重新对齐
- 校验和
- 解析报文族 `0x50` 到 `0x54`
- 若至少处理到一帧有效报文，返回 `0`
- 当无完整有效报文时返回 `-EAGAIN`

`wit_hwt906_channel_get()` 支持：

- `SENSOR_CHAN_ACCEL_XYZ`
- `SENSOR_CHAN_GYRO_XYZ`
- `SENSOR_CHAN_ROTATION`
- `SENSOR_CHAN_MAGN_XYZ`
- `SENSOR_CHAN_DIE_TEMP`
- `SENSOR_CHAN_VOLTAGE`
- `SENSOR_CHAN_PRIV_START`（时间）
- `SENSOR_CHAN_PRIV_START + 1`（固件/版本）

支持转换的上报值会转换为标准单位：

- 加速度：m/s^2
- 陀螺：rad/s
- 欧拉角：度
- 温度：摄氏度
- 电压：V

### 错误处理与边界条件

- 不支持的通道返回 `-ENOTSUP`
- 当 UART 设备未就绪时初始化返回 `-ENODEV`
- `sample_fetch()` 在完整有效报文未到达前返回 `-EAGAIN`
- 报文对齐错误与校验和失败会丢弃字节，直到找到有效帧边界

`CONFIG_WIT_HWT906_THREAD_STACK_SIZE` 与 `CONFIG_WIT_HWT906_THREAD_PRIORITY` 这两个 Kconfig 项存在，但当前驱动实现未消费。

## 消费方、示例与验证

主要示例：

- `samples/wit_hwt906_test`

示例覆盖：

- `sensor_sample_fetch()`
- 通过 `SENSOR_CHAN_PRIV_START` 读取时间通道
- 加速度、陀螺、旋转、磁场通道

其他消费者：

- `samples/IMU/IMU_Calibrate`
- `lib/ares/ekf/imu_task.c`
- `lib/ares/mahony/imu_task.c`
