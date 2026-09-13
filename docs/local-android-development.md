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
