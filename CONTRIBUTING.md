# Contributing to this RetroArch fork

欢迎提交问题和 Pull Request。请先说明平台、本分支版本、游戏系统和所用核心，区分前端问题与核心问题。

## 开发

1. 阅读 `AGENTS.md`，保持现有架构和 C89 风格。
2. 从当前默认分支创建主题分支，保持变更聚焦。
3. 按 `docs/release/build.md` 安装依赖并构建目标平台。
4. 执行与改动相关的测试。快捷菜单至少执行 `bash validation/p2/tests/test_quick_menu.sh` 和 `sh validation/p2.1/tests/test_bridge.sh`。
5. PR 说明问题、最终行为、验证方式和未验证的平台。

不得提交签名密钥、账号令牌、游戏 ROM、BIOS、用户存档、完整构建目录或设备私人数据。截图和测试内容需有相应使用权限。

报告问题时请先移除日志中的用户名、私人路径、令牌和设备标识。请勿向上游提交未经上游版本复现的 本分支专有问题。
