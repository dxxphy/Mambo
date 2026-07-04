# 应用结构与入口

Mambo 应用使用标准 Zephyr 应用结构。本文只描述应用目录、构建入口和应用级配置边界；
Mambo 作为 Zephyr 模块的加载方式见 `Documents/zephyr/module.md`。

## 目录结构

推荐结构：

```text
app_name/
├── CMakeLists.txt
├── README.md
├── prj.conf
├── sample.yaml
├── app.overlay
├── boards/
│   ├── dm_mc02.overlay
│   └── robomaster_board_c.overlay
└── src/
    └── main.c
```

## 文件边界

- `CMakeLists.txt`
  - 定义工程名与源码集合，不承载硬件参数。
  - 不重新添加 Mambo 的 `drivers/` 或 `lib/` 目录；模块已经由 Zephyr 加载。
- `prj.conf`
  - 配置 Kconfig 开关与功能开销。
- `app.overlay`
  - 与样例语义相关、跨板通用的外设定义。
- `boards/<board>.overlay`
  - 目标板资源映射与引脚复用差异。
- `boards/<board>.conf`
  - 板级构建选项覆盖，通常用于强制关闭某一接口。
- `sample.yaml`
  - Twister/CI 元数据、集成平台与过滤规则。
- `src/*.c`
  - 业务逻辑与运行时流程。

`app/` 目录通常用于本地业务应用，不作为公共样例或 CI 入口。可复用示例放在 `samples/`，
自动化测试放在 `tests/`。

## 最小 CMake

```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(my_app)

target_sources(app PRIVATE src/main.c)
```

## 应用 Kconfig

`prj.conf` 只描述应用需求：

```conf
CONFIG_CAN=y

CONFIG_MOTOR=y
CONFIG_MOTOR_DJI=y
CONFIG_MOTOR_DM=y

CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=3
```

板级固定能力进入 board defconfig。调试日志、应用协议、电机类型和传感器选择进入应用配置。
模块级 Kconfig 选项清单见 `Documents/zephyr/kconfig.md`。

## Devicetree overlay

Overlay 声明设备实例和设备之间的连接关系：

```dts
/ {
    speed_ctrl: speed_ctrl {
        compatible = "motor-controller,vo";
        target = "speed";
        k_p = "2.0";
    };

    motor0: motor0 {
        compatible = "dji,motor";
        status = "okay";
        id = <1>;
        tx_id = <0x200>;
        rx_id = <0x201>;
        can_channel = <&can1>;
        gear_ratio = "19.2";
        controllers = <&speed_ctrl>;
        is_m3508;
    };
};
```

公共设备树契约见 `Documents/devicetree-reference.md`。

## 构建和烧录入口

推荐在仓库根目录执行：

```bash
west build -b <board> <sample_dir>
west flash
```

常见示例：

```bash
west build -b dm_mc02 samples/motor/dji_m2006_verify
west flash

west build -b robomaster_board_c samples/communication/ares_communication -d build/ares_communication
west flash -d build/ares_communication
```

在 west workspace 根目录执行时，样例路径通常带模块目录名：

```bash
west build -b dm_mc02 mambo/samples/motor/dm_demo --pristine
```

构建 native_sim 测试：

```bash
west build -b native_sim/native/64 tests/native_sim/pid --pristine
```

指定烧录执行器：

```bash
west flash --runner openocd
west flash --runner jlink
west flash --runner stlink
```

执行器配置由 board 目录中的 `board.cmake` 和 `support/` 文件提供。

## 可复用约定

- 外设实例化（CAN/UART/SPI/LED）优先放在 overlay，不在 `prj.conf` 中描述节点参数。
- 应用层不新增板级初始化动作；板级初始化由 `lib/ares/board/` 侧统一处理。
- `sample.yaml` 只在支持 Twister/CI 的样例中维护；当前仓库中为：
  - `samples/IMU/IMU/sample.yaml`
  - `samples/IMU/IMU_Calibrate/sample.yaml`
  - `samples/chassis/sample.yaml`
- 新增样例需补充或更新 `sample.yaml` 的 `depends_on` 与 `integration_platforms`，避免 CI 误报。
- 公共样例应能从干净 west workspace 构建，不依赖本地未跟踪文件。
- 硬件依赖写在样例 README 或模块文档中。
