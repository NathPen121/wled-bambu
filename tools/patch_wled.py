#!/usr/bin/env python3
"""
patch_wled.py - injects Bambu hooks into WLED's main file
Handles both old (void setup/loop) and new (WLED::setup/WLED::loop) patterns
"""

import sys
import re
import os

INCLUDE_MARKER = '#include "wled.h"'
INCLUDE_INJECT = '#include "bambu_status.h"\n'

# Matches both:  void setup() {   AND   void WLED::setup() {
SETUP_MARKER = re.compile(r'(void\s+(?:WLED::)?setup\s*\(\s*\)\s*\{[^\n]*\n)')
SETUP_INJECT = '  setupBambuWebRoutes();\n  loadDefaultBambuEffects();\n'

LOOP_MARKER  = re.compile(r'(void\s+(?:WLED::)?loop\s*\(\s*\)\s*\{[^\n]*\n)')
LOOP_INJECT  = '  pollBambu();\n  applyBambuEffects();\n'

def find_main_file(directory):
    for name in ['wled_main.cpp', 'wled00.ino']:
        path = os.path.join(directory, name)
        if os.path.exists(path):
            print(f'[patch_wled] Found: {path}')
            return path
    return None

def patch(target):
    if os.path.isdir(target):
        path = find_main_file(target)
    else:
        path = target if os.path.exists(target) else find_main_file(os.path.dirname(target))

    if not path:
        print(f'[patch_wled] ERROR: Cannot find wled_main.cpp or wled00.ino in {target}')
        sys.exit(1)

    with open(path, 'r') as f:
        src = f.read()

    if 'bambu_status.h' in src:
        print(f'[patch_wled] Already patched, skipping.')
        return

    # Inject include after wled.h
    if INCLUDE_MARKER in src:
        src = src.replace(INCLUDE_MARKER, INCLUDE_MARKER + '\n' + INCLUDE_INJECT, 1)
    else:
        src = INCLUDE_INJECT + src

    # Inject into setup
    new_src, count = SETUP_MARKER.subn(r'\1' + SETUP_INJECT, src, count=1)
    if count == 0:
        print(f'[patch_wled] WARNING: setup() not found, appending to end of file')
        new_src = src + '\nvoid _bambu_init() {\n' + SETUP_INJECT + '}\n'
    src = new_src

    # Inject into loop
    new_src, count = LOOP_MARKER.subn(r'\1' + LOOP_INJECT, src, count=1)
    if count == 0:
        print(f'[patch_wled] WARNING: loop() not found')
    src = new_src

    with open(path, 'w') as f:
        f.write(src)

    print(f'[patch_wled] Patched {path} successfully.')
    # Show what was injected
    for line in src.split('\n'):
        if 'Bambu' in line or 'bambu' in line:
            print(f'  >> {line}')

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Usage: patch_wled.py <path/to/wled00/>')
        sys.exit(1)
    patch(sys.argv[1])
