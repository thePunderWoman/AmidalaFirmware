# Amidala Firmware — Claude Notes

## Design principles

This firmware is intended to run on many different R2 builds with varying hardware configurations. Keep these principles in mind for all new features, integrations, and refactoring work.

**Modularity and flexibility**
- Features should be independently enable/disable-able via runtime configuration (stored settings) rather than requiring source edits. Reserve `#define` compile-time flags for major component-level choices only — e.g. swapping in a different drive system or audio system.
- Avoid hard-coding assumptions about which hardware is present. New integrations should degrade gracefully when a feature is disabled.
- Prefer small, focused classes and functions with clear responsibilities over monolithic ones.

**Test coverage**
- All new logic should have unit tests in `test/` to prevent regressions.
- Tests run on the native PlatformIO environment (no hardware required) — keep them that way. Do not introduce test dependencies that require Arduino or physical hardware.
- When fixing a bug, add a test that would have caught it.

## Git workflow

**All changes must go through pull requests — never push directly to `main`.**

1. Create a feature branch, make your changes, then open a PR.
2. Always target **`github.com/thePunderWoman/AmidalaFirmware`** (the `thePunderWoman` remote), not the upstream fork.
3. Push branches and create PRs with:
   ```
   git push thePunderWoman <branch>
   gh pr create --repo thePunderWoman/AmidalaFirmware ...
   ```

## Build verification

CI only runs the native unit tests (`pio test -e native`).
**Always verify the firmware build locally before pushing:**

```
pio run
```

If the firmware build is broken, fix it before opening or merging a PR.

## Running tests

```
pio test -e native
```

Tests live in `test/` and use the native PlatformIO environment (no hardware required).
