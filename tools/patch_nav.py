#!/usr/bin/env python3
"""Injects a Bambu button into WLED's index.htm after the Config button."""
import sys, os

base = os.path.dirname(os.path.abspath(__file__))
path = os.path.join(base, '..', 'wled-src', 'wled00', 'data', 'index.htm')
path = os.path.normpath(path)
if not os.path.exists(path):
    path = 'wled-src/wled00/data/index.htm'
if not os.path.exists(path):
    print('[patch_nav] ERROR: index.htm not found')
    sys.exit(1)

with open(path, 'r') as f:
    src = f.read()

if 'buttonBambu' in src:
    print('[patch_nav] Already injected, skipping.')
    sys.exit(0)

bambu_btn = '<button id="buttonBambu" onclick="window.location=\'/bambu\'"><i class="icons">&#xe023;</i><p class="tab-label">Bambu</p></button>'

inserted = False
for marker in ['id="buttonConfig"', 'id="buttonPcm"']:
    idx = src.find(marker)
    if idx < 0:
        continue
    end = src.find('</button>', idx)
    if end < 0:
        continue
    end += len('</button>')
    if marker == 'id="buttonConfig"':
        # insert AFTER Config's closing </button>
        src = src[:end] + bambu_btn + src[end:]
    else:
        # insert BEFORE buttonPcm's opening <button tag
        btn_start = src.rfind('<button', 0, idx)
        src = src[:btn_start] + bambu_btn + src[btn_start:]
    inserted = True
    print(f'[patch_nav] Inserted using marker: {marker}')
    break

if not inserted:
    # last resort - insert before </body>
    src = src.replace('</body>', bambu_btn + '</body>', 1)
    print('[patch_nav] Fallback: inserted before </body>')

with open(path, 'w') as f:
    f.write(src)
print('[patch_nav] Done.')
