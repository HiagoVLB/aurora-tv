---
name: aurora-build-ipk
description: "Build, package, and validate Aurora for ARM webOS, always delivering aurora.ipk at the project root. Use to generate or regenerate the current IPK installer; preserves the previous package on failure and does not bump the version or install on the TV."
---

# Build the Aurora IPK

Locate Aurora root using `AGENTS.md` and `CMakeLists.txt`; read
[AGENTS.md](../../../AGENTS.md) and version/build sections in
[current state](../../../docs/CURRENT_STATE.md). Deliver **`aurora.ipk` at that
root**, not in the directory containing sibling repositories. Keep the same
filename for each build; internal version remains the central project version.

## Run

From Aurora root:

```sh
python3 scripts/dev/aurora_version.py --check
python3 scripts/webos/build_ipk.py
```

Use Python 3.9+. The helper selects Docker Linux/amd64 on Mac and without a
configured SDK; on Linux with valid `TOOLCHAIN_FILE`, it selects native build.
To choose explicitly:

```sh
python3 scripts/webos/build_ipk.py --backend docker --jobs 6
python3 scripts/webos/build_ipk.py --backend native --build-type Release
```

Native SDK environment must provide `cmake`, `cpack`, `ares-package`, ARM
toolchain, and build dependencies. Docker reuses/creates image
`aurora-webos-builder:local` and volume `aurora-webos-build-cache`; initial
downloads use project-pinned versions. Source is mounted read-only; SDK/build
remain in the volume, artifacts/logs in `.cache/ipk/`. Do not run two builds
concurrently in the same build volume/directory.

Initialize missing submodules recursively in fresh clones. Inspect patches
first in existing checkouts; helper does not pull/reset/update sources. If Docker
is unavailable, use an existing Linux SDK or report the concrete dependency;
do not substitute the UI simulator for cross-compilation.

## Accept only verified delivery

The helper builds in a new per-run directory, validates control/appinfo,
ID `com.aurora.gamestream`, version, ARM architecture, ELF version marker.
It compares source state before/after to reject builds during changes.
Only then does it atomically replace `aurora.ipk`. Build/validation failure
preserves the old installer; do not present it as newly built.

Inspect `.cache/ipk/latest.json`: version, SHA-256, commit/local state, backend,
build type, log directory. On error, read that run's `build.log` and fix the
cause before retrying. Do not choose an old `dist` IPK by mtime and rename it
as if freshly built.

Do not bump the version as a build side effect. If requested, first run
[aurora-bump-version](../aurora-bump-version/SKILL.md). Do not automatically
create tags/releases, push, or install on the TV.

Report a clickable `aurora.ipk` path, internal version, hash, validation result.
Distinguish package compilation from C2 operation: ELF/metadata checks validate
neither streaming, audio, nor installation. Update current state's build area
with the actual result.
