# P2.1 unattended Quick Menu regression

Run: 2026-09-12 Asia/Shanghai
Device: GR0006 / 2419TNW44608 / Android 11 / arm64-v8a
Package: com.cutoolee.gamego
Commit: d0337cc3
APK SHA256: 34722a4fe700344e27f2c6e1f8aab2dc01a53fb93d8d8c5fc784ee815f2bf484

Content launch succeeded through `RetroActivityFuture` with ROM and LIBRETRO
extras. mGBA loaded `/storage/emulated/0/Roms/GBA/龙珠大冒险.zip`; Quick Menu
was visible and the process remained alive.

The device's ADB-injected key events did not enter the GameGo gamepad input
pipeline: `KEYCODE_BUTTON_A=96` did not activate the selected Save action and
`KEYCODE_BUTTON_MODE=110` did not close the menu. No existing device test
harness was available. Per the run instructions, the dependent functional
cases were stopped rather than inferred.

AUTO_FUNCTIONAL_REGRESSION=PARTIAL
HUMAN_CONFIRMATION_REMAINING=CJK字体观感；Continue/Save/Load/Reset/Advanced/Exit及20轮压力回归仍需可用的真实input pipeline或现有test harness；Load视觉恢复

Human physical G/Menu evidence remains previously confirmed and is not reused
as synthetic-input evidence.
