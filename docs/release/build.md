# 从源码构建

使用对应 Release 的完整 source.tar.gz，或克隆对应标签。`pkg/`、依赖、构建脚本和许可文件都是源码交付的一部分，不得删除。外部前端资源由 `handheld/distribution/resources.lock` 锁定并校验 SHA256。

## 本地快速检查

```sh
python3 tools/gamego-release.py audit
bash validation/p2/tests/test_quick_menu.sh
sh validation/p2.1/tests/test_bridge.sh
bash validation/controllers/tests/test_input_action.sh
```

## Android ARM64

需要 JDK 25、Android SDK 36、Build Tools 36.0.0、NDK 29.0.14206865；Gradle 版本使用仓库 wrapper。

```sh
handheld/distribution/prepare_android_assets.sh
cd pkg/android/phoenix
./gradlew assembleAarch64Debug -PGAMEGO_PRODUCT=1 -PHAVE_HANDHELD_RUNTIME=1 -PHAVE_HANDHELD_QUICK_MENU=1
```

资源脚本拒绝覆盖已有 assets 目录；本地已有资源时先核对版本，或使用 ANDROID_ASSETS_DIR 指定空的 staging 位置。调试构建不等于正式发布包。

正式包额外传入 `-PGAMEGO_RELEASE=1`、固定 `GAMEGO_VERSION_CODE`、`GAMEGO_APPLICATION_ID` 和正式签名属性。密码使用 Gradle 环境变量 `ORG_GRADLE_PROJECT_RELEASE_STORE_PASSWORD` 等注入，不写入命令行或仓库。完整参数见 `.github/workflows/GameGo-Release.yml`。

## Linux / Windows

安装平台工具链和图形、音频依赖后：

```sh
./configure --disable-qt
make -j2 HAVE_GAMEGO_PRODUCT=1
```

Windows 使用 MSYS2 MINGW64 环境和 MinGW 编译器。打包时必须收集实际依赖 DLL 和其许可。Linux 发行包基于 Ubuntu 22.04 构建，不承诺在更旧 glibc 系统运行。

`tools/gamego-release.py stage` 将可执行文件、前端资源、GameGo 资源、配置、启动器、许可和 BUILD.json 汇总到便携目录。

## macOS / iOS

使用 Xcode，复用稳定 DerivedData 路径。macOS 使用 RetroArch scheme；iOS 使用 RetroArch iOS Release scheme，附加 `-xcconfig pkg/apple/GameGo.xcconfig`。无签名构建设置 `CODE_SIGNING_ALLOWED=NO`，但它不会自动变成可安装的签名应用。

资源通过 `tools/gamego-release.py apple-assets` 生成 assets.zip，并放在应用现有资源解压机制读取的位置。GitHub 发布构建不运行模拟器，也不创建模拟器克隆。

本机需要运行测试时遵循 AGENTS.md 的单设备、串行和存储约束。真机调试另需用户授权。
