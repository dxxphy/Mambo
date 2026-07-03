# GitHub Actions 与测试流水线

本文档覆盖当前仓库所有 CI 入口、触发策略、任务职责、产物与本地复现命令。描述按当前工作流文件逐项对应，不包含改动记录。

## 工作流一览

- `ci.yml`：主 CI。触发 `push`（`master`、`main`、`develop`）、`pull_request`（同分支）、手动触发。
- `pr-check.yml`：PR 元数据检查。触发 `pull_request` 的 `opened/synchronize/reopened`。
- `release.yml`：发布构建。触发 `v*.*.*` tag、`release` 创建事件、手动触发。

## `ci.yml`

### 触发

- push：`master`、`main`、`develop`
- pull_request：`master`、`main`、`develop`
- workflow_dispatch：允许手动启动

### 任务

#### code-format-check

- 执行器：`ubuntu-latest`
- 动作：
  - `actions/checkout@v4`
  - `actions/setup-python@v5`（Python 3.12）
  - 安装 `pre-commit`
  - 执行 `pre-commit run --all-files --show-diff-on-failure`
- 作用：统一格式规则与钩子检查。

#### native-sim-tests

- 执行器：`ubuntu-22.04`
- 关键步骤：
  - 代码签出到 `mambo`
  - 安装系统构建依赖
  - `west init -l mambo`
  - `west update`
  - 安装 Zephyr Python 依赖：
    - `zephyr/scripts/requirements-base.txt`
    - `requirements-build-test.txt`
    - `natsort`
  - 以 host 工具链执行测试：

    ```shell
    west twister -T mambo/tests/native_sim --platform native_sim/native/64 --inline-logs
    ```

  - 环境变量：`ZEPHYR_TOOLCHAIN_VARIANT=host`
- 作用：验证 native_sim 全量测试用例。

#### build-samples

- 执行器：`ubuntu-22.04`
- 矩阵：
  - `board`
    - `robomaster_board_c`
    - `robomaster_board_a`
    - `dm_mc02`
  - `sample`
    - `samples/motor/dji_m3508_demo`
    - `samples/motor/dm_demo`
  - 排除项：
    - `dm_mc02 + samples/motor/dji_m3508_demo`
- 关键步骤：
  - 安装 `arm-zephyr-eabi` 所需依赖和 Zephyr SDK 缓存
  - `west zephyr-export`
  - 安装 Zephyr 依赖
  - `west build -b <board> mambo/<sample> --pristine`
  - 生成构建产物名 `build-<board>-<sample-without-slash>`
  - 上传制品：
    - `build/zephyr/zephyr.elf`
    - `build/zephyr/zephyr.bin`
    - `build/zephyr/zephyr.hex`
- 保留天数：7 天

#### static-analysis

- 执行器：`ubuntu-latest`
- 关键步骤：
  - 安装 clang-tidy
  - 对 `drivers` 目录执行 readability 与 bugprone 检查（注意命令尾部为 `|| true`，本任务仅用于提示）

## `pr-check.yml`

### 触发

- pull_request：`opened`、`synchronize`、`reopened`

### 任务

#### pr-title-check

- 使用 `amannn/action-semantic-pull-request@v5`
- 允许的 type：`feat`、`fix`、`docs`、`style`、`refactor`、`test`、`chore`
- `requireScope: false`

#### code-size-check

- 输出 `origin/${{ github.base_ref }}...HEAD` 的新增/删除行数
- 当新增行数 `> 1000` 时输出警告

#### license-check

- 检查 PR 新增文件（`--diff-filter=A`）中 `.c/.cpp/.h/.py/.cmake` 是否包含 `SPDX-License-Identifier`
- 不满足条件时失败

#### critical-files-check

- 检查是否修改以下关键文件：
  - `.github/workflows/ci.yml`
  - `west.yml`
  - `.pre-commit-config.yaml`
- 命中时在日志中输出提示，不阻塞合并

#### pr-comment

- PR 打开时，使用 `actions/github-script@v7` 追加欢迎与自检清单评论

## `release.yml`

### 触发

- tag：`v*.*.*`
- release：`created`
- workflow_dispatch：带 `version` 输入参数

### 构建参数

- 执行器：`ubuntu-22.04`
- 同 `ci.yml` 的 board/sample 矩阵（同样排除 `dm_mc02 + samples/motor/dji_m3508_demo`）
- 使用 Zephyr SDK `0.16.8`（优先缓存，缺失时下载并执行 `setup.sh`）
- 构建命令：

```shell
west build -b <board> <sample> --pristine
```

### 包与产物

- 在 `motor/build/release/<board>-<sample>-<version>/` 生成：
  - `zephyr.elf`
  - `zephyr.bin`
  - `zephyr.hex`
  - `build-info.txt`
- 发布压缩包：
  - `<package>.tar.gz`
  - `<package>.zip`
- 上传制品：
  - 名称包含 `firmware-<board>-<sample>-tar` / `firmware-<board>-<sample>-zip`
- `release` 事件触发时写入 GitHub Release 附件
- 制品保留：90 天

## 本地复现

以下命令默认在仓库根目录执行（`mambo`）：

### native_sim

```shell
west twister -T mambo/tests/native_sim --platform native_sim/native/64 --inline-logs
```

```shell
west twister -T mambo/tests/native_sim/motor_driver_sim --platform native_sim/native/64 --inline-logs --verbose
```

```shell
west twister -T mambo/tests/native_sim/motor_driver_sim --platform native_sim/native/64 --inline-logs --no-clean
```

### 样例构建（复核 `build-samples`）

```shell
west build -b robomaster_board_c mambo/samples/motor/dm_demo --pristine
west build -b dm_mc02 mambo/samples/motor/dm_demo --pristine
```

### PR 元检查（本地核对）

```shell
git diff --stat origin/${BASE_REF}...HEAD
git diff --name-only --diff-filter=A origin/${BASE_REF}...HEAD
```

按文件后缀过滤 SPDX 检查时可复用 CI 命令逻辑。`BASE_REF` 建议使用 `master`、`main` 或当前目标基线分支。

## 维护建议（用于 PR 准备）

- 修改 workflow 时保持触发条件与分支策略一致，避免无意扩大影响范围。
- 更新 native_sim 测试前先确认本地 `west twister` 可复现。
- release 流程修改时同步确认：
  - 版本号来源路径（tag / 手工输入）
  - SDK 版本与安装路径
  - 制品命名与上传范围
