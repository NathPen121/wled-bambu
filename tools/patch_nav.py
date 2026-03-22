#!/usr/bin/env python3
"""Injects a Bambu button into WLED's index.htm after the PC Mode button."""
import sys, os

base = os.path.dirname(os.path.abspath(__file__))
path = os.path.join(base, '..', 'wled-src', 'wled00', 'data', 'index.htm')
path = os.path.normpath(path)
if not os.path.exists(path):
    path = 'wled-src/wled00/data/index.htm'
if not os.path.exists(path):
    print(f'[patch_nav] ERROR: index.htm not found')
    sys.exit(1)

with open(path, 'r') as f:
    src = f.read()

if 'buttonBambu' in src:
    print('[patch_nav] Already injected, skipping.')
    sys.exit(0)

bambu_btn = '<button id="buttonBambu" onclick="window.location=\'/bambu\'"><i class="icons">&#xe023;</i><p class="tab-label">Bambu</p></button>'

# Insert after Config button (before PC Mode)
if 'buttonConfig' in src:
    idx = src.find('id="buttonConfig"')
    end = src.find('</button>', idx) + len('</button>')
    src = src[:end] + bambu_btn + src[end:]
    print('[patch_nav] Inserted after buttonConfig.')
elif 'buttonPcm' in src:
    idx = src.find('id="buttonPcm"')
    src = src[:idx] + bambu_btn + src[idx:]
    print('[patch_nav] Inserted before buttonPcm.')
else:
    # Fallback: insert before </body>
    src = src.replace('</body>', bambu_btn + '</body>', 1)
    print('[patch_nav] Fallback: inserted before </body>.')

with open(path, 'w') as f:
    f.write(src)
print('[patch_nav] Done.')
