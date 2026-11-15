#!/usr/bin/env python3
"""Build GameGo release inventories and portable distributions."""

import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DEFAULTS = ROOT / "pkg/android/phoenix/gamego-assets/gamego-defaults"


def git(*args):
    return subprocess.check_output(["git", "-C", str(ROOT), *args])


def version():
    value = (ROOT / "GAMEGO_VERSION").read_text().strip()
    if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+(?:-[a-z0-9.]+)?", value):
        raise SystemExit("Invalid GAMEGO_VERSION")
    return value


def version_code():
    match = re.fullmatch(r"(\d+)\.(\d+)\.(\d+)(?:-(alpha|beta|rc)\.(\d+))?", version())
    if not match:
        raise SystemExit("Android versions support alpha.N, beta.N, rc.N or stable releases")
    major, minor, patch = (int(match[i]) for i in (1, 2, 3))
    if major > 2000 or minor > 99 or patch > 99:
        raise SystemExit("Version exceeds the Android version-code range")
    ordinal = 99
    if match[4]:
        number = int(match[5])
        if not 1 <= number <= 19:
            raise SystemExit("Prerelease number must be between 1 and 19")
        ordinal = {"alpha": 0, "beta": 20, "rc": 60}[match[4]] + number
    return major * 1000000 + minor * 10000 + patch * 100 + ordinal


def copy_tree(source, destination):
    shutil.copytree(source, destination, dirs_exist_ok=True)


def notices(destination):
    destination.mkdir(parents=True, exist_ok=True)
    for filename in ("COPYING", "THIRD_PARTY_NOTICES.md", "GAMEGO_VERSION"):
        shutil.copy2(ROOT / filename, destination / filename)
    copy_tree(ROOT / "docs/release", destination / "docs")
    shutil.copy2(ROOT / "handheld/distribution/resources.lock", destination)


def assets(source, destination):
    if not (source / "ozone/regular.ttf").is_file():
        raise SystemExit("Frontend assets missing ozone/regular.ttf")
    if not (source / "pkg/chinese-fallback-font.ttf").is_file():
        raise SystemExit("Frontend assets missing Chinese fallback font")
    copy_tree(source, destination / "assets")
    copy_tree(DEFAULTS / "icons", destination / "assets/gamego/icons")
    copy_tree(DEFAULTS / "shaders", destination / "shaders")
    copy_tree(DEFAULTS / "presets", destination / "preset-examples")


def metadata(destination, platform):
    data = {
        "product": "GameGo", "version": version(), "platform": platform,
        "commit": git("rev-parse", "HEAD").decode().strip(),
        "upstream_version": re.search(r'PACKAGE_VERSION "([^"]+)"', (ROOT / "version.all").read_text()).group(1),
        "frontend_assets": json.loads((ROOT / "handheld/distribution/resources.lock").read_text()),
        "bundled_cores": [], "runtime_acceptance": "not-verified-by-packaging",
    }
    (destination / "BUILD.json").write_text(json.dumps(data, indent=2) + "\n")


def stage(args):
    destination = args.output.resolve()
    destination.mkdir(parents=True, exist_ok=True)
    executable = "retroarch.exe" if args.platform == "windows-x64" else "retroarch"
    shutil.copy2(args.binary, destination / executable)
    assets(args.assets, destination)
    notices(destination)
    for directory in ("saves", "states", "system", "cores", "config", "screenshots", "playlists"):
        (destination / directory).mkdir(exist_ok=True)
    config = [
        'assets_directory = ":/assets"', 'video_shader_dir = ":/shaders"',
        'savefile_directory = ":/saves"', 'savestate_directory = ":/states"',
        'system_directory = ":/system"', 'libretro_directory = ":/cores"',
        'menu_config_directory = ":/config"', 'screenshot_directory = ":/screenshots"',
        'playlist_directory = ":/playlists"', 'menu_driver = "ozone"',
        'savestate_auto_save = "true"', 'savestate_thumbnail_enable = "true"',
    ]
    (destination / "gamego.cfg").write_text("\n".join(config) + "\n")
    if args.platform == "windows-x64":
        (destination / "GameGo.cmd").write_text(
            '@echo off\ncd /d "%~dp0"\n"%~dp0retroarch.exe" --config "%~dp0gamego.cfg" %*\n')
    else:
        launcher = destination / "GameGo"
        launcher.write_text('#!/bin/sh\nset -eu\nbase=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)\n'
                            'cd "$base"\nexport LD_LIBRARY_PATH="$base/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\n'
                            'exec "$base/retroarch" --config "$base/gamego.cfg" "$@"\n')
        launcher.chmod(0o755)
    metadata(destination, args.platform)


