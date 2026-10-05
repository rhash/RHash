#!/usr/bin/env python3
import os
from pathlib import Path
import subprocess
import sys
import tempfile

binary = str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as directory:
    template = Path(directory, "template.fmt")
    os.mkfifo(template)
    process = subprocess.Popen([binary, f"--template={template}", "--message=abc"],
                               cwd=directory, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               text=True)
    with template.open("wb", buffering=0) as writer:
        try:
            writer.write(b"x" * 65536)
        except BrokenPipeError:
            pass
        try:
            stdout, stderr = process.communicate(timeout=2)
        except subprocess.TimeoutExpired:
            process.kill()
            stdout, stderr = process.communicate()
            raise AssertionError("RHash kept reading the template after the size limit")
    assert process.returncode == 2, (process.returncode, stdout, stderr)
    assert "template file is too big" in stderr, stderr
print("PASS: template loading stops at the configured size limit")
