#!/usr/bin/env python3
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
library = next((root / "librhash").glob("librhash.so.*"))
with tempfile.TemporaryDirectory() as directory:
    directory = Path(directory)
    hook = directory / "fail_alloc.c"
    driver = directory / "check_init.c"
    hook.write_text(r'''#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <stddef.h>
static int fail_next;
static int failed_count;
static void* (*real_aligned_alloc)(size_t, size_t);
__attribute__((constructor)) static void resolve_aligned_alloc(void) {
    real_aligned_alloc = dlsym(RTLD_NEXT, "aligned_alloc");
}
void rhash_test_fail_next_aligned_alloc(void) { fail_next = 1; }
int rhash_test_failed_count(void) { return failed_count; }
void* aligned_alloc(size_t alignment, size_t size) {
    if (fail_next) { fail_next = 0; failed_count++; errno = ENOMEM; return NULL; }
    return real_aligned_alloc(alignment, size);
}
''')
    driver.write_text(r'''#include <dlfcn.h>
#include <stdio.h>
#include <rhash.h>
int main(void) {
    unsigned id = RHASH_SHA256;
    void (*fail_next)(void) = dlsym(RTLD_DEFAULT, "rhash_test_fail_next_aligned_alloc");
    int (*failed_count)(void) = dlsym(RTLD_DEFAULT, "rhash_test_failed_count");
    rhash ctx;
    if (!fail_next || !failed_count) return 2;
    rhash_library_init();
    fail_next();
    ctx = rhash_init_multi(1, &id);
    if (ctx) { rhash_free(ctx); return 3; }
    if (failed_count() != 1) return 4;
    puts("PASS: allocation failure returns NULL");
    return 0;
}
''')
    cc = os.environ.get("CC", "cc")
    subprocess.run([cc, "-shared", "-fPIC", str(hook), "-ldl", "-o", str(directory / "fail_alloc.so")], check=True)
    subprocess.run([cc, str(driver), "-I", str(root / "librhash"), str(library), "-ldl",
                    f"-Wl,-rpath,{root / 'librhash'}", "-o", str(directory / "check_init")], check=True)
    env = dict(os.environ, LD_PRELOAD=str(directory / "fail_alloc.so"),
               LD_LIBRARY_PATH=str(root / "librhash"),
               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    result = subprocess.run([str(directory / "check_init")], env=env, capture_output=True,
                            text=True, timeout=10)
    assert result.returncode == 0, (result.returncode, result.stdout, result.stderr)
    assert "PASS: allocation failure returns NULL" in result.stdout, result.stdout
    assert "runtime error:" not in result.stderr, result.stderr
print("PASS: rhash_init_multi handles aligned-allocation failure")
