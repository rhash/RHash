"""Exercise public hash names through the compiled RHash CLI."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

binary = Path(sys.argv[1] if len(sys.argv) > 1 else "./rhash").resolve()
environment = dict(os.environ)
library = str(binary.parent / "librhash")
environment["LD_LIBRARY_PATH"] = library + os.pathsep + environment.get("LD_LIBRARY_PATH", "")


def run(*arguments):
    return subprocess.run([str(binary), *arguments], env=environment,
                          text=True, capture_output=True, check=True, timeout=10).stdout


with tempfile.TemporaryDirectory() as temporary:
    sample = Path(temporary) / "a b"
    sample.write_bytes(b"abc")
    names = run("--list-hashes").splitlines()
    assert names
    for name in names:
        canonical = run("--printf", "%{" + name + "}", str(sample))
        alias = run("--printf", "%{" + name.lower().replace("-", "") + "}", str(sample))
        assert re.fullmatch("[0-9A-Za-z]+", canonical), (name, canonical)
        assert canonical.lower() == alias.lower(), (name, canonical, alias)
    assert run("--printf", "%{SHA-256}", str(sample)).lower() == hashlib.sha256(b"abc").hexdigest()
    assert run("--printf", "%{urlname}", str(sample)) == "a%20b"
    assert run("--printf", "%{mtime}", str(sample))
    print(f"PASS {len(names)} canonical/alias hash pairs, SHA-256 known value and legacy urlname/mtime")