def apple_assets(args):
    assets(args.assets, args.output)
    notices(args.output)
    metadata(args.output, args.platform)
    with zipfile.ZipFile(args.output / "assets.zip", "w", zipfile.ZIP_DEFLATED) as archive:
        for category in ("assets", "shaders"):
            for path in sorted((args.output / category).rglob("*")):
                if path.is_file():
                    archive.write(path, path.relative_to(args.output))


def source(args):
    if git("status", "--porcelain", "--untracked-files=normal").strip():
        raise SystemExit("Commit release sources before creating the corresponding source archive")
    args.output.mkdir(parents=True, exist_ok=True)
    target = args.output / ("GameGo-" + version() + "-source.tar.gz")
    with gzip.open(target, "wb", compresslevel=6) as output:
        process = subprocess.Popen(["git", "-C", str(ROOT), "archive", "--format=tar",
                                    "--prefix=GameGo-" + version() + "/", "HEAD"], stdout=subprocess.PIPE)
        with process.stdout:
            shutil.copyfileobj(process.stdout, output)
        if process.wait():
            raise SystemExit("git archive failed")
    print(target)


def checksums(args):
    output = args.output.resolve()
    entries = []
    for path in sorted(output.iterdir()):
        if not path.is_file() or path.name == "SHA256SUMS":
            continue
        digest = hashlib.sha256()
        with path.open("rb") as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(block)
        entries.append(digest.hexdigest() + "  " + path.name)
    if not entries:
        raise SystemExit("No release files to checksum")
    (output / "SHA256SUMS").write_text("\n".join(entries) + "\n")


def audit(args):
    patterns = [
        ("private-key", re.compile(rb"-----BEGIN (?:RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----")),
        ("github-token", re.compile(rb"\b(?:gh[pousr]_[A-Za-z0-9]{36,}|github_pat_[A-Za-z0-9_]{50,})\b")),
        ("aws-access-key", re.compile(rb"\b(?:AKIA|ASIA)[A-Z0-9]{16}\b")),
    ]
    patterns.append(("device-identifier", re.compile(
        rb'(?:\\*")(?:deviceKey|imei|imsi|androidId|android_id|serialNumber)(?:\\*")\s*:\s*(?:\\*")(?!REDACTED)[^"\\\r\n]{6,}(?:\\*")', re.I)))
    forbidden = {".jks", ".keystore", ".p12", ".mobileprovision", ".gba", ".nes", ".apk", ".ipa"}
    findings = []
    public_keys = json.loads((ROOT / "handheld/distribution/public-test-keys.json").read_text())
    names = git("ls-files", "-z", "--cached", "--others", "--exclude-standard").split(b"\0")
    for name in filter(None, names):
        path = ROOT / os.fsdecode(name)
        if not path.is_file():
            continue
        relative = path.relative_to(ROOT).as_posix()
        if path.suffix.lower() in forbidden:
            findings.append({"path": relative, "rule": "private-or-runtime-artifact"})
        if path.stat().st_size > 50 * 1024 * 1024:
            findings.append({"path": relative, "rule": "file-over-50MiB"})
        if path.stat().st_size > 5 * 1024 * 1024:
            continue
        content = path.read_bytes()
        if b"\0" in content:
            continue
        for rule, pattern in patterns:
            match = pattern.search(content)
            if match and not (rule == "private-key" and
                              public_keys.get(relative) == hashlib.sha256(content).hexdigest()):
                findings.append({"path": relative, "rule": rule,
                                 "line": content[:match.start()].count(b"\n") + 1})
    report = {"scope": "tracked-and-unignored-working-tree", "findings": findings,
              "history": "requires separate gitleaks scan", "checked_paths": len(names) - 1}
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    return bool(findings)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("version")
    commands.add_parser("version-code")
    for name in ("stage", "apple-assets", "source", "checksums"):
        command = commands.add_parser(name)
        command.add_argument("--output", type=Path, required=True)
        if name in ("stage", "apple-assets"):
            command.add_argument("--assets", type=Path, required=True)
            command.add_argument("--platform", required=True)
        if name == "stage":
            command.add_argument("--binary", type=Path, required=True)
    command = commands.add_parser("audit")
    command.add_argument("--report", type=Path)
    args = parser.parse_args()
    if args.command == "version":
        print(version())
    elif args.command == "version-code":
        print(version_code())
    else:
        return globals()[args.command.replace("-", "_")](args)


if __name__ == "__main__":
    sys.exit(main())
