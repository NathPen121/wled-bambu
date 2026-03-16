#!/usr/bin/env python3
"""Injects a Bambu tab into WLED's index.htm nav bar."""
import re, sys, os

# Works whether called from repo root or anywhere else
base = os.path.dirname(os.path.abspath(__file__))
path = os.path.join(base, '..', 'wled-src', 'wled00', 'data', 'index.htm')
path = os.path.normpath(path)

# In the workflow the CWD is the repo root, so also try relative path
if not os.path.exists(path):
    path = 'wled-src/wled00/data/index.htm'

if not os.path.exists(path):
    print(f'[patch_nav] ERROR: index.htm not found at {path}')
    sys.exit(1)

with open(path, 'r') as f:
    src = f.read()

if '/bambu' in src:
    print('[patch_nav] Already injected, skipping.')
    sys.exit(0)

tab = '<a class="bt" href="/bambu" target="_self">&#x1F5A8; Bambu</a>'
new_src = re.sub(
    r'(<a[^>]+href="/settings"[^>]*>[^<]+</a>)',
    r'\1' + tab,
    src,
    count=1
)

if new_src == src:
    print('[patch_nav] WARNING: settings anchor not found, trying buttonS fallback')
    new_src = src.replace('id="buttonS"', 'id="buttonS">' + tab + '<a style="display:none"', 1)

with open(path, 'w') as f:
    f.write(new_src)
print('[patch_nav] Nav tab injected.')
