#!/usr/bin/env python3
"""Verify APK integrity and the permanent release signing identity."""
import os
from pathlib import Path
import re
import subprocess
import sys

EXPECTED_SHA256 = '8032fd2a79885883916a0de46e3fb8e64b21db502b96c482ee0b55fba85a0464'


def verify(apk):
    sdk = os.environ.get('ANDROID_HOME') or os.environ.get('ANDROID_SDK_ROOT')
    if not sdk:
        raise SystemExit('ANDROID_HOME or ANDROID_SDK_ROOT must identify the Android SDK')
    candidates = list((Path(sdk) / 'build-tools').glob('*/apksigner'))
    if not candidates:
        raise SystemExit('No apksigner found in the Android SDK build-tools')
    tool = max(candidates, key=lambda p: tuple(int(n) for n in re.findall(r'\d+', p.parent.name)))
    result = subprocess.run([str(tool), 'verify', '--print-certs', apk],
                            text=True, capture_output=True, check=True)
    fingerprints = re.findall(r'^Signer #\d+ certificate SHA-256 digest: ([0-9a-fA-F]+)$',
                              result.stdout, re.M)
    if [value.lower() for value in fingerprints] != [EXPECTED_SHA256]:
        raise SystemExit('APK signing certificate does not match the permanent release key')
    print('APK signature verified; permanent release certificate SHA-256: ' + EXPECTED_SHA256)


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('Usage: verify-release-signature.py APK')
    verify(sys.argv[1])
