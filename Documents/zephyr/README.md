# Zephyr 集成

本目录描述 Mambo 作为 Zephyr 模块接入构建系统的方式，以及模块级 Kconfig 入口。

## 文档索引

- [module.md](module.md)
  Mambo 作为 Zephyr 模块的清单、CMake、Kconfig、include、binding 和 board 集成方式。
- [kconfig.md](kconfig.md)
  模块 Kconfig 入口、主要配置项和维护规则。

## 边界

Zephyr 集成文档只描述模块清单、CMake、Kconfig、include、binding 和 syscall 等模块接入契约。
应用目录结构、overlay、构建和烧录入口见 [平台应用文档](../platform/apps.md)。
具体驱动、库、测试和样例的使用方式由对应模块文档维护。
