# 发布流程

## 版本和源码

本分支版本保存在 `GAMEGO_VERSION`，当前候选为 `0.1.0-beta.2`。上游版本保留在 `version.all`。版本标签使用 `retroarch-v0.1.0-beta.2`，每次公开发行使用新标签；不要重新指向已经公开的标签。

1. 提交所有需要交付的修改，并确保工作区没有未跟踪的源码。
2. 运行 GameGo Checks 检查和密钥审计，确认外部资源版本。
3. 在 Actions 手动运行 RetroArch Release，保留 create_release 选项；也可推送 `retroarch-v` 标签触发。
4. 工作流生成各端产物、完整源码、许可、SHA256SUMS 和 Release 草稿。
5. 在候选包上执行平台验收，更新平台状态和已知问题。
6. 完成验收后公开预发布；稳定版只包含已验证的平台。

完整源码包基于发布 commit 的 git archive，保留 pkg/、依赖和构建资料。GitHub 自动生成的源码 ZIP 不代替需要另行提供的外部源码/资源版本信息。

## Android 签名

仓库 Actions secrets：

- `GAMEGO_ANDROID_KEYSTORE_B64`
- `GAMEGO_ANDROID_STORE_PASSWORD`
- `GAMEGO_ANDROID_KEY_ALIAS`
- `GAMEGO_ANDROID_KEY_PASSWORD`

仓库 Actions variables：

- `GAMEGO_ANDROID_APPLICATION_ID`：默认 `com.retroarch.aarch64`，应用名称保留 `RetroArch (AArch64)`。
- `GAMEGO_ANDROID_VERSION_CODE`：可选覆盖值，整数且大于上一公开版；默认由版本号递增计算，beta.1 为 10021，beta.2 为 10022。

缺少正式签名时 Android 发布任务失败，不生成伪装成正式包的 Debug 签名 APK。密钥需离线备份；不得提交 Git 或放入公开 artifact。旧开发包与正式发布证书可能不同，迁移方法见安装指南。

## Apple 签名

初始 macOS 包可以使用明确标注的 ad-hoc 签名；它不是 Developer ID 签名，也未完成公证。iOS 默认输出 unsigned IPA，不包含 provisioning profile。正式 Apple 分发需要维护者的开发者账号、有效证书和相应分发配置；不能用模拟器构建代替真机安装包。

## 发布清单

- 源码 commit、本分支 / RetroArch 上游版本和外部依赖版本一致。
- 安装包包含字体、图标、着色器、许可和安装说明。
- 不包含游戏、BIOS、用户数据、测试入口或签名密钥。
- 完整源码包可用于还原平台工程和构建脚本。
- SHA256SUMS 覆盖全部交付文件。
- 每个平台明确构建、签名、公证和运行验收状态。
- 存档升级验证通过，未解决问题写入 Release 说明。

CI artifact 用于候选包检查，GitHub Release 才是面向用户的下载入口。发布草稿不代表已经公开发行。
