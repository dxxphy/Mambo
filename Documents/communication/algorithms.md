# 算法层

ARES 算法相关代码集中在姿态估计、基础数学工具、标定矩阵持久化和 IMU 触发任务上。
这些模块不依赖 ARES 通信栈，可以独立用于板载姿态解算。

仓库当前包含两条姿态算法线：

- 四元数 EKF
- Mahony AHRS

## 组成

主要目录如下：

- `include/ares/ekf/`
- `include/ares/mahony/`
- `lib/ares/ekf/`
- `lib/ares/mahony/`
- `lib/ares/vofa/` 之外的调试输出不在本章范围

## 四元数 EKF

### 角色

Quaternion EKF 提供基于四元数的姿态估计，并维护：

- 四元数状态
- 陀螺零偏
- 加速度计偏置与比例因子
- Roll/Pitch/Yaw
- 收敛、稳定和卡方检验状态

公开定义在 `include/ares/ekf/QuaternionEKF.h`。

### 公开接口

- `IMU_QuaternionEKF_Init()`
- `IMU_QuaternionEKF_Predict_Update()`
- `IMU_QuaternionEKF_Measurement_Update()`
- `CalcBias()`

全局状态对象：

- `QEKF_INS`

### 初始化语义

EKF 初始化依赖一个初始四元数。该四元数不是固定常量，而是由 IMU 任务在启动阶段通过静态加速度样本估计。

典型顺序：

1. 采样加速度与陀螺数据。
2. 估计重力方向。
3. 构造初始四元数。
4. 调用 `IMU_QuaternionEKF_Init()`。
5. 使能传感器触发。

维护者调整算法参数时，应优先把“初值建立阶段”和“运行更新阶段”分开看。

### 运行时行为

EKF 路径将更新拆为两类：

- predict：由陀螺触发
- measurement update：由加速度计触发

这与 `imu_task.c` 中对 `SENSOR_CHAN_GYRO_XYZ` 和 `SENSOR_CHAN_ACCEL_XYZ` 的分支完全对应。

### 误差与边界

- 没有持久化标定时，算法仍会运行，但记录告警并退回默认偏置/比例。
- 初始静态采样若没有找到足够接近重力模长的样本，会退回 `[0, 0, PHY_G]`。
- `gyro_dt` 和 `accel_dt` 依赖 `k_cycle_get_32()`，因此触发时间戳的一致性直接影响估计质量。

## Mahony AHRS

### 角色

Mahony 路径提供较轻量的姿态估计，支持：

- 纯陀螺积分更新
- IMU 更新
- 包含磁力计的姿态更新

公开定义在 `include/ares/mahony/MahonyAHRS.h`。

### 公开接口

- `MahonyAHRSupdate()`
- `MahonyAHRSupdateIMU()`
- `MahonyAHRSupdateGyro()`

全局状态对象：

- `MahonyAHRS_INS`
- `q0`、`q1`、`q2`、`q3`
- `twoKp`、`twoKi`

### 运行模型

Mahony 的 IMU 任务与 EKF 类似，也由传感器触发驱动，但额外维护磁力计数据。
在加速度触发分支中会同时取：

- 加速度
- 陀螺
- 磁力计

随后：

1. 做 bias/beta 修正。
2. 评估磁力计是否可信。
3. 更新姿态。
4. 将四元数转换为 Yaw/Pitch/Roll。

### 磁力计门限

Mahony 路径对磁力计引入简单一致性门限：新磁场向量与当前向量点积绝对值足够大时才采纳。
否则磁场分量被置零。这是运行时稳定性的一部分，不应在维护中随意删去。

### 误差与边界

- 没有持久化标定时，磁力计、加速度计、陀螺都回退到默认偏置。
- 若磁力计瞬态异常，姿态仍可继续运行，只是航向约束会减弱。
- Mahony 路径的头文件与实现共享若干全局对象，修改时需同时检查 ABI 和调用约定。

## IMU 任务

### 角色

`imu_task.c` 负责把具体传感器设备接到算法库。它不是一个通用 Zephyr driver，
而是带有明显系统集成语义的启动和更新入口。

EKF 与 Mahony 都各自带一份 `imu_task` 实现，接口近似，但参数并不完全相同：

- EKF: `IMU_Sensor_trig_init(accel_dev, gyro_dev)`
- Mahony: `IMU_Sensor_trig_init(accel_dev, gyro_dev, mag_dev)`

维护者迁移调用点时，必须区分这两套签名。

### 公开接口

两条实现都暴露：

