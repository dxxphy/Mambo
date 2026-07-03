# 板级支持

本文档维护三块板卡的板级边界：`dm_mc02`、`robomaster_board_a`、`robomaster_board_c`。

## 文件角色

- `boards/<vendor>/<board>/board.yml`
  - 板卡元信息与 SoC 标识。
- `boards/<vendor>/<board>/board.cmake`
- 烧录执行器与默认参数。
- `boards/<vendor>/<board>/<board>_defconfig`
  - 板级默认 Kconfig 集合。
- `boards/<vendor>/<board>/support/*`
  - `openocd`、`jlink`、`svd` 资源与调试配置。
- `boards/<vendor>/<board>/*.c`
  - `ares_board_power_init()`、`ares_board_status_led_*()` 的实现。
- `lib/ares/board/init.c`
  - 统一初始化入口。

## 1. dm_mc02

- 路径：`boards/damiao/dm_mc02`
- Kconfig：`BOARD_DM_MC02`，SoC `stm32h723xx`
- DTS：`dm_mc02.dts`

### board.cmake 与支持文件

- 执行器：
  - `stm32cubeprogrammer --port=swd --reset-mode=hw`
  - `openocd`
  - `pyocd --target=STM32H723VG`
  - `jlink --device=STM32H723VG --speed=12000`
- support：
  - `support/openocd.cfg`
  - `support/jlink.cfg`
  - `support/STM32H723.svd`

### defconfig 约束

- `CONFIG_ARM_MPU=y`、`CONFIG_HW_STACK_PROTECTION=y`
- `CONFIG_DMA=y`、`CONFIG_GPIO=y`、`CONFIG_SPI=y`、`CONFIG_I2C=y`
- `CONFIG_CAN=y`、`CONFIG_COUNTER=y`、`CONFIG_PWM=y`
- `CONFIG_SERIAL=y`、`CONFIG_CONSOLE=y`、`CONFIG_UART_CONSOLE=y`
- `CONFIG_SENSOR=y`、`CONFIG_BMI08X=y`
- `CONFIG_LOG=y`、`CONFIG_LOG_BACKEND_UART=y`、`CONFIG_LOG_DEFAULT_LEVEL=3`
- `CONFIG_LED_STRIP=y`
- `CONFIG_THREAD_RUNTIME_STATS=y`、`CONFIG_SCHED_THREAD_USAGE_ALL=y`
- `CONFIG_ISR_STACK_SIZE=3200`、`CONFIG_MAIN_STACK_SIZE=3200`

### 板级初始化边界

- `ares_board_power_init()`：仅在 `power1`、`power2` 节点存在时将其配置为输出高电平。
- `ares_board_status_led_set_rgb()`：基于 `led_strip`（WS2812）设置单点颜色。
- `ares_board_status_led_max_channel()`：返回 `0x7e`。
- `ares_board_status_led_init()`：设备就绪后点亮固定灰色并启动 `ares_board_status_led_service_start()`。

## 2. robomaster_board_a

- 路径：`boards/dji/robomaster_board_a`
- Kconfig：`BOARD_ROBOMASTER_BOARD_A`，SoC `stm32f427xx`
- DTS：`robomaster_board_a.dts`

### board.cmake 与支持文件

- 执行器：
  - `stm32cubeprogrammer --port=swd --reset-mode=hw`
  - `openocd`
  - `pyocd --target=STM32F427II`
  - `jlink --device=STM32F427II --speed=4000`
- support：
  - `support/openocd.cfg`
  - `support/jlink.cfg`
  - `support/STM32F427.svd`

### defconfig 约束

- `CONFIG_ARM_MPU=n`、`CONFIG_HW_STACK_PROTECTION=n`
- `CONFIG_CBPRINTF_FP_SUPPORT=y`
- `CONFIG_GPIO=y`
- `CONFIG_CAN_DEFAULT_BITRATE=1000000`
- `CONFIG_LOG=y`
- `CONFIG_CAN=y`
- `CONFIG_ISR_STACK_SIZE=3200`、`CONFIG_MAIN_STACK_SIZE=3200`

### 板级初始化边界

- `ares_board_power_init()`：对 `power1`、`power2`、`power3`、`power4` 进行输出高电平配置。
- `ares_board_status_led_init()`：空实现。
- `ares_board_status_led_set_rgb()`：返回 `-ENOTSUP`。
- `ares_board_status_led_max_channel()`：返回 `0xff`。

## 3. robomaster_board_c

- 路径：`boards/dji/robomaster_board_c`
- Kconfig：`BOARD_ROBOMASTER_BOARD_C`，SoC `stm32f407xx`
- DTS：`robomaster_board_c.dts`

### board.cmake 与支持文件

- 执行器：
  - `stm32cubeprogrammer --port=swd --reset-mode=hw`
  - `openocd`
  - `pyocd --target=STM32F407IG`
  - `jlink --device=STM32F407IG --speed=16000`
- support：
  - `support/openocd.cfg`
  - `support/jlink.cfg`
  - `support/STM32F407.svd`

### defconfig 约束

- `CONFIG_GPIO=y`、`CONFIG_PWM=y`
- `CONFIG_SERIAL=y`、`CONFIG_UART_ASYNC_API=y`、`CONFIG_UART_INTERRUPT_DRIVEN=y`
- `CONFIG_UART_CONSOLE=y`、`CONFIG_CONSOLE=y`
- `CONFIG_SPI=y`、`CONFIG_SPI_STM32_DMA=y`
- `CONFIG_DMA=y`、`CONFIG_DMA_STM32=y`
- `CONFIG_EVENTS=y`、`CONFIG_THREAD_ANALYZER=y`、`CONFIG_SCHED_THREAD_USAGE=y`
- `CONFIG_CAN=y`、`CONFIG_CAN_COUNT=2`
- `CONFIG_FPU=y`
- `CONFIG_ISR_STACK_SIZE=3200`、`CONFIG_MAIN_STACK_SIZE=3200`
- `CONFIG_LOG=y`、`CONFIG_LOG_MODE_IMMEDIATE=y`

### 板级初始化边界

- `ares_board_power_init()`：空实现（不主动配置供电 GPIO）。
- `ares_board_status_led_set_rgb()`：通过 PWM 3 路输出 `led-red/led-green/led-blue`。
- `ares_board_status_led_init()`：点亮灰色后启动 `ares_board_status_led_service_start()`。
- `ares_board_status_led_max_channel()`：返回 `0xff`。

## 通用初始化边界（适用于三块板）

`lib/ares/board/init.c` 在 `APPLICATION` 阶段执行：

- `k_sleep(K_MSEC(550))`
- `ares_board_power_init()`
- 如果 `CONFIG_ARES_BOARD_STATUS_LED` 使能，执行 `ares_board_status_led_init()`
- 打印 `Board init done.`

维护要求：

- 变更板级供电、状态灯、初始化次序只改对应 `lib/ares/board/<board>.c`。
- 变更板级外设依赖时同步更新 `boards/<board>/<board>.dts` 与本目录文档。
