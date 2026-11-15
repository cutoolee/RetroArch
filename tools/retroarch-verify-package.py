#!/usr/bin/env python3
"""Verify the application names inside actual release archives."""

import json
from pathlib import Path
import plistlib
import sys
import tarfile
import zipfile


def require(condition, message):
    if not condition:
        raise SystemExit(message)


def verify(path):
    require(path.name.startswith("RetroArch-"), "Incorrect download name: " + path.name)
    if path.name.endswith(".tar.gz"):
        with tarfile.open(path, "r:gz") as archive:
            names = archive.getnames()
            if path.name.endswith("-source.tar.gz"):
                prefix = path.name.removesuffix("-source.tar.gz") + "/"
                require(all(name == prefix[:-1] or name.startswith(prefix) for name in names),
                        "Incorrect source archive directory")
                return
            prefix = path.name.removesuffix(".tar.gz") + "/"
            require(prefix + "RetroArch" in names, "Missing RetroArch portable launcher")
            build = json.load(archive.extractfile(prefix + "BUILD.json"))
    else:
        with zipfile.ZipFile(path) as archive:
            names = archive.namelist()
            if "-windows-" in path.name:
                prefix = path.stem + "/"
                require(prefix + "RetroArch.cmd" in names, "Missing RetroArch.cmd")
                build = json.loads(archive.read(prefix + "BUILD.json"))
            else:
                ios = path.suffix == ".ipa"
                app = "Payload/RetroArch.app/" if ios else "RetroArch.app/"
                resources = app if ios else app + "Contents/Resources/"
                info = app + ("Info.plist" if ios else "Contents/Info.plist")
                require(info in names, "Missing RetroArch.app Info.plist")
                plist = plistlib.loads(archive.read(info))
                for key in ("CFBundleDisplayName", "CFBundleName"):
                    require(plist.get(key) == "RetroArch", "Incorrect " + key)
                require(not any(name.startswith(("GameGo.app/", "Payload/GameGo.app/")) for name in names),
                        "Unexpected GameGo.app in archive")
                build = json.loads(archive.read(resources + "BUILD.json"))
    require(build.get("product") == "RetroArch", "Incorrect BUILD.json product")


if __name__ == "__main__":
    require(len(sys.argv) > 1, "Provide at least one release archive")
    for filename in sys.argv[1:]:
        verify(Path(filename))
        print("PASS: RetroArch application naming in " + Path(filename).name)
