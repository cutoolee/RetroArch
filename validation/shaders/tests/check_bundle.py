#!/usr/bin/env python3
import pathlib
import re
import subprocess
import sys
import tempfile

root = pathlib.Path(__file__).resolve().parents[3]
bundle = root / 'pkg/android/phoenix/gamego-assets/gamego-defaults/shaders/gamego'
seed = (root / 'frontend/drivers/platform_unix.c').read_text()

for preset in bundle.rglob('*p'):
    text = preset.read_text()
    for ref in re.findall(r'^#reference\s+"([^"]+)"', text, re.M):
        assert (preset.parent / ref).is_file(), (preset, ref)
    for source in re.findall(r'^shader\d+\s*=\s*"?([^"\n]+)', text, re.M):
        assert (preset.parent / source.strip()).is_file(), (preset, source)
    relative = 'shaders/gamego/' + str(preset.relative_to(bundle))
    assert relative in seed, relative

for source in bundle.rglob('*.slang'):
    assert 'shaders/gamego/' + str(source.relative_to(bundle)) in seed
    text = source.read_text()
    common, stages = text.split('#pragma stage vertex', 1)
    vertex, fragment = stages.split('#pragma stage fragment', 1)
    common = re.sub(r'^#pragma parameter.*$', '', common, flags=re.M)
    if len(sys.argv) < 2:
        continue
    with tempfile.TemporaryDirectory() as tmp:
        for stage, body in [('vert', vertex), ('frag', fragment)]:
            path = pathlib.Path(tmp) / ('shader.' + stage)
            path.write_text(common + body)
            subprocess.run([sys.argv[1], str(path), '-o', str(path) + '.spv'], check=True)
print('PASS: bundled preset references and Android seeding')
if len(sys.argv) > 1:
    print('PASS: Slang vertex/fragment compilation')
