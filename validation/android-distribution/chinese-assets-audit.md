# Chinese / Android Assets Audit

审计类型：AUDIT ONLY  
审计日期：2026-09-12  
仓库：`/Users/yujian/Documents/RetroArch-GameGo`  
分支：`handheld/p1-p2-baseline`  
基线 HEAD：`c6a0caeabd086bb112984e37104ebf456d02238a`  

## 1. Executive Summary

- 历史上确实修过中文链路：生成的 `msg_hash_chs.h` 使用八进制转义保留 UTF-8；`ae4d627` 修复 packed translation 与 `ids[]` 的错位；`0e275a2` 修复简中一条带 positional `%n$` 的格式化翻译。
- 当前分支保留了八进制 UTF-8 编码机制，也保留了 `ae4d627` 的 CHS 行配对修复；但当前分支没有合入 `0e275a2` 删除 `MSG_CHEEVOS_HARDCORE_PAUSED_SYSTEM_NOT_FOR_CORE` 的变更。
- Android native build 当前明确启用 `-DHAVE_LANGEXTRA`，`msg_hash.c` 在该宏下包含并注册 `intl/msg_hash_chs.h`。这部分不是当前 `????` 的主要缺口。
- 当前源码有完整的字体选择逻辑：简中/繁中选择 `chinese-fallback-font.ttf`，Ozone 使用 `regular.ttf` / `bold.ttf` 及 `assets/pkg` 下的语言 fallback 字体。
- 当前本地存在完整的 `pkg/android/phoenix/assets/`，但该目录被 `.gitignore` 整体忽略，目录内字体和 frontend assets 均未被 Git 跟踪。该本地状态足以让本机构建包含资源，不能代表 clean CI checkout。
- 当前 CI workflow 没有下载、校验、解压或准备 frontend assets，也没有验证 Ozone 字体/图标或 APK 内 assets。故 CI 可能构建源码与 native binary，但不携带运行时所需资源。
- Golden Run 34670457568 的期望 APK SHA256 `f8816820...` 不在当前仓库；本地 AArch64 APK `3ee2bea6...` 不是 Golden，且它明确包含本机忽略目录中的 assets。因此本报告不把该本地 APK 当作 CI Golden APK 证据。

最可能的根因链为：

```text
Translation Data       PASS（另有一条尚未同步的 CHS 格式行）
Compile / Encoding     PASS（当前宏、包含链和本地 binary 证据成立）
Font / Glyph           CI MISSING
Distribution Packaging CI FAIL / MISSING
```

当前真机 `????` 最可能来自 CI APK 没有打包 `chinese-fallback-font.ttf` / Ozone frontend assets，导致运行时无法加载包含中文 glyph 的字体；不是“重新修翻译字符串”的问题。

审计没有修改功能代码、翻译文件、Android.mk、Gradle、CI 或 Git 历史；只新增本报告。

## 2. Historical Chinese Fixes

可见 Git 历史中的相关提交如下。仓库是 shallow clone，历史边界停在 `cd8580f`；因此更早的引入时间不能从当前对象库证明。

