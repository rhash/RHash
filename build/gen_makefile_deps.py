#!/usr/bin/env python3
"""
Regenerate the source-dependency rules in the LibRHash Makefile.

Runs `gcc -MM -DUSE_OPENSSL *.c` in the librhash source directory and
rewrites the block between the BEGIN/END markers in the Makefile with
per-source .o (static) and .lo (shared) rules.

Usage: gen_makefile_deps.py [source_dir]
"""

import os
import re
import subprocess
import sys

BEGIN_MARK = "BEGIN_GENERATED_SOURCES_DEPENDENCIES"
END_MARK = "END_GENERATED_SOURCES_DEPENDENCIES"
MAX_LINE_LENGTH = 80
MAX_RULE_LINE = MAX_LINE_LENGTH - 2  # 2 for trailing " \\"
NOT_SHARED_SOURCES = {"testlib.c"}
EXTRA_CFLAGS = {
    "rhash.c": "$(VERSION_CFLAGS)",
    "sha_ni.c": "$(SHA_CFLAGS)",
}


def run_gcc(src_dir):
    """Run gcc -MM -DUSE_OPENSSL *.c and return its stdout."""
    sources = (f for f in os.listdir(src_dir) if f.endswith(".c"))
    if not sources:
        sys.exit(f"error: no .c files found in {src_dir}")
    sorted_sources = sorted(sources)

    cmd = ["gcc", "-MM", "-DUSE_OPENSSL"] + sorted_sources
    try:
        proc = subprocess.run(
            cmd,
            cwd=src_dir,
            check=True,
            capture_output=True,
            text=True,
        )
    except FileNotFoundError:
        sys.exit("error: gcc not found in PATH")
    except subprocess.CalledProcessError as e:
        sys.stderr.write(e.stderr)
        sys.exit(f"error: gcc -MM failed (exit {e.returncode})")
    return proc.stdout


def parse_deps(gcc_mm_output):
    """
    Parse 'gcc -MM' output into a list of (target, prerequisites).

    gcc -MM emits, per source:
        crc32.o: crc32.c byte_order.h ustd.h crc32.h
    possibly wrapped over multiple lines with trailing backslashes.
    """
    text = gcc_mm_output.replace("\\\n", " ")
    entries = []
    for line in text.splitlines():
        line = line.strip()
        if not line or ":" not in line:
            continue
        target, _, prereqs = line.partition(":")
        entries.append((target.strip(), prereqs.split()))
    return entries


def extra_flags_for(source):
    """Return the extra CFLAGS to append for this source."""
    flag = EXTRA_CFLAGS.get(source)
    return f" {flag}" if flag else ""


def format_rule(prereqs, obj_ext, cflags):
    """Build the .o (or .lo) rule text for one source."""
    source = prereqs[0]
    target = os.path.splitext(source)[0] + obj_ext
    compile_flags = cflags + extra_flags_for(source)

    rule = f"{target}:"
    current_len = len(rule)
    for p in prereqs:
        if current_len + len(p) + 1 > MAX_RULE_LINE:
            rule += " \\\n"
            current_len = 0
        rule += " " + p
        current_len += len(p) + 1

    return f"\n{rule}\n\t$(CC) -c {compile_flags} $< -o $@\n"


def generate_deps_block(entries):
    """Return the block of generated rules."""
    # Group by source so that .o and .lo rules are adjacent
    by_source = {}
    for target, prereqs in entries:
        if not prereqs:
            sys.exit(f"error: no prerequisites for target {target!r}")
        by_source.setdefault(prereqs[0], prereqs)

    out = []
    for src, prereqs in by_source.items():
        out.append(format_rule(prereqs, ".o", "$(CFLAGS)"))
        if src not in NOT_SHARED_SOURCES:
            out.append(format_rule(prereqs, ".lo", "$(SHARED_CFLAGS)"))
    return "".join(out)


def find_marked_line(mark, text):
    """Find a commented line contating given mark"""
    mark_regex = rf"^[ \t]*#[ \t]*{re.escape(mark)}.*?$"
    if match := re.search(mark_regex, text, re.MULTILINE):
        return match
    sys.exit(f"error: no marker {mark} in Makefile")


def replace_deps_block(makefile_text, new_deps_block):
    """Replace the lines between BEGIN_MARK and END_MARK, keeping markers."""
    m_begin_mark = find_marked_line(BEGIN_MARK, makefile_text)
    m_end_mark = find_marked_line(END_MARK, makefile_text)
    if m_end_mark.start() < m_begin_mark.start():
        sys.exit("error: END marker appears before BEGIN marker")

    begin_block = makefile_text[: m_begin_mark.end()]
    end_block = makefile_text[m_end_mark.start():]
    return begin_block + new_deps_block + end_block


def main():
    src_dir = sys.argv[1] if len(sys.argv) > 1 else "."
    src_dir = os.path.abspath(src_dir)

    if not os.path.isdir(src_dir):
        sys.exit(f"error: not a directory: {src_dir}")

    makefile_path = os.path.join(src_dir, "Makefile")
    if not os.path.isfile(makefile_path):
        sys.exit(f"error: Makefile not found: {makefile_path}")

    gcc_mm_output = run_gcc(src_dir)
    entries = parse_deps(gcc_mm_output)
    if not entries:
        sys.exit("error: no dependencies parsed from gcc -MM output")

    new_block = generate_deps_block(entries)

    with open(makefile_path, "r", encoding="utf-8") as f:
        makefile_text = f.read()

    new_text = replace_deps_block(makefile_text, new_block)

    with open(makefile_path, "w", encoding="utf-8") as f:
        f.write(new_text)

    print(f"updated {makefile_path} ({len(entries)} source files)")


if __name__ == "__main__":
    main()
