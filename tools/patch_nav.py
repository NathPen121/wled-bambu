#!/usr/bin/env python3
"""Injects a Bambu tab into WLED's index.htm nav bar."""
import re, sys

path = 'wled-src/wled00/data/index.htm'
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
    print('[patch_nav] WARNING: Could not find settings tab anchor, appending tab manually')
    # Fallback: inject before </nav> or </div> at end of nav
    new_src = src.replace('id="buttonS"', 'id="buttonS"' + tab, 1)

with open(path, 'w') as f:
    f.write(new_src)
print('[patch_nav] Nav tab injected.')
