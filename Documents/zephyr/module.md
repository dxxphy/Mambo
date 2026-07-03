# Zephyr 模块集成

Mambo 以 Zephyr 模块方式集成。Zephyr 构建系统加载模块后，会自动获得 Mambo 的 include 路径、Kconfig、CMake 子目录、boards、binding、samples 和 tests。

## 清单与入口

`west.yml` 固定 Zephyr revision，并只导入当前工程需要的上游模块：

| 项目 | 说明 |
| --- | --- |
| `zephyr` | Zephyr RTOS 主仓库。 |
| `cmsis`、`cmsis_6` | ARM CMSIS 支持。 |
| `cmsis-dsp` | EKF 等算法使用的 DSP 库。 |
| `hal_st`、`hal_stm32` | STM32 HAL。 |
| `mcuboot` | Zephyr 引入的 bootloader 模块。 |
| `segger` | SEGGER 调试支持。 |

Manifest 的 `self.west-commands` 指向 `scripts/west-commands.yml`，用于注册项目自定义 west 命令。

## 模块元信息

Zephyr 模块元数据位于 `zephyr/module.yml`。它声明 Mambo 的 CMake 和 Kconfig 入口，并使 Zephyr 能发现本仓库中的 board、binding、sample 和测试。

根 `CMakeLists.txt` 执行以下工作：

- 设置 `ZEPHYR_BASE` 默认路径。
- 导出 `compile_commands.json`。
- 将 `include/` 加入 Zephyr 的 include 与 syscall include 搜索路径。
- 加载 `drivers/`、`lib/` 和 `include/` 子目录。

根 `Kconfig` 加载：

- `drivers/Kconfig`
- `lib/Kconfig`

## 包含路径

公共应用接口位于：

- `include/zephyr/drivers/`
- `include/ares/`

实现内部头文件位于：

- `drivers/**`
- `lib/ares/**`

应用代码应优先 include 公共路径。直接 include 驱动内部头文件会使应用绑定到实现细节。

## Devicetree 绑定

`dts/bindings/` 随模块一起被 Zephyr 加载。应用 overlay 可以直接使用 Mambo compatible，例如：

- `dji,motor`
- `dm,motor`
- `ares,chassis`
- `ares,steerwheel`
- `custom,spi-can-mfd`
- `yinshi,la`

Binding 参考见 `Documents/devicetree-reference.md`。

## 板卡

`boards/` 目录提供 Mambo 自定义 board。使用方式与 Zephyr 标准 board 相同：

```shell
west build -b dm_mc02 mambo/samples/motor/dm_demo
```

板卡文档见 `Documents/platform/boards.md`。

## 系统调用接口

Mambo 的部分驱动 API 使用 Zephyr 系统调用（syscall）机制：

- `motor`
- `chassis`
- `wheel`
- `sbus`

`zephyr_syscall_include_directories(include)` 使 Zephyr 能扫描这些头文件并生成 syscall glue。新增系统调用封装时，应放在 `include/zephyr/drivers/`，并保持 `z_impl_*` 实现与公开声明（public prototype）一致。

## 构建边界

Mambo 模块不应假定应用目录名称。公共构建入口只能依赖 Zephyr 提供的变量、Kconfig 和 devicetree。面向具体业务应用的配置，应放在对应应用目录内，不应进入模块根构建逻辑。
