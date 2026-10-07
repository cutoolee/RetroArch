# RetroArch Android 真机 E2E 固定流程

本流程用于 `com.retroarch.aarch64` 的本地开发和真机 E2E。它使用本机 Android 默认 debug keystore，保留同包名应用数据并通过覆盖安装更新 APK。

## 固定启动顺序

不要在 APK 安装完成后直接通过 `RetroActivityFuture` 加载 ROM。Android 首次安装后可能仍在完成 `base.apk` 解压、dex 优化和应用初始化；ROM 过早启动会造成不稳定启动或误判内容崩溃。

统一使用：

```sh
./tools/retroarch-android-dev e2e-start
```

该命令固定执行以下顺序：

1. 构建并校验 E2E APK 的包名、签名和 ABI。
2. 使用 `adb install -r` 覆盖安装，不卸载、不清除数据。
3. 清理本轮 logcat，停止旧进程。
4. 启动 `MainMenuActivity`，完成一次安装后的首次应用启动。
5. 等待 10 秒，让 `base.apk` 解压及应用初始化完成。
6. 停止预热进程，再从全新进程启动 `RetroActivityFuture`，加载同一 ROM 和 core。

输出中的 `E2E_POST_INSTALL_WAIT_SECONDS=10` 是本流程的必要门槛。只有看到它之后，才允许判断 ROM 启动、内容运行、Bridge 或 Quick Menu 状态。

## 后续 E2E 操作

```sh
./tools/retroarch-android-dev e2e-status
./tools/retroarch-android-dev e2e-input menu
./tools/retroarch-android-dev e2e-input down
./tools/retroarch-android-dev e2e-input confirm
./tools/retroarch-android-dev e2e-evidence
```

如果进程已经发生 native crash，工具应报告 `PROCESS_CRASHED`；只有进程仍存活但没有 Bridge 时，才报告 `BRIDGE_UNAVAILABLE`。

## 真机和数据安全约束

- 设备默认为 `2419TNW44608`，可用 `GAMEGO_DEVICE_SERIAL` 覆盖。
- ROM 和 core 可分别用 `GAMEGO_E2E_ROM`、`GAMEGO_E2E_CORE` 覆盖。
- 禁止 `adb uninstall`、`pm clear`、删除 core、save/state 或 config。
- applicationId 固定为 `com.retroarch.aarch64`；本地 debug APK 使用 `~/.android/debug.keystore` 签名。
- 测试失败时先保存 logcat 和 crash buffer，再分析，不通过替换配置来绕过失败。

## 标准检查点

```text
E2E_INSTALL=PASS
E2E_FIRST_LAUNCH=PASS
E2E_POST_INSTALL_WAIT_SECONDS=10
E2E_ROM_LAUNCH=PASS
CONTENT_RUNNING=PASS
```

后续对话应先阅读本 runbook，并从 `e2e-start` 开始；不要自行改成“安装后立即加载 ROM”。
