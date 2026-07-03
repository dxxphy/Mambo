# 电机子系统

本目录收纳 motor 子系统的维护者文档。接口、设备树和总体架构文档已经分别说明 API 约束、节点写法和层次划分；这里补充控制器、公用 CAN 调度、链路遥测以及各厂商驱动的维护要点。

## 文档索引

- [API 接口](api.md)
  统一应用接口、setpoint/status 语义和控制器选择规则。
- [设备树](devicetree.md)
  电机节点、控制器节点和常用属性。
- [架构](architecture.md)
  子系统分层、通用数据流和公共组件职责。
- [控制器](controllers.md)
  控制器模型、匹配规则、参数组织和驱动接入约束。
- [CAN 调度器](can-scheduler.md)
  motor 公共 CAN 发送调度器、优先级和回复跟踪。
- [链路与遥测](link-telemetry.md)
  在线状态机、遥测输出和常见维护边界。
- [驱动](drivers.md)
  DJI、DM、MI、RS、LK、VESC 和 Yinshi LA 驱动说明。

## 阅读顺序

建议先读 `api.md`、`devicetree.md` 和 `architecture.md`，再根据维护任务进入下面几类主题：

- 修改控制器匹配、参数装载或模式扩展时，先看 `controllers.md`。
- 修改 CAN 发送节奏、回复超时或总线公平性时，先看 `can-scheduler.md`。
- 修改在线判定、离线日志或链路恢复策略时，先看 `link-telemetry.md`。
- 修改具体协议驱动、节点属性或反馈映射时，直接看 `drivers.md`。

## 维护范围

`motor` 目录里的旋转电机驱动共享 `include/zephyr/drivers/motor.h` 接口。`yinshi,la` 属于执行器驱动，协议位于同一子系统目录下，但对外接口是 `linear_actuator.h`，不参与旋转电机控制器模型。