| 项目 | 历史状态 | Commit | 当前仍存在 | 可复用 |
| -- | -- | -- | -- | -- |
| 生成 CHS packed header，并将非 ASCII 字节写成固定三位八进制转义 | `msg_hash_chs.h` 整体在当前 shallow 边界出现；文件头明确写明 Pure-ASCII source，转义后的 UTF-8 字节不依赖编译器执行字符集 | `cd8580f951fdfb1cb542289fc849b6a809b536f8` | 是 | 直接复用 |
| packed translation 行与 `ids[]` 按位置配对 | 删除 CHS 中多余的 `s_af6007a1` 行、对应翻译和 7 字节 blob 长度，避免后续字符串整体错位 | `ae4d627adb4e9b694ca8e589ccddb720b191bf3d` | 是；同一变更也由当前分支的 `3306a08` 带入 | 直接复用 |
| 格式化参数安全 | 简中翻译中的 `%2$s` / `%1$s` positional 转换与调用点不匹配；提交删除该 translation row、对应 struct member、blob 长度和 enum id | `0e275a27d859b0774adb7ae32b1d9fbbd6536e24` | 否；当前 HEAD 仍保留该 row，当前文件比 upstream 多 1 个 row | 部分复用；应先同步该既有修复 |
| 当前分支 CHS 改动 | `3306a08` 的 diff 实际删除了 `s_af6007a1` 及 `"坍塌"` 行并调整长度；提交说明本身是 handheld baseline，不是编码修复 | `3306a081276981e19704ea4fe4e32abae7cc276f` | 是 | 仅作为已存在的配对修复保留 |
| Android `HAVE_LANGEXTRA` | 当前 Android.mk 已有 `-DHAVE_LANGEXTRA`；可见历史中没有一个单独引入该宏的提交 | shallow 边界内未找到；当前行归属于 `cd8580f` 可见根提交 | 是 | 直接复用；不要重做 |
| 字体文件加入 Android frontend 资源 | 未找到把 `pkg/android/phoenix/assets` 或其 TTF/OTF 资源加入 Git 的提交 | NOT FOUND | 否 | 不存在可直接复用的 Git 资源提交 |

### `msg_hash_chs.h` 专项结果

1. **是否曾被修改：是。** 可见历史有 `cd8580f`、`ae4d627`、`0e275a2`，以及当前分支 `3306a08`。
2. **修改内容：**
   - 编码/生成方式：文件是生成文件，非 ASCII 内容以固定三位八进制转义保存；这是编码链修复。
   - packed table pairing：删除无对应 `ids[]` 的行，并更新 sizeof 检查；这是表结构/索引修复。
   - format safety：删除 positional `%n$` 且参数顺序不安全的简中行；这是翻译格式一致性修复。
3. **当前分支状态：** 编码机制与 `ae4d627` 修复仍在；`0e275a2` 的格式安全修复不在当前 HEAD。
4. **与 upstream 的差异：** 当前文件与 `upstream/master` 不同，当前分支多出：
   - `s_853088a2[73]`；
   - 对应含 `%2$s` / `%1$s` 的两段八进制字符串；
   - `MSG_CHEEVOS_HARDCORE_PAUSED_SYSTEM_NOT_FOR_CORE` id；
   - blob 期望长度 `128247u`，upstream 为 `128174u`。
5. **数据是否按 UTF-8 正确进入编译：** 源文件本身是 ASCII 文本，八进制转义产生 UTF-8 字节；`msg_hash.c` 在 `HAVE_LANGEXTRA` 下包含并注册 CHS 表。当前本地 AArch64 APK 的 native library 中可见中文 UTF-8 字符串（例如“设置”），支持本地编译链成立。该本地 APK 不是 Golden CI APK，因此不能替代对 CI artifact 的直接验证。

### 历史范围限制

`git rev-parse --is-shallow-repository` 返回 `true`，`.git/shallow` 为 `cd8580f...`。该 commit 的 parent object 不在本地，因此：

- `git log --all -S'HAVE_LANGEXTRA'`、相关 `-G` 查询在可见历史中没有引入提交；
- 不能据此断言 `HAVE_LANGEXTRA` 在整个 RetroArch 历史上首次何时加入；
- 能够断言的是：它在当前可见 upstream 基线以前已经存在，当前 handheld 分支没有以中文修复的方式新增或移除它。

## 3. Current Chinese Pipeline

```text
intl/msg_hash_chs.h
    ↓  generated packed C header; octal escapes encode UTF-8 bytes
msg_hash.c
    ↓  #ifdef HAVE_LANGEXTRA + include + msg_hash_strtab_chs registration
Android.mk
    ↓  -DHAVE_LANGEXTRA
libretroarch-activity.so
    ↓  language selection resolves chinese-fallback-font.ttf
pkg/android/phoenix/assets/assets/pkg/
    ↓  Gradle sourceSets.main.assets = ['assets']
APK assets / runtime asset path
```

