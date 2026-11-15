#!/usr/bin/env python3
import subprocess
import sys
import tempfile
from pathlib import Path

binary = str(Path(sys.argv[1]).resolve())

# (case name, file contents, expected stdout)
CASES = [
    ("bom_plus_colon", b"\xEF\xBB\xBF:", b":::"),
    ("bom_only", b"\xEF\xBB\xBF", b""),
]

with tempfile.TemporaryDirectory() as directory:
    for name, contents, expected in CASES:
        template = Path(directory, "template.fmt")
        template.write_bytes(contents)

        process = subprocess.run(
            [binary, f"--template={template}", "-m", "", "-m", "", "-m", ""],
            cwd=directory,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        assert process.returncode == 0, (
            name, process.returncode, process.stdout, process.stderr
        )
        assert process.stdout == expected, (
            name, "stdout mismatch", process.stdout, "expected", expected, process.stderr
        )

print("PASS: UTF-8 BOM is handled in printf-template")
