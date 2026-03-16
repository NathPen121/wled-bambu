#!/usr/bin/env python3
"""Patches WLED's platformio.ini - removes bad flags, adds PubSubClient."""
import re

with open('platformio.ini', 'r') as f:
    src = f.read()

# Remove unsupported compiler flag
src = src.replace('-Wshadow=compatible-local', '')

# Add PubSubClient - try nodemcuv2 first, fall back to [common]
if 'PubSubClient' not in src:
    new_src = re.sub(
        r'(\[env:nodemcuv2\][^\[]*?lib_deps\s*=\s*[^\[]+?)(\n\[|\Z)',
        lambda m: m.group(1).rstrip() + '\n  knolleary/PubSubClient@^2.8\n' + m.group(2),
        src, flags=re.DOTALL
    )
    if new_src == src:
        # nodemcuv2 has no own lib_deps - add to [common]
        new_src = re.sub(
            r'(\[common\][^\[]*?lib_deps\s*=\s*[^\[]+?)(\n\[|\Z)',
            lambda m: m.group(1).rstrip() + '\n  knolleary/PubSubClient@^2.8\n' + m.group(2),
            src, flags=re.DOTALL
        )
    src = new_src
    print('[patch_platformio] Added PubSubClient')

# Add MQTT buffer size - try nodemcuv2 build_flags, fall back to [common]
if 'MQTT_MAX_PACKET_SIZE' not in src:
    new_src = re.sub(
        r'(\[env:nodemcuv2\][^\[]*?build_flags\s*=\s*[^\[]+?)(\n\[|\Z)',
        lambda m: m.group(1).rstrip() + '\n  -DMQTT_MAX_PACKET_SIZE=4096\n' + m.group(2),
        src, flags=re.DOTALL
    )
    if new_src == src:
        new_src = re.sub(
            r'(\[common\][^\[]*?build_flags\s*=\s*[^\[]+?)(\n\[|\Z)',
            lambda m: m.group(1).rstrip() + '\n  -DMQTT_MAX_PACKET_SIZE=4096\n' + m.group(2),
            src, flags=re.DOTALL
        )
    src = new_src
    print('[patch_platformio] Added MQTT_MAX_PACKET_SIZE')

with open('platformio.ini', 'w') as f:
    f.write(src)

print('[patch_platformio] Done.')
