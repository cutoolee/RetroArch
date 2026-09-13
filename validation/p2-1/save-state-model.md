# GameGo Save State Metadata Model

状态：设计稿（P2.1，non-blocking）
范围：产品数据模型与兼容策略；不改变 Runtime、Bridge、UI 或
`task_save` 实现。

## 1. 目标与不变量

- 手动存档提供固定的 10 个 slot：`0` 到 `9`。
- state binary 是唯一的主存档。metadata 和截图是可重建的展示数据，不能
  把它们的失败升级为 state 保存失败。
- slot 的身份由 `gameId` 和 `slot` 决定，不由 ROM 文件名或绝对路径决定。
- 保存的“成功”只表示 state 文件已完整写入并可读；请求进入队列只表示
  `ACCEPTED`，不能提前显示成功。
- 覆盖必须是可恢复的事务：新 state 未确认落盘前，旧 state 不得被破坏。

## 2. 记录模型

每个 slot 有一个 metadata sidecar。字段如下：

| 字段 | 类型 | 约定 |
| --- | --- | --- |
| `stateVersion` | integer | metadata schema 版本，从 `1` 开始；不是 core 的 state binary 版本。读取器按版本迁移，不能把未知的高版本当作当前版本。 |
| `gameId` | string | 稳定的、不透明的内容身份。应由不含绝对路径的 canonical content identity 生成（例如内容摘要或等价的 libretro 内容身份）；同一 ROM 改名/移动后不变，内容本身改变时应改变。 |
| `core` | string | 稳定的 core 标识（core id），不使用展示名称。 |
| `coreVersion` | string | 创建 state 时的精确 core 版本；未知时为 `null`，不得猜测。 |
| `slot` | integer | 当前为 `0..9`。不使用 `-1` 或大于 `9` 表示 GameGo 手动 slot。 |
| `createdAt` | string | state 成功落盘时写入的 UTC RFC 3339 时间；metadata 失败时不以截图时间替代。 |
| `stateFile` | string | 相对于 save-state root 的路径，不能写入绝对路径；指向实际 state binary。 |
| `screenshot` | string/null | 相对于 screenshot root 的 PNG 路径；不可用或失败时为 `null`。 |
| `contentHash` | string | stateFile 落盘后对其实际字节计算的摘要（建议 `sha256:<hex>`）；用于发现损坏或 metadata 与 state 不匹配。 |
| `compatibility` | object | 当前环境下的派生检查结果，见下节；不是对 core binary 可恢复性的绝对保证。 |

建议的 sidecar 内容（展示完整字段，不要求实现阶段照抄序列化格式）：

```json
{
  "stateVersion": 1,
  "gameId": "sha256:GAME_ID",
  "core": "mgba",
  "coreVersion": "0.10.3",
  "slot": 0,
  "createdAt": "2026-09-12T08:30:15Z",
  "stateFile": "sha256-GAME_ID/slot-00.state",
  "screenshot": "sha256-GAME_ID/slot-00.png",
  "contentHash": "sha256:STATE_BYTES",
  "compatibility": {
    "status": "compatible",
    "reason": null,
    "checkedAt": "2026-09-12T08:30:16Z"
  }
}
```

`compatibility` 至少使用这些状态：

- `compatible`：`gameId`、`core`、metadata schema 均可接受，且 core 版本
  相同；state 文件存在、摘要匹配。
- `warning`：同一 `gameId` 和 `core`，但 `coreVersion` 不同或未知。允许
  用户明确确认后尝试 Load；失败时按普通 Load error 处理，不删除 state。
- `incompatible`：`gameId` 或 `core` 不匹配，或 metadata schema 高于当前
  读取器。默认禁用 Load，避免把其他游戏/核心的 state 静默载入。
- `legacy_unknown`：没有可验证的 metadata 身份（旧 RA state）。只能在
  当前内容与当前 core 上显示为未知兼容性，并要求确认后尝试。
- `missing` / `corrupt`：stateFile 不存在、摘要不匹配或读取失败；slot
  显示错误，不把 sidecar 当作可加载存档。

兼容性可在读取时重新计算；`checkedAt` 和缓存的 `status` 不是事实来源。
metadata 不能替代 core 的 `unserialize` 结果，最终是否能恢复仍由 core
决定。

## 3. 文件组织与查找

推荐使用按 `gameId` 隔离的相对路径：

```text
<save-state-root>/<gameId>/slot-00.state
<save-state-root>/<gameId>/slot-00.json
<screenshot-root>/<gameId>/slot-00.png
```

实际目录名可以采用安全编码后的 `gameId`，但必须保持可逆或可通过
metadata 查回。`stateFile` 和 `screenshot` 均只允许 root 内的相对路径，避免
ROM 文件名中的路径分隔符、重命名和路径注入影响 slot 定位。

slot 列表按 `0..9` 固定顺序返回。每个 slot 可独立处于 empty、ready、warning、
incompatible、missing 或 corrupt 状态；一个坏 slot 不应阻塞其它 slot 的显示
或加载。

## 4. 保存、截图与 metadata 的顺序

一次手动 Save 的产品事务为：

1. 解析当前 `gameId`、`core`、`coreVersion` 和目标 `slot`，生成新的临时
   state 路径。
2. 序列化并完整写入临时 state；成功后计算 `contentHash`，并以原子替换
   将 state 提升为该 slot 的主文件。
3. 在 state 成功后、成功提示前尽快捕获截图；应尽量抑制保存提示/OSD，且
   无有效 framebuffer 时允许跳过。
