# 新增测试与 CI 维护规则

本页约束 native_sim 和 CI 相关的新增流程，目标是保证新增测试在多人并行修改时可快速合并、可重复运行、可定位。

## 新增 native_sim 测试

### 目录结构

标准目录应包含：

- `tests/native_sim/<name>/CMakeLists.txt`
- `tests/native_sim/<name>/prj.conf`
- `tests/native_sim/<name>/testcase.yaml`
- `tests/native_sim/<name>/src/main.c`

`CMakeLists.txt` 使用 Zephyr 最小模板：

```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ares_native_sim_<name>)
target_sources(app PRIVATE src/main.c)
```

### Test 名称与分类

建议命名：

- 套件名：`ares_native_sim_<name>` 或同等命名风格
- testcase 名：`ares.native_sim.<name>`
- tags：
  - `ares`
  - `native_sim`
  - 业务域标签（如 `pid`、`motor`）
- 平台：
  - `platform_allow: [native_sim/native/64]`
  - `integration_platforms: [native_sim/native/64]`

### `prj.conf` 最低要求

- 所有测试：`CONFIG_ZTEST=y`
- 与控制算法相关：按需打开 `CONFIG_CBPRINTF_FP_SUPPORT=y`
- 与驱动相关：仅开启本测试依赖项，避免引入与当前执行器无关、host 环境不可满足的组件。

### 断言策略

优先采用可观测行为断言：

- API 返回值与错误码。
- 在线/离线状态变化。
- CAN 帧 `id / dlc / payload / 顺序 / 时间窗口`。
- 超时阈值是否满足预期。

不建议把“内部瞬时计数器”作为第一优先断言，除非该计数器是公共接口。

### 测试粒度

新增用例应分层：

- 新增最小构建验证：`module_smoke` 风格。
- 算法数学行为：`pid` 风格纯函数/数据边界验证。
- 协议与驱动行为：`motor_driver_sim` 风格的状态机 + 帧序列 + 时序检查。

### 运行与自检

提交前至少执行：

```shell
west twister -T tests/native_sim/<name> --platform native_sim/native/64 --inline-logs
```

若新增 `prj.conf` 改动复杂，建议先加 `--no-clean` 定位缓存相关问题并保留构建目录进行复核。

### 文件层次与并行编辑注意

多人协作时，尽量只修改目标测试目录内的文件；避免同时改动公共测试基座的 fixture 与 helper，减少交叉冲突。

## CI 维护规则

### 何时修改 workflow

只有以下情况才需要改 CI：

- 新增/移除样例导致 `build-samples` 矩阵变化。
- `native_sim` 新增需要额外工具链或依赖。
- 发布产物格式和命名规则变更。
- PR 质量检查项（SPDX、PR title）策略调整。

### 修改 `ci.yml` 的守则

- 保持 `push` 与 `pull_request` 分支列表与既有规则一致。
- 若新增依赖，优先放在对应任务，避免全局执行器变慢。
- native_sim 测试步骤应保留：
  - `west init/update`
  - `requirements-base/build-test`
  - `ZEPHYR_TOOLCHAIN_VARIANT=host`
  - `west twister -T mambo/tests/native_sim --platform native_sim/native/64 --inline-logs`
- 构建任务的产物路径仍保留 `build/zephyr/zephyr.{elf,bin,hex}`。

### 修改 `release.yml` 的守则

- 保持构建输入（`requirements`、`build matrix`、SDK 版本）与发布行为一致，避免断开发布物读取路径。
- 如更改打包内容，必须同步更新：
  - `build-info.txt`
  - 工件命名
  - release 上传范围

### PR 检查相关

- `pr-check.yml` 的 `license-check` 与 `code-size-check` 为轻量审计型工作流，变更其逻辑前需确认命中范围与误报边界。
- 关键文件提醒清单中若新增 workflow 文件，应同步评估是否需要加入提醒列表。

### 变更验收清单

新增测试合入前确认：

- `twister` 在独立测试目录可通过。
- `mambo/tests/native_sim` 全量回归不出现必现失败。
- `ci.yml` 相关任务在执行器环境可复现。
- 文档更新同步到 `Documents/tests/*`，不修改无关文档路径。
