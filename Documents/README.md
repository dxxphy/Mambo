# Mambo 文档

本文档集面向维护者和应用开发者，描述 Mambo 的模块边界、公共接口、设备树契约和测试入口。文档仅记录当前代码对外承诺的行为，不作为变更记录使用。

## 文档索引

### 总览

- [模块参考](modules.md): 仓库中各子系统的职责、依赖关系和维护边界。
- [公共 API 参考](api-reference.md): 驱动接口、ARES 通信接口、算法库接口和内部公共接口。
- [设备树参考](devicetree-reference.md): 本仓库提供的 devicetree binding 及主要属性。

### 子系统索引

- [电机](motor/README.md): 电机 API、控制器、CAN 调度、link/telemetry 与厂商驱动。
- [驱动子系统](drivers/README.md): 底盘、轮组、SBUS、PID、传感器与图形。
- [通信协议](communication/README.md): ARES 接口、协议、上下位机链路、板间通信与 VOFA。
- [VCAN](vcan/readme.md): SPI 虚拟 CAN 桥的使用和实现说明。
- [平台](platform/README.md): 板级定义、应用结构、构建和烧录边界。
- [样例](samples/README.md): 样例分类、硬件依赖和维护规则。
- [测试与 CI](tests/README.md): CI、native_sim 与新增测试规范。
- [Zephyr](zephyr/README.md): Mambo 作为 Zephyr 模块的集成方式。

## 约定

Mambo 以 Zephyr 的设备模型为主接口。对应用而言，稳定入口通常位于
`include/zephyr/drivers/` 和 `include/ares/`。`drivers/**` 下同名头文件主要服务驱动实现和
devicetree 实例展开，除非文档明确列出，不建议作为应用层接口直接依赖。

公共接口文档采用以下分类：

- **API**：可直接在应用代码中调用的接口。
- **驱动 API**：Zephyr 设备 `api` 表中的实现回调，由驱动提供，应用通常通过包装器调用。
- **Internal API**：子系统内部共享接口，可跨驱动使用，但边界保持在子系统范围内。
- **Binding**：devicetree 节点与属性契约。

## 维护原则

新增模块时应同时补充三类信息：

1. 模块在 `Documents/modules.md` 中的职责和边界。
2. 对外函数、结构体或宏在 `Documents/api-reference.md` 中的接口说明。
3. `devicetree` binding 在 `Documents/devicetree-reference.md` 中的属性说明。
4. 模块专属文档目录中的维护者文档。

接口文档应描述调用者可依赖的行为、参数含义、返回值和错误边界。实现细节仅在影响调用契约时记录。
