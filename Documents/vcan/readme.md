# VCAN 双路 SPI-CAN 桥

VCAN 在 Host 板上提供两个 Zephyr `can` 设备，实际 CAN 控制器位于另一块 Slave 板上。
Host 与 Slave 之间通过 SPI 交换固定长度同步帧。

## 组成

- Host 驱动：
  - `drivers/vcan/spi_can_mfd.c`
  - `drivers/vcan/spi_can_node.c`
- 设备树 binding：
  - `dts/bindings/can/custom,spi-can-mfd.yaml`
  - `dts/bindings/can/custom,spi-can-node.yaml`
- 示例：
  - `samples/vcan/vcan_host_demo`
  - `samples/vcan/vcan_slave_demo`

## 接口模型

Host 侧应用使用标准 Zephyr CAN API：

- `can_start()`
- `can_stop()`
- `can_send()`
- `can_add_rx_filter()`
- `can_get_state()`
- `can_set_timing()`

`custom,spi-can-node` 子设备对上表现为 CAN 控制器。`custom,spi-can-mfd` 父设备负责 SPI
事务、队列、同步帧和远端状态缓存。

## 当前约束

- 固定支持两个逻辑通道：`channel 0`、`channel 1`。
- 每次 SPI 同步帧固定 `192` 字节。
- 每次同步最多传输 `4` 个 CAN 帧。
- Host 侧只支持 `CAN_MODE_NORMAL`。
- Host 只向 Slave 同步 CAN 帧、通道号和 `bitrate`。
- Slave 本地根据 `bitrate` 调用 `can_calc_timing()`、`can_set_timing()` 和 `can_start()`。

## 同步触发

Host 在以下条件之一成立时发起 SPI 同步：

- 本地任一通道有待发送 CAN 帧。
- 任一通道有待同步 `bitrate`。
- Slave 的 `INT` 引脚为高。
- 上层查询状态并触发强制同步。

Host 有一个固定轮询周期，并在本地发送时使用短聚合窗口。相关实现细节见
[IMPLEMENTATION.md](IMPLEMENTATION.md)。

## 设备树

典型 Host 侧 overlay：

```dts
&spi2 {
	status = "okay";
	cs-gpios = <&gpiob 12 GPIO_ACTIVE_LOW>;

	virtual_can_host: spi_can@0 {
		compatible = "custom,spi-can-mfd";
		reg = <0>;
		spi-max-frequency = <10000000>;
		can-core-clock = <42000000>;
		int-gpios = <&gpiob 8 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>;
		status = "okay";

		can_v0: can_0 {
			compatible = "custom,spi-can-node";
			status = "okay";
			can-channel = <0>;
			bitrate = <1000000>;
			sample-point = <875>;
		};

		can_v1: can_1 {
			compatible = "custom,spi-can-node";
			status = "okay";
			can-channel = <1>;
			bitrate = <1000000>;
			sample-point = <875>;
		};
	};
};
```

属性约束：

- `int-gpios` 连接 Slave 到 Host 的提示线。
- `can-core-clock` 表示远端 CAN 控制器核心时钟，用于 Host 侧 timing 到 bitrate 的换算。
- 子节点 `can-channel` 只能是 `0` 或 `1`。
- 子节点 `bitrate` 是 Slave 自启动时使用的目标 bitrate。

## 样例构建

Host：

```bash
west build -b robomaster_board_c samples/vcan/vcan_host_demo --pristine
west flash
```

Slave：

```bash
west build -b robomaster_board_c samples/vcan/vcan_slave_demo --pristine
west flash
```

## 接线

- `PB12 <-> PB12`：SPI2 CS
- `PB13 <-> PB13`：SPI2 SCK
- `PB14 <-> PB14`：SPI2 MISO
- `PB15 <-> PB15`：SPI2 MOSI
- `PB8 Slave -> Host`：INT
- `CAN1_TX/RX <-> CAN1_TX/RX`
- `CAN2_TX/RX <-> CAN2_TX/RX`
- `GND <-> GND`

## 验证

Host demo 验证四条路径：

- `can1 -> vcan0`
- `vcan0 -> can1`
- `can2 -> vcan1`
- `vcan1 -> can2`

Host 日志应周期性输出每条路径的通过统计。Slave 日志应能看到通道启动、bitrate 配置和
INT 状态变化。若 Host 只有发送统计而无接收统计，优先检查 `INT`、SPI 接线和 Slave 侧
物理 CAN 是否启动。
