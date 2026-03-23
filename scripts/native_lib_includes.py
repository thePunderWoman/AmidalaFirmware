"""
native_lib_includes.py — PlatformIO pre-build extra script for the native env.

Arduino libraries (Reeltwo, etc.) are marked "framework incompatible" for the
native platform by PlatformIO's LDF, so their headers are never added to the
include path.  Native tests need those headers (but not the Arduino source
files, which are stubbed out by arduino_mock.h).

This script adds the relevant library src directories explicitly, preferring
the project-local lib/ copy (developer workflow) and falling back to the
PlatformIO-managed download under .pio/libdeps/ (CI workflow where lib/ is
gitignored).
"""

Import("env")  # noqa: F821 — SCons Import
import os

HEADER_ONLY_LIBS = ["Reeltwo"]
PROJECT_DIR = env.subst("$PROJECT_DIR")
PIOENV = env["PIOENV"]  # e.g. "native"

for lib in HEADER_ONLY_LIBS:
    for candidate in [
        os.path.join(PROJECT_DIR, "lib", lib, "src"),
        os.path.join(PROJECT_DIR, ".pio", "libdeps", PIOENV, lib, "src"),
    ]:
        if os.path.isdir(candidate):
            env.Append(CPPPATH=[candidate])
            break