4. 生成 metadata 临时文件并原子替换 sidecar。`createdAt` 对应 state 落盘
   成功时间，而不是 metadata 或截图完成时间。
5. slot 展示刷新；截图和 metadata 的后续失败以 warning 表达，不撤销已成功
   的 state。

截图不是 state 的组成部分。截图失败时保留 state，写入
`"screenshot": null`（以及可选的短期诊断状态），slot 使用无预览占位并允许
后续重试。截图文件写成功但 metadata 写失败时，截图是可清理的孤儿派生物，
不得阻止 Load。

metadata 写失败时，state 仍然是成功；下次扫描应通过 stateFile、文件存在性
和 `contentHash` 检测 sidecar 缺失/过期，并尝试重建最小 metadata。若旧
sidecar 的 hash 与新 state 不同，应标记 metadata stale，而不是展示旧时间或
旧截图为新 state 的信息。metadata 失败不应覆盖旧 sidecar，除非新 sidecar
已经完整写好并可原子替换。

若 state 本身写入、序列化、摘要计算或原子替换失败：清理临时文件，不更新
`createdAt`、`contentHash` 或旧截图，slot 保持旧内容；若没有旧内容则保持
empty/error。只有 state 主文件确认成功后才可发出 completed-success。

## 5. Slot 覆盖策略

- 覆盖非空 slot 前必须有明确的用户操作/确认；快速连续操作在同一 slot 上
  应被标记 busy，而不是排队造成最后一次意外覆盖。
- 新 state 使用临时文件和原子 replace。新写入失败时旧 state、旧 metadata
  和旧截图继续有效。
- 新 state 成功后再替换 metadata；旧 metadata 不可被截断成空文件。
- 新截图只有在新 state 成功后才提升为 slot 的截图。截图失败不删除旧截图
  的展示能力，除非产品明确选择显示无预览；推荐保留旧截图但标记为 stale，
  以免把旧画面误认成新 state。
- 覆盖后 `createdAt`、`contentHash`、`stateFile` 和截图引用必须属于同一
  次保存；扫描时 hash 不匹配即显示 stale/corrupt，不静默信任 metadata。

## 6. 旧 metadata 与旧 state 的兼容

`stateVersion` 是 sidecar schema 版本：

- 当前版本读取器必须支持从已发布的旧版本迁移；新增字段采用默认值，未知
  字段可保留或忽略，但不能改变已存在字段的含义。
- 高于当前版本的 metadata 不做破坏性迁移，slot 标为
  `incompatible`/unknown；若 state 路径仍明确可定位，可允许用户在确认后
  选择 legacy load，但不能覆盖原 metadata。
- 没有 sidecar 的旧 RetroArch state 走 legacy import：从当前 RA state 目录
  和现有命名规则发现文件，slot 由可可靠推断的数字得到；不能从文件名
  猜造 `gameId`。缺少 `createdAt` 时可显示文件 mtime，但应标记时间来源为
  `filesystem`，不伪装成精确创建时间。
- 旧 state 首次成功 Load 后可以补写 sidecar；补写失败不影响这次 Load，且
  不得移动或覆盖原 state。
- 旧 metadata 的 `coreVersion` 缺失视为 unknown，不能当作当前版本完全兼容。

## 7. ROM 改名、移动与内容变更

- ROM 改名或移动：重新打开内容后计算同一 `gameId`，从 save-state root 的
  `<gameId>` 目录找到原有 10 个 slot；只更新可选的展示名称/最近路径，不能
  把路径写进身份或迁移 state。
- ROM 内容改变但文件名不变：`gameId` 应改变，旧 slot 不得自动显示为新
  内容的可加载 slot。用户可通过 legacy/import 流程明确选择，但不能静默
  复用。
- 多文件内容（例如盘片集合）应对 canonical 的文件集合/顺序/身份计算
  `gameId`，不使用某一个绝对路径；集合变更按新内容处理。
- 找不到原 save root 时，只显示当前配置 root 下的结果；不在任意路径做
  无界扫描。更换存储 root 属于迁移操作，应保持相同相对路径和 `gameId`。

## 8. 与未来 auto-save / quick-save 的扩展

当前产品只承诺 `kind = manual` 的 10 个 slot，但 schema 应预留独立命名空间，
不要把自动存档塞入 `0..9`：

```text
manual/<gameId>/slot-00.state ... slot-09.state
quick/<gameId>/quick.state
auto/<gameId>/auto.state 或 auto-<sequence>.state
```

未来 metadata 可增加 `kind`（`manual`、`quick`、`auto`）、`sequence`、
`trigger` 和 `retention`。其中：

- quick-save 是单独的、可快速替换的用户快捷位，不占用手动 slot；
- auto-save 可采用单文件或有限 ring，按 `sequence`/`createdAt` 选择最新，
  不参与手动 slot 覆盖确认；
- 三种 kind 仍复用 `gameId`、`core`、`coreVersion`、`contentHash` 和兼容性
  检查；Load 前仍须验证 stateFile 与 metadata 的 hash。

这样可在不改变现有 10-slot UI 语义的情况下增加 quick-save/auto-save，也能
让清理策略按 namespace 独立执行。

## 9. P2.1 边界

本文件只定义未来产品模型。P2.1 的 Runtime/Bridge 只需继续保证
`ACCEPTED` 与真实 state task `COMPLETED` 的区分；metadata、截图失败的 warning
展示和迁移逻辑属于后续实现工作，不阻塞本阶段完成。