| 层次 | 状态 | 证据 |
| -- | -- | -- |
| Translation Data | **PASS**（但有一条旧格式行未同步） | `intl/msg_hash_chs.h` 存在；当前文件包含 `msg_hash_chs_blob` 与 `msg_hash_chs_ids`；`0e275a2` 的单行修复尚未带入，但不是全量中文缺失。 |
| Compile / Encoding | **PASS** | `pkg/android/phoenix-common/jni/Android.mk:111` 定义 `-DHAVE_LANGEXTRA`；`msg_hash.c:251-285, 413-415, 526-533, 776-779` 完成包含、注册、查询和索引构建；CHS header 为纯 ASCII 八进制 UTF-8 表。 |
| Font / Glyph | **MISSING（CI）**；本地残留资源 **PASS** | `gfx/font_driver.c:405-418` 将简中/繁中映射到 `chinese-fallback-font.ttf`；本地有该文件，但 clean CI checkout 不会有被忽略的 Android assets 目录。 |
| Android Distribution Packaging | **FAIL / MISSING（CI）** | Gradle 只配置 `assets.srcDirs = ['assets']`；当前 workflow 没有准备该目录或验证 APK 内容。 |

当前 Android native build 的 source → generated header → compile define → binary 路径是成立的；断点在字体与 frontend distribution resource 是否随 APK 分发并能在首次启动的运行时路径下被找到。

## 4. Historical Font / Assets Evidence

### A. 是否真正加入过字体文件

在可见 Git 历史中没有发现 Android frontend 字体文件的加入记录。`git log --all -- '*.ttf' '*.otf'` 只命中与 Android frontend 无关的 Apple WebServer Glyphicons 文件；没有发现 `pkg/android/phoenix/assets` 下的 TTF/OTF 被 commit。

当前源码确实依赖字体文件：

- `gfx/font_driver.c`：中文选择 `chinese-fallback-font.ttf`；
- `menu/drivers/ozone.c`：默认加载 `ozone/bold.ttf`、`ozone/regular.ttf`，中文时从 `pkg` 目录选择语言 fallback；
- Ozone 图标路径由 `APPLICATION_SPECIAL_DIRECTORY_ASSETS_OZONE_ICONS` 解析到 assets 下的 Ozone icon tree。

这证明“字体选择逻辑存在”，不证明“字体资源进入 Git 或 CI APK”。

### B. 当前本地残留资源

当前本地 `pkg/android/phoenix/assets/`：

- 文件数：约 17,333（含 `.DS_Store`）；
- 大小：约 266 MB；
- TTF：17 个；
- 关键文件：`assets/pkg/chinese-fallback-font.ttf`、`assets/ozone/regular.ttf`、`assets/ozone/bold.ttf`；
- 目录修改时间：2026-09-11 11:17:47；关键字体时间：2026-09-10 18:06:26。

这是本机环境证据：本地曾经/目前具备完整 frontend assets，因而本机构建可能正常显示中文。没有找到更早工作区备份或 patch 可以把它证明为某个历史 commit 的内容；“历史成功来自本地 assets”是高度吻合当前证据的解释，但历史事件本身标记为 **NOT PROVEN**。

### C. 是否被 `.gitignore` 忽略

`.gitignore:150-159` 的 Android 规则包括：

```text
/pkg/android/phoenix/assets/
```

`git check-ignore -v` 明确命中该规则。`git ls-files` 不包含本地中文字体或 Ozone 字体，`git status --short --ignored` 显示整个目录为 ignored。没有发现通用 `*.ttf` / `*.otf` 规则；但目录级规则已经足以使全部 Android frontend resources 不进入 Git。

