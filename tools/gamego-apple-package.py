#!/usr/bin/env python3
"""Package an Apple device or macOS build without claiming distribution signing."""

import argparse
import json
from pathlib import Path
import plistlib
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--products", type=Path, required=True)
    parser.add_argument("--resources", type=Path, required=True)
    parser.add_argument("--platform", choices=("macos-arm64", "macos-x64", "ios-arm64"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    version = (root / "GAMEGO_VERSION").read_text().strip()
    ios = args.platform.startswith("ios")
    products = args.products / ("Release-iphoneos" if ios else "Release")
    apps = list(products.glob("*.app"))
    if len(apps) != 1:
        raise SystemExit("Expected exactly one application in " + str(products))
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="gamego-apple-") as directory:
        stage = Path(directory)
        parent = stage / "Payload" if ios else stage
        parent.mkdir(exist_ok=True)
        app = parent / "RetroArch.app"
        shutil.copytree(apps[0], app, symlinks=True)
        resources = app if ios else app / "Contents/Resources"
        shutil.copy2(args.resources / "assets.zip", resources / "assets.zip")
        for name in ("COPYING", "THIRD_PARTY_NOTICES.md", "BUILD.json", "resources.lock"):
            shutil.copy2(args.resources / name, resources / name)
        shutil.copytree(args.resources / "docs", resources / "RetroArch-docs")
        info = app / "Info.plist" if ios else app / "Contents/Info.plist"
        with info.open("rb") as stream:
            plist = plistlib.load(stream)
        plist["CFBundleDisplayName"] = "RetroArch"
        plist["CFBundleName"] = "RetroArch"
        plist["CFBundleIdentifier"] = "com.cutoolee.gamego"
        plist["CFBundleShortVersionString"] = version.split("-")[0]
        with info.open("wb") as stream:
            plistlib.dump(plist, stream)
        for signature in app.rglob("_CodeSignature"):
            shutil.rmtree(signature)
        for profile in app.rglob("embedded.mobileprovision"):
            profile.unlink()
        status = "unsigned; requires user signing and separately supplied compatible cores"
        if not ios:
            subprocess.run(["codesign", "--force", "--deep", "--sign", "-", str(app)], check=True)
            subprocess.run(["codesign", "--verify", "--deep", "--strict", str(app)], check=True)
            status = "ad-hoc signature; no Developer ID or notarization"
        name = "RetroArch-" + version + "-" + args.platform + ("-unsigned.ipa" if ios else "-adhoc.zip")
        subprocess.run(["ditto", "-c", "-k", "--sequesterRsrc", "--keepParent",
                        str(parent if ios else app), str(args.output / name)], check=True)
        (args.output / (args.platform + "-signing.json")).write_text(json.dumps({
            "platform": args.platform, "signing": status,
            "runtime_acceptance": "not-verified", "bundled_cores": [],
        }, indent=2) + "\n")


if __name__ == "__main__":
    main()
