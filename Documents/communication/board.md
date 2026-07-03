# 板级支持

ARES 的板级模块提供统一的板级公共入口，用于处理：

- 板载电源使能
- 状态 LED 初始化和更新
- 一个通用的板级启动钩子

实现分为公共部分和按板型落地的实现部分。

## 文件布局

公共入口：

- `lib/ares/board/init.c`
- `lib/ares/board/init.h`

按板型实现：

- `lib/ares/board/robomaster_board_a.c`
- `lib/ares/board/robomaster_board_c.c`
- `lib/ares/board/dm_mc02.c`

注意：`include/ares/board/init.h` 当前只保留了很薄的占位声明，实际公开 API 以
`lib/ares/board/init.h` 中的内容为准。维护这部分时应同时核对包含路径是否满足调用方预期。

## 公共接口

统一 API 包括：

- `ares_board_power_init()`
- `ares_board_status_led_init()`
- `ares_board_status_led_service_start()`
- `ares_board_status_led_set_rgb(const struct ares_led_rgb *color)`
- `ares_board_status_led_max_channel()`

颜色结构：

- `struct ares_led_rgb { uint8_t r, g, b; }`

这些函数由具体板型实现，同名符号在编译期按目标板进入链接结果。

## 公共初始化流程

`init.c` 通过：

- `SYS_INIT(board_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT)`

在应用初始化阶段执行公共板级启动。

顺序如下：

1. 启动后先睡眠约 550 ms。
2. 调用 `ares_board_power_init()`。
3. 若 `CONFIG_ARES_BOARD_STATUS_LED` 打开，则调用 `ares_board_status_led_init()`。

该入口只负责板级公共动作，不做协议绑定或业务启动。

## 状态 LED 服务

### 角色

`ares_board_status_led_service_start()` 会创建一个线程，周期性估计 CPU 占用并映射到 LED 颜色：

- CPU 越高，红色分量越高
- CPU 越低，绿色分量越高

蓝色分量固定为 0。

### CPU 占用来源

若打开：

- `CONFIG_SCHED_THREAD_USAGE_ALL`

则通过线程运行时统计估计空闲率；否则退回一个固定的近似值。

因此状态 LED 服务本质上是“运行态健康指示”，不是精确性能计量工具。

## 板型实现

### RoboMaster A 板

`robomaster_board_a.c` 提供：

- 四路 XT30 电源 GPIO 上电
- LED 接口占位实现

行为特征：

- `ares_board_power_init()` 直接把 `power1` 到 `power4` 拉高。
- `ares_board_status_led_set_rgb()` 返回 `-ENOTSUP`。
- `ares_board_status_led_max_channel()` 返回 `0xff`。

这说明 A 板目前只落实了电源开关，没有真正的状态灯输出设备。

### RoboMaster C 板

`robomaster_board_c.c` 通过三个 PWM LED alias 控制 RGB：

- `led_red`
- `led_green`
- `led_blue`

行为特征：

- `ares_board_power_init()` 为空实现。
- `ares_board_status_led_init()` 检查 PWM 设备可用，点亮默认灰色，再启动状态灯服务。
- `ares_board_status_led_set_rgb()` 将 0..255 缩放到 PWM period。

### DM MC02

`dm_mc02.c` 使用：

- 一个 WS2812 类 LED strip alias
- 可选的两路 XT30 电源 GPIO

行为特征：

- 电源节点存在时才进行配置。
- 状态灯通过 `led_strip_update_rgb()` 输出。
- `ares_board_status_led_max_channel()` 返回 `0x7e`，不是满量程 `0xff`。

这一上限会影响状态灯服务的颜色映射幅度。

## 配置项

相关配置主要来自 `lib/ares/Kconfig`：

- `CONFIG_BOARD_LOG_LEVEL`
- `CONFIG_ARES_BOARD_STATUS_LED`

同时各板型实现还依赖 devicetree 中存在对应节点或 alias。

## 初始化/绑定边界

板级模块本身不需要像通信栈那样绑定协议对象，但它有几个重要前提：

- 板型实现必须与目标 devicetree 对应
- `SYS_INIT` 运行时，相关设备应已进入可用状态
- 状态灯服务线程会长期运行，板型实现应保证 `set_rgb()` 可重复调用

## 错误边界

- PWM 或 LED strip 设备未 ready 时，状态灯初始化记录错误并返回。
- A 板的状态灯设置明确返回 `-ENOTSUP`。
- 电源 GPIO 配置调用当前未集中检查返回码，问题会表现为外设不上电而不是统一初始化失败。

维护者排查板级启动异常时，应把“系统启动成功”和“板载外设真正进入工作态”分开验证。

## 最小入口

通常不需要业务代码手工调用 `board_init()`，因为它已由 `SYS_INIT` 注册。
若业务只想单独使用状态灯统一接口，可直接调用：

```c
struct ares_led_rgb color = { .r = 0x20, .g = 0x40, .b = 0x00 };
ares_board_status_led_set_rgb(&color);
```

前提是对应板型已经完成初始化并且硬件支持该能力。
