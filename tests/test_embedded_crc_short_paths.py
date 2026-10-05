#!/usr/bin/env python3
import os
from pathlib import Path
import subprocess
import sys
import tempfile

binary = str(Path(sys.argv[1]).resolve())
env = dict(os.environ, ASAN_OPTIONS="detect_invalid_pointer_pairs=2")
with tempfile.TemporaryDirectory() as directory:
    Path(directory, "a").write_bytes(b"A")
    result = subprocess.run([binary, "--check-embedded", "a"], cwd=directory,
                            env=env, capture_output=True, text=True, timeout=10)
    assert "file name doesn't contain a CRC32" in result.stderr, result.stderr
    assert "invalid-pointer-pair" not in result.stderr, result.stderr
print("PASS: short filename is rejected without invalid pointer comparison")
