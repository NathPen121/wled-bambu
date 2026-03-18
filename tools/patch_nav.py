#!/usr/bin/env python3
"""Injects a Bambu nav tab into WLED's index.htm via a script tag."""
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

if 'bambu-nav-inject' in src:
    print('[patch_nav] Already injected, skipping.')
    sys.exit(0)

# Inject a script that adds the Bambu button to the nav on page load.
# This is more robust than regex-matching the nav HTML structure.
script = '''<script id="bambu-nav-inject">
document.addEventListener("DOMContentLoaded",function(){
  var a=document.createElement("a");
  a.className="bt";a.href="/bambu";a.textContent="Bambu";
  var nav=document.querySelector("nav")||document.querySelector(".btns")||document.body;
  nav.appendChild(a);
});
</script>'''

# Insert just before </head>
if '</head>' in src:
    src = src.replace('</head>', script + '</head>', 1)
elif '</body>' in src:
    src = src.replace('</body>', script + '</body>', 1)
else:
    src = src + script

with open(path, 'w') as f:
    f.write(src)
print('[patch_nav] Nav script injected.')