- `IMU_temp_read()`
- `IMU_Sensor_set_update_cb()`
- `IMU_Sensor_set_IMU_temp()`（条件启用）
- `IMU_Sensor_trig_init()`

### 初始化顺序

以 EKF 路径为例，初始化顺序大体为：

1. 检查温控 PWM 是否启用并可用。
2. 初始化安装误差参数。
3. 保存设备句柄。
4. 进行静态初值采样。
5. 从矩阵存储读取标定数据。
6. 初始化姿态算法。
7. 重置时间戳。
8. 注册传感器触发回调。

Mahony 路径与之相同，只是多了磁力计参与和状态复制。

### 更新回调

`IMU_Sensor_set_update_cb()` 允许外部注册一个“每次更新后调用”的回调。
这通常是算法层对业务层的唯一主动输出路径。

维护者若要做日志、控制闭环或可视化接出，优先使用该回调，而不是在算法内部硬插业务代码。

### 温控

若打开 `CONFIG_IMU_PWM_TEMP_CTRL`：

- IMU 温度通过 `SENSOR_CHAN_DIE_TEMP` 读取。
- PWM 占空比由 PID 计算。
- 目标温度可通过 `IMU_Sensor_set_IMU_temp()` 修改。

这部分能力依赖：

- `DT_CHOSEN(ares_pwm)`
- `imu_temp_pid` devicetree 节点

缺一不可。

### 运行边界

当前实现对 `CONFIG_IMU_PWM_TEMP_CTRL` 的判断较强，未开启时初始化函数会直接报错返回。
维护者若希望算法在无温控硬件时也正常工作，需要先确认调用点是否接受这一行为。

## 数学工具

### 角色

`algorithm.h` / `algorithm.c` 提供算法层基础工具，主要包括：

- 向量范数、点积、叉积
- 角度约束
- 平方根与限幅
- 平均滤波
- 地面加速度推算

同时把 CMSIS-DSP 的矩阵类型和运算接口映射为统一宏：

- `mat`
- `Matrix_Init`
- `Matrix_Add`
- `Matrix_Subtract`
- `Matrix_Multiply`
- `Matrix_Transpose`
- `Matrix_Inverse`

### 维护约束

- 这些工具被 EKF 和 Mahony 共用。
- 能复用 CMSIS-DSP 的地方应继续复用，不要引入第二套矩阵抽象。
- 头文件中有历史遗留注释和宏，维护时应优先保证兼容，而不是做大幅风格重写。

## 矩阵存储

### 角色

`matrix_storage` 用 Zephyr ZMS 在 flash 中持久化两块 `3x3 float` 矩阵。
其职责是保存标定相关矩阵，而不是通用 KV 存储。

公开接口：

- `matrix_storage_save()`
- `matrix_storage_read()`
- `matrix_storage_exists()`
- `matrix_storage_delete()`

### 数据布局

存储内容是一个组合对象：

- `matrix1[3][3]`
- `matrix2[3][3]`

二者总是一起读写。

对 EKF 而言，当前主要用 `matrix1` 保存：

- `AccelBias`
- `AccelBeta`
- `GyroBias`

Mahony 路径还会用到 `matrix2` 保存磁力计相关标定。

### 初始化语义

ZMS 初始化是惰性的，首次读写时才会：

1. 获取 flash page 信息。
2. 校验 sector size。
3. 根据 storage partition 计算 sector count。
4. `zms_mount()`。

模块内部通过 `zms_lock` 保证并发安全。

### 错误边界

- flash device 不 ready：返回 `-ENODEV`
- 分区过小：返回 `-ENOSPC`
- 数据不存在：`matrix_storage_read()` 返回 `-ENOENT`
- 数据长度不匹配：视为损坏，返回 `-EIO`

调用者不应把 `exists()` 与 `read()` 之间视为原子关系；真正需要数据时仍应处理 `read()` 失败。

## 配置

顶层相关配置位于 `lib/ares/Kconfig`：

- `CONFIG_EKF_LIB`
- `CONFIG_MAHONY_LIB`
- `CONFIG_IMU_PWM_TEMP_CTRL`

其中：

- EKF 会选择 `CMSIS_DSP`
- 温控会选择 `PID`

## 最小入口

EKF 路径的典型入口：

```c
IMU_Sensor_set_update_cb(my_update_cb);
IMU_Sensor_trig_init(accel_dev, gyro_dev);
```

Mahony 路径的典型入口：

```c
IMU_Sensor_set_update_cb(my_update_cb);
IMU_Sensor_trig_init(accel_dev, gyro_dev, mag_dev);
```

在这之前，设备树中的传感器、可选 PWM、可选存储分区都应已经可用。
