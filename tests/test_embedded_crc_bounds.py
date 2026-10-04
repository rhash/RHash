#!/usr/bin/env python3
from pathlib import Path
import subprocess
import sys
import tempfile
import zlib

binary = str(Path(sys.argv[1]).resolve())
crc = f"{zlib.crc32(b"A"):08X}"
cases = [("a", False), ("123456789", False), ("1234567890", False),
         (f"[{crc}]", True), (f"({crc})", True),
         (f"prefix_[{crc}].data", True), (f"[{crc}][ZZZZZZZZ]", True),
         ("[ZZZZZZZZ]", False), ("x[ZZZZZZZZ]", False),
         (f"[{crc}]/plain.data", False), (f"nested/[{crc}].data", True)]
with tempfile.TemporaryDirectory() as directory:
    for index, (name, expected) in enumerate(cases):
        case_directory = Path(directory, str(index))
        path = case_directory / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(b"A")
        result = subprocess.run([binary, "--check-embedded", name], cwd=case_directory,
                                capture_output=True, text=True, timeout=10)
        successful = "OK" in result.stdout and result.returncode == 0
        assert successful == expected, (name, result.returncode, result.stdout, result.stderr)
print(f"PASS: {len(cases)} embedded CRC filename cases")
