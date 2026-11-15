# 安装、升级与存档迁移

## 获取与校验

从 [Releases](https://github.com/cutoolee/RetroArch/releases) 下载。macOS/Linux 在下载目录执行 `shasum -a 256 -c SHA256SUMS`（只保留你下载文件对应的校验行）；Windows PowerShell 使用 `Get-FileHash 文件名 -Algorithm SHA256` 对照 SHA256SUMS。

包名中的平台、架构与 `unsigned` 标记有实际含义。实验性构建尚未完成运行验收。

## Android

安装 ARM64 APK，按系统提示允许该来源安装。新发布渠道采用固定发布证书，后续版本使用同一证书覆盖升级。若系统提示签名不一致，先备份，不要直接卸载。

正式包的 applicationId 由发布配置指定，Release 说明列出实际值。本地开发默认仍可使用 `com.retroarch.aarch64`。正式包保留 `RetroArch (AArch64)` 名称和 `com.retroarch.aarch64` 包名。若已有官方同包名应用或旧开发包，但签名不同，无法覆盖安装；必须先导出数据，再处理旧包。正式发布证书不会自动继承开发包证书。

通过系统文件选择器授权游戏目录；核心从高级菜单的核心管理入口安装或手动导入。某些设备还需授予文件管理权限。前端启动入口仍为 `com.retroarch.browser.retroactivity.RetroActivityFuture`；第三方启动器应使用实际 applicationId，不应假定独立 GameGo 等于官方 RetroArch 包。

## Windows / Linux

解压到可写目录，通过 `GameGo.cmd` 或 `./GameGo` 启动。保留目录结构；不要只复制可执行文件。配置、核心和存档使用便携目录。Linux 包不是跨所有发行版通用的 AppImage；最低系统和库要求以 Release 说明为准。

## macOS

解压并将 GameGo.app 放入 Applications。是否完成 Developer ID 签名和公证以 Release 为准。未公证构建可能被 Gatekeeper 阻止；仅在你信任并校验来源后，通过系统提供的安全设置批准打开，不建议全局关闭安全保护。

## iOS

标注 `unsigned` 的 IPA 不能直接点击安装，需要使用你自己的 Apple 签名身份和支持的侧载工具重新签名。开发签名的有效期、设备限制和续签规则由 Apple 账号及分发方式决定。

初始流水线不附带第三方核心。iOS 核心需要兼容的 arm64 构建并满足代码签名要求，不能直接使用 Android 的 .so。未补齐核心并完成运行验收前，IPA 只作为实验性前端产物，不宣称可直接游玩。此渠道不等于 App Store 或 TestFlight 发布。

## 升级和迁移

1. 在旧版的“设置 → 目录”记录实际配置、存档、即时存档、系统/BIOS、播放列表路径。
2. 退出游戏并关闭应用，将上述数据复制到备份位置。
3. 同包名同证书可覆盖安装；跨包名或签名变化时，将可导出的数据复制到新版对应目录。
4. 手动导入核心；不同平台的核心二进制不可直接复制。
5. 用相同核心和版本验证存档可读后，再考虑清理旧版。

Android 数据目录随系统版本与授权方式变化，以界面显示的实际目录为准。桌面便携包升级时保留 `gamego.cfg`、`config/`、`saves/`、`states/`、`system/`、`playlists/`、`cores/` 等用户目录；不要直接覆盖这些数据。

即时存档不保证跨核心、跨版本兼容；长期保存应同时备份游戏内存档。最近游戏媒体可能引用原游戏路径，移动游戏后需要重新导入。
