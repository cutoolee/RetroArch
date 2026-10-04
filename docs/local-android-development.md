# Local GameGo Android Development

首次配置：把 `tools/gamego-dev.env.example` 复制为 `~/.config/gamego/dev.env`，填写现有 GameGo development keystore 的本机密码，并执行 `chmod 600 ~/.config/gamego/dev.env`。脚本不会生成或覆盖 keystore。

```sh
./tools/gamego-dev doctor
./tools/gamego-dev test
./tools/gamego-dev build
./tools/gamego-dev install
./tools/gamego-dev run
```

`install` 默认先构建、校验 package/签名/ABI，再以 `adb install -r` 安装到 `2419TNW44608`。可用 `GAMEGO_DEVICE_SERIAL=xxx` 覆盖设备，或用 `install --no-build` 安装已有 APK。

包名为 `com.retroarch.aarch64`，应用名称为 `RetroArch (AArch64)`。第三方前端选择 RetroArch 64 位，游戏启动入口为 `com.retroarch.browser.retroactivity.RetroActivityFuture`。从 `com.cutoolee.gamego` 切换包名会创建独立的应用数据目录，旧包的配置和核心不会自动迁移；若已安装官方同包名版本且签名不同，不能直接覆盖安装。

## 真机 E2E

必须从固定入口开始。该命令会构建并校验 E2E APK，使用 `adb install -r` 保留应用数据，启动主入口并等待 10 秒后才加载 ROM：

```sh
./tools/gamego-dev e2e-start
```

看到 `E2E_POST_INSTALL_WAIT_SECONDS=10` 后，才执行后续操作：

```sh
./tools/gamego-dev e2e-status
./tools/gamego-dev e2e-input menu
./tools/gamego-dev e2e-input down
./tools/gamego-dev e2e-input confirm
./tools/gamego-dev e2e-evidence
```

标准启动检查点为 `E2E_INSTALL=PASS`、`E2E_FIRST_LAUNCH=PASS`、`E2E_POST_INSTALL_WAIT_SECONDS=10`、`E2E_ROM_LAUNCH=PASS` 和 `CONTENT_RUNNING=PASS`。如果出现 native crash，先保存 `e2e-evidence` 生成的 logcat 和 crash buffer；不要通过卸载应用、清除数据或替换 core 绕过问题。

完整约束见 [GameGo 本地 Android 真机 E2E 固定流程](gamego-local-e2e-runbook.md)。