结论：之前本地成功运行时存在的资源完全可能从未进入仓库；当前工作区证据支持这个判断。

### D. assets.zip / glui_minimal_assets.zip

- 未找到 `glui_minimal_assets.zip`；
- Git 中有 Apple 平台 `pkg/apple/assets.zip` / `pkg/apple/OSX/assets.zip` 的构建引用，但这不是 Android `pkg/android/phoenix/assets` 的来源证据；
- 未找到 Android CI 下载或解压这些包的逻辑。

## 5. Current APK Evidence

### Golden artifact 定位

用户给出的 Golden P1-B1 artifact：

```text
Run: c6a0cae... / 34670457568
Expected SHA256: f8816820c68595937dd89e7839875d73bfc39e9e07b3d2ed9deb77fd3bb1febe
```

在当前仓库及其本地构建目录中没有找到 `RetroArch-runtime-enabled.apk`，也没有找到该期望 SHA256。因此无法直接对 Golden CI APK 执行 `unzip -l`，该项直接证据为 **NOT FOUND LOCALLY**。

### 本地可见 APK

本地存在多个 Gradle APK，先核对哈希后，唯一与当前 CI AArch64 variant 对应的本地文件为：

```text
pkg/android/phoenix/build/outputs/apk/aarch64/debug/phoenix-aarch64-debug.apk
SHA256: 3ee2bea6de68f5ce6c59e1b24cdc64f444e9352f723670a6ff86abe23f25b81d
Expected: f8816820c68595937dd89e7839875d73bfc39e9e07b3d2ed9deb77fd3bb1febe
```

该 APK **不是 Golden CI APK**，但它提供了本地忽略 assets 被 Gradle 打包时的结构对照：

| 项目 | 本地 APK 证据 |
| -- | -- |
| APK `assets/` | 17,325 条 |
| Ozone | `assets/assets/ozone/` 存在，849 条；含 `regular.ttf`、`bold.ttf` |
| Ozone icons | `assets/assets/ozone/png/icons/` 存在 |
| 中文字体 | `assets/assets/pkg/chinese-fallback-font.ttf` 存在 |
| 全部 TTF/OTF | 17 个 TTF，0 个 OTF |
| Autoconfig | 1,099 条 |
| Core info | `assets/info/` 322 条 |
| Overlays | 2,226 条 |
| Shaders | 6,681 条 |
| Cheats | 未发现 `assets/cheats/` |
| Database | 未发现 `assets/database/` |

这组结果说明 Gradle 配置本身可以把资源打入 APK；资源是否存在是另一个问题。若 clean CI checkout 没有 ignored assets 目录，Gradle 配置正确也不会自动生成这些文件。

## 6. CI Packaging Gap

检查 `.github/workflows/Handheld-Runtime-P1-B.yml`：

| CI 步骤 | 当前状态 |
| -- | -- |
| Download frontend assets | **缺失** |
| Verify resource SHA256 | **缺失**；只有 APK/manifest runtime evidence，没有 frontend resource hash |
| Extract frontend assets | **缺失** |
| Prepare `pkg/android/phoenix/assets` | **缺失** |
| Verify Ozone fonts | **缺失** |
| Verify Ozone icons | **缺失** |
| Inspect final APK assets | **缺失**；现有 `find` 只是列出 APK 输出位置，不检查 ZIP 条目 |
| Build source/native APK | **存在**；执行 `./gradlew clean assembleAarch64Debug` |
| Collect APK and SHA256 | **存在**；收集的是最终 APK 与 runtime link evidence |

因此当前 CI 的实际性质是：

```text
CI currently builds source code without preparing complete frontend distribution assets.
```

`.github/workflows/Handheld-Runtime-P1-B.yml:38-117` 没有任何 frontend asset preparation 或 APK asset-content assertion。`pkg/android/phoenix/build.gradle:146` 只声明 `assets.srcDirs = ['assets']`，并不负责下载资源。

