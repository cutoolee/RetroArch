# Third-party notices

This community fork is derived from [RetroArch](https://github.com/libretro/RetroArch). Copyright notices and license headers in the source tree remain authoritative. This fork does not claim ownership of RetroArch, libretro, emulator cores, or third-party artwork.

| Component | Source / attribution | License / notice |
| --- | --- | --- |
| RetroArch and modifications in this fork | RetroArch contributors and contributors to this fork | GNU GPLv3; see `COPYING` and per-file headers |
| libretro-common and vendored dependencies | Original authors identified in each directory | Per-file and directory licenses; preserved in the complete source archive |
| Frontend assets | [libretro/retroarch-assets](https://github.com/libretro/retroarch-assets), revision and SHA256 in `handheld/distribution/resources.lock` | Root `COPYING`: CC BY 4.0; retain additional font/artwork licenses supplied by upstream |
| Lucide menu icons, including monitor/history | Lucide Contributors; portions by Cole Bemis (Feather), lucide-static 0.468.0 | ISC; `handheld/ui/icons/LICENSE`, also installed with PNG assets |
| zfast CRT shader variants | Greg Hogan (SoltanGris42), Jose Linares (Dogway) | GPLv2 or later; complete headers retained in shader files |
| SameBoy LCD shader variants | Lior Halphon (LIJI32) | MIT; complete notice retained in shader files |
| RetroArch logos and other historical media | Original authors; see `media/icons/LICENSE/` | Some media has CC BY-NC terms; do not assume code licensing covers logos or trademarks |

No commercial game ROMs or BIOS images are included in this fork’s binary releases. Third-party emulator cores are not bundled by the initial release pipeline. Users obtain cores separately, subject to each core's license and platform signing requirements.

The complete source archive preserves dependency notices and the platform build projects. The asset lock identifies the exact externally downloaded frontend archive; packaging preserves its license files. Windows runtime DLL licenses and build/package versions must accompany Windows packages when such DLLs are redistributed.

Fork-specific menu shapes and presets are modifications made in this fork. Generated controller artwork is kept as a project asset; the Lucide ISC notice applies to Lucide icons, not automatically to unrelated artwork.
