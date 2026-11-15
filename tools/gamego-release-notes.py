#!/usr/bin/env python3
"""Generate release notes from actual job outcomes."""

import json
import os

results = json.loads(os.environ["BUILD_RESULTS"])
version = os.environ["RELEASE_VERSION"]
print("# GameGo " + version)
print("\n公开预览候选。构建成功不代表已完成安装、升级或游戏运行验收。")
print("\n| 构建任务 | 结果 |\n| --- | --- |")
for name, result in results.items():
    print("| " + name + " | " + result["result"] + " |")
print("\n仅附成功生成的产物；失败或跳过的任务不提供安装包。")
print("\n- Android APK 使用发布证书，实际包名、版本码及证书指纹见 android-signing.txt。")
print("- macOS 产物标注 adhoc，未完成 Developer ID 签名或公证。")
print("- iOS 产物标注 unsigned，需用户自行签名；默认不附带核心，尚不能宣称可直接游玩。")
print("- Windows/Linux 为便携包；Linux 基于 Ubuntu 22.04，依赖见包内列表。")
print("- 不附带游戏 ROM、BIOS 或第三方模拟器核心。")
print("- 完整对应源码保留平台工程；外部资源版本见 resources.lock。")
print("\n下载后对照 SHA256SUMS。安装和存档迁移见源码中的 docs/release/install.md。")
print("\n安全扫描和人工核对记录见源码中的 docs/release/security-audit.md。")
print("\n维护者公开草稿前须补齐各平台实际验收结果和已知问题。")