## 7. Reusable vs Missing

### REUSE

- `intl/msg_hash_chs.h` 的 generated packed-table 方案；
- 纯 ASCII + 八进制转义的 UTF-8 表示；
- `HAVE_LANGEXTRA` Android 编译定义；
- `msg_hash.c` 的 CHS include、strtab registration、lookup 和 index build；
- `gfx/font_driver.c` 的 `chinese-fallback-font.ttf` 选择逻辑；
- Ozone 当前的 `pkg` / `regular.ttf` / `bold.ttf` / icon 路径逻辑。

### PARTIAL REUSE

- 当前分支的 CHS packed-row 对齐修复可保留；
- `0e275a2` 的格式安全修复应作为既有历史方案同步回来，但这属于已有翻译表一致性修复，不是本次 Distribution Packaging 主问题；
- 本地现有 assets 的目录结构可作为资源布局参考，但当前本地文件本身不是 Git 资产，也不能直接视为 CI 输入。

### MISSING

- CI 可重复取得的 frontend assets 来源；
- 资源版本/归档的 SHA256 校验；
- CI 中把资源放入 `pkg/android/phoenix/assets` 的步骤；
- Ozone `regular.ttf`、`bold.ttf`、Ozone icons 和 `pkg/chinese-fallback-font.ttf` 的最终 APK 断言；
- Golden CI APK 的本地可验证副本。

### DO NOT TOUCH

- 不要因为当前 `????` 重写 `msg_hash_chs.h`；
- 不要移除或重做 `HAVE_LANGEXTRA`；
- 不要重设计 font fallback 或 Ozone 路径；
- 不要把本地 ignored assets 当作已经进入 Git 的修复。

## 8. Root Cause

基于现有证据，根因判断为：

```text
CHS translation data exists
  → HAVE_LANGEXTRA is defined for Android
  → msg_hash_chs is included and registered
  → generated header preserves UTF-8 bytes
  → runtime asks for chinese-fallback-font.ttf
  → clean CI checkout does not prepare ignored pkg/android/phoenix/assets
  → Gradle assets directory is empty/missing for frontend distribution
  → APK lacks Chinese glyph font and likely Ozone frontend assets
  → Chinese strings cannot be rendered and appear as ???????
```

当前 `0e275a2` 未合入是一个真实的历史翻译格式缺口，但它只影响特定带参数消息；它不能解释“所有中文大量显示为 `???????`”。全量显示问号与 Font/Assets/Distribution Packaging 缺失更吻合。

由于 Golden APK 不在本机，本报告对“该指定 CI APK 的 ZIP 内容”采用：用户提供的外部检查结果 + workflow/source 证据；本地 APK 仅用于证明资源目录存在时 Gradle 的打包形态。

## 9. Minimal Next Fix

只提出建议，不实施：

1. 复用当前源码、中文表和字体路径逻辑，不重做中文系统。
2. 为 Android CI 增加固定版本的 frontend asset acquisition/preparation，放入 Gradle 已声明的 `pkg/android/phoenix/assets` 路径。
3. 对资源归档或资源目录执行 SHA256 校验，并明确校验 `assets/pkg/chinese-fallback-font.ttf`、`assets/ozone/regular.ttf`、`assets/ozone/bold.ttf` 与 Ozone icons。
4. 构建后直接检查最终 APK ZIP 条目，再保存 APK SHA256 与资源检查结果。
5. 人工审核后，另行决定是否同步 `0e275a2` 的既有 CHS 格式修复；不要把它与本次 distribution packaging 修复混为一谈。

## Audit Conclusion

```text
Chinese / Assets Audit = PASS
```

这里的 PASS 表示审计已把历史修复、当前保留状态、ignored local assets、CI packaging gap 和最可能根因厘清；不表示当前 Android 中文显示已经修复。
