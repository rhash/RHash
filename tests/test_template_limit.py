#!/usr/bin/env python3
"""Test that template loading stops at the configured size limit."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if not hasattr(os, "mkfifo"):
    print("SKIP: mkfifo is not available on this platform")
    sys.exit(0)

BINARY = str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as directory:
    template = Path(directory, "template.fmt")
    os.mkfifo(template)
    with subprocess.Popen(
        [BINARY, f"--template={template}", "--message=abc"],
        cwd=directory,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    ) as process:
        with template.open("wb", buffering=0) as writer:
            try:
                writer.write(b"x" * 65536)
            except BrokenPipeError:
                pass
            try:
                stdout, stderr = process.communicate(timeout=2)
            except subprocess.TimeoutExpired as exc:
                process.kill()
                stdout, stderr = process.communicate()
                raise AssertionError(
                    "RHash kept reading the template after the size limit"
                ) from exc
        assert process.returncode == 2, (process.returncode, stdout, stderr)
        assert "template file is too big" in stderr, stderr
print("PASS: template loading stops at the configured size limit")
