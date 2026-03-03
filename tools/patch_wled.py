#!/usr/bin/env python3
"""
patch_wled.py — injects P1S hooks into WLED's wled00.ino

Usage: python3 patch_wled.py path/to/wled00/wled00.ino
"""

import sys
import re

INCLUDE_MARKER = '#include "wled.h"'
INCLUDE_INJECT = '''\
#include "bambu_status.h"
// wled_bambu_server.cpp included in bambu_status.cpp build
'''

SETUP_MARKER   = re.compile(r'(void\s+setup\s*\(\s*\)\s*\{[^\n]*\n)')
SETUP_INJECT   = '  setupBambuWebRoutes();\n  loadDefaultBambuEffects();\n'

LOOP_MARKER    = re.compile(r'(void\s+loop\s*\(\s*\)\s*\{[^\n]*\n)')
LOOP_INJECT    = '  pollBambu();\n  applyBambuEffects();\n'

def patch(path):
    with open(path, 'r') as f:
        src = f.read()

    # Skip if already patched
    if 'bambu_status.h' in src:
        print(f'[patch_wled] {path} already patched, skipping.')
        return

    # Inject include
    if INCLUDE_MARKER in src:
        src = src.replace(INCLUDE_MARKER,
                          INCLUDE_MARKER + '\n' + INCLUDE_INJECT, 1)
    else:
        # Prepend if marker not found
        src = INCLUDE_INJECT + src

    # Inject into setup()
    src = SETUP_MARKER.sub(r'\1' + SETUP_INJECT, src, count=1)

    # Inject into loop() — find closing brace of loop and insert before it
    src = LOOP_MARKER.sub(r'\1' + LOOP_INJECT, src, count=1)

    with open(path, 'w') as f:
        f.write(src)

    print(f'[patch_wled] Patched {path} successfully.')

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Usage: patch_wled.py <path/to/wled00.ino>')
        sys.exit(1)
    patch(sys.argv[1])
