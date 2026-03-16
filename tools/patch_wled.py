#!/usr/bin/env python3
"""
patch_wled.py - registers BambuUsermod in WLED's usermods_list.cpp
"""
import sys, os, re

def patch(wled00_dir):
    target = os.path.join(wled00_dir, 'usermods_list.cpp')
    if not os.path.exists(target):
        print(f'[patch_wled] ERROR: {target} not found')
        sys.exit(1)

    with open(target, 'r') as f:
        src = f.read()

    if 'BambuUsermod' in src:
        print('[patch_wled] Already patched, skipping.')
        return

    # Add include at top
    src = '#include "bambu_status.h"\n' + src

    # Register instance - find the line that calls registerUsermod
    # In WLED 0.15, usermods_list.cpp has a function void registerUsermods()
    # We add our registration before the closing brace
    src = re.sub(
        r'(void\s+registerUsermods\s*\(\s*\)\s*\{)',
        r'\1\n  usermods.add(new BambuUsermod());',
        src
    )

    with open(target, 'w') as f:
        f.write(src)

    print(f'[patch_wled] Patched {target}')
    for line in src.split('\n'):
        if 'Bambu' in line or 'bambu' in line:
            print(f'  >> {line}')

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Usage: patch_wled.py <path/to/wled00/>')
        sys.exit(1)
    patch(sys.argv[1])
