# 测试与 CI 总览

本文档集合用于维护 `mambo` 仓库的持续集成与 `native_sim` 测试实践。文档记录当前执行边界与工作流程，不作为变更日志。

## 文档清单

- [ci.md](ci.md)
  GitHub Actions 的工作流、触发条件、任务职责、产物、PR 检查、release 构建与本地复现命令。
- [native_sim.md](native_sim.md)
  native_sim 测试范围、运行方式、`module_smoke` / `pid` / `motor_driver_sim` 细节、fake/test CAN 边界与时序约束。
- [adding-tests.md](adding-tests.md)
  新增测试用例与 CI 维护规范。

## 快速入口

- 仅运行 native_sim 全量：

  - `west twister -T tests/native_sim --platform native_sim/native/64 --inline-logs`

- 仅运行单个测试目录：

  - `west twister -T tests/native_sim/motor_driver_sim --platform native_sim/native/64 --inline-logs`

- 仅复现 CI 常见步骤：

  - 格式检查、native_sim、样例构建、PR 校验、发布构建见 [ci.md](./ci.md)。

## 范围说明

CI 主要覆盖三类能力：

- 源码质量与约束执行（pre-commit、SPDX、PR 元信息）。
- `native_sim` 功能回归。
- 关键样例的可构建性与发布物输出。

以下能力不在当前 CI / native_sim 直接覆盖范围：

- 硬件 CAN 总线物理行为。
- 电机电源侧瞬态、供电与热特性。
- 上板实时性与中断抖动的实测边界。
- 运行期跨板间接口兼容性演练。

## 术语与入口

- native_sim：`tests/native_sim/*` 下在主机平台运行的单元与驱动层验证。
- CI 任务：`.github/workflows/*.yml` 中定义的工作流任务。
- 打包产物：样例构建与 release 任务上传到 GitHub Actions artifact 的二进制文件。
