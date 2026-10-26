#!/usr/bin/env python3
from pathlib import Path
import os
import subprocess
import sys
import tempfile

binary = str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as directory:
    config = Path(directory, "rhash", "rhashrc")
    config.parent.mkdir()
    config.write_text("=ignored\nsha256=on\n")
    env = dict(os.environ, XDG_CONFIG_HOME=directory)
    result = subprocess.run([binary, "--message=abc"], env=env,
                            capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, (result.returncode, result.stderr)
    assert "unknown option" in result.stderr, result.stderr
    assert "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad" in result.stdout, result.stdout
print("PASS: empty configuration name warns and subsequent SHA-256 option works")
