# AGENTS.md

Guidelines for working on Aurora. Prioritize correct webOS behavior, predictable
latency, and an interface usable with the TV remote. Implement the requested
scope while preserving local work and existing session contracts.

## Project and starting point

Aurora (`com.aurora.gamestream`) is a C11 fork of moonlight-tv for LG webOS.
The Linux desktop build supports development. The UI uses LVGL v8 and SDL2;
on webOS, that API runs on SDL3 through sdl2-compat, with custom NDL and key
shims. Builds use CMake and submodules. CI targets webOS and desktop; legacy
scripts for other platforms do not imply active support.

Before changing an area, inspect its code and `git status --short`. Do not revert
existing checkout changes or update submodules incidentally. HEAD alone does
not identify a version with local changes.

- [Current state](docs/CURRENT_STATE.md): dated baseline, references,
  implementation, gaps, and validation. Consult it for planning, investigation,
  and comparison; verify task-relevant claims against the code.
- [Streaming history](docs/WEBOS_STREAMING_HISTORY.md): experiments and audio
  already evaluated on the C5. Read before changing pacing, PTS, AV1 120, or 5.1 mapping.
- [VibeShine integration](docs/VIBESHINE_INTEGRATION.md) and
  [Nonary/webOS review](docs/NONARY_MOONLIGHT_WEBOS_REVIEW.md): contracts and analysis.
  The review includes later corrections; verify the working tree before treating
  a historical diagnosis as a current defect.

The current target of interest is the LG C2. Keep C5 + eARC evidence identified
as such. The CMake version, generated IPK, and version installed on the TV are
separate states; do not assume they match.

## Code map

| Responsibility | Location |
| --- | --- |
| Startup and main loop | `src/app/app.c`, `src/main.c` |
| GameStream protocol, XML, pairing, launch | `core/libgamestream/`; Limelight transport and callbacks in `core/moonlight-common-c/` |
| Discovery, hosts, catalog | `src/app/backend/pcmanager/`, `src/app/backend/apploader/` |
| Session lifecycle, configuration, ABR | `src/app/stream/session.c`, `session_worker.c`, `adaptive_bitrate.c` |
| Streaming video, audio, input | `src/app/stream/video/`, `audio/`, `input/`; SS4S output in `third_party/ss4s/` |
| Launcher, settings, overlay | `src/app/ui/launcher/`, `settings/`, `streaming/` |
| Preferences and profiles | `src/app/app_settings.{h,c}`, `src/app/profile/` |
| webOS integration, SDL/NDL, Game Mode, priority | `src/app/platform/webos/`, `third_party/commons/cmake/ExternalSDL3ForWebOS.cmake` |
| Tests, simulator, packaging | `tests/`, `tools/ui-sim/`, `scripts/webos/`, `.github/workflows/` |

SS4S selects modules at runtime. Confirm the selected backend in the log; do not
attribute Starfish/SMP or simulator dummy decoder behavior to NDL.

## Build and validation

Run commands from the Aurora root. In a fresh clone, initialize with
`git submodule update --init --recursive`; moonlight-common-c has nested
dependencies. Inspect submodule state first in an existing checkout.

### Linux desktop

Ubuntu dependencies are in
[build-desktop.yml](.github/workflows/build-desktop.yml); the Ubuntu 24.04 simulator
image includes the expanded development set. Desktop configure requires
`gettext` (`msgfmt`, `xgettext`, `msgmerge`).

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DTARGET_DESKTOP=ON
cmake --build build
xvfb-run ctest --test-dir build --output-on-failure
```

`BUILD_TESTS` defaults to ON for desktop and OFF for webOS. Select relevant tests,
such as `ctest --test-dir build -R test_settings --output-on-failure`, for focused
changes. Main tests use Unity; protocol fixtures are a standalone executable.

### macOS/Linux with Docker: UI and protocol

```sh
./tools/ui-sim/run.sh
```

The [simulator](tools/ui-sim/README.md) builds the real SDL/LVGL UI with dummy
SS4S output. Runtime has no network and uses isolated preferences. Results are
in `.cache/ui-sim-run-*/artifacts/` (`report.json`, `index.html`, PNGs, logs).
Use scenarios for focus, navigation, popups, mouse/wheel, directional input, and
resize. This does not emulate LG firmware or validate real decoding/audio/streaming.

For VibeShine contracts on Linux with development dependencies:

```sh
bash tests/core/run_vibeshine_protocol.sh
```

On Mac, after building the simulator image, run the same fixture on Linux:

```sh
docker run --rm --network none --volume "$PWD:/source:ro" --workdir /source aurora-ui-sim:local bash tests/core/run_vibeshine_protocol.sh
```

### webOS

For a local installer, use the
[aurora-build-ipk](.agents/skills/aurora-build-ipk/SKILL.md) skill or:

```sh
python3 scripts/webos/build_ipk.py
```

The fixed deliverable is **`aurora.ipk` at this repository root**, replaced only
after a successful build and validation. Logs and receipt are in `.cache/ipk/`.
Building does not bump the version. Use
[aurora-bump-version](.agents/skills/aurora-bump-version/SKILL.md) for version
changes: patch by default, or the requested version/type. `MOONLIGHT_VERSION`
is the single source; appinfo, UI, and ELF marker must derive from it. An older
IPK does not acquire the new source version until rebuilt.

Consult [BUILD_WEBOS.md](docs/BUILD_WEBOS.md), `scripts/webos/`, and the
[CI action](.github/actions/build-webos/action.yml). The SDK is buildroot-nc4
`webos-a38c582`, x86_64 host, ARM target. ARM Macs need a compatible Linux x86_64
environment; the ARM64 simulator build does not replace this cross-build.
With SDK/ares-package ready on Linux or WSL:

```sh
CI=1 TOOLCHAIN_FILE=/path/to/sdk/share/buildroot/toolchainfile.cmake ./scripts/webos/easy_build.sh -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

`CI=1` skips automatic submodule updates in that script; initialize them first.
`TARGET_WEBOS` is detected from the toolchain/compiler and changes sources,
libraries, and resources. webOS builds convert `.po` files to packaged `cstrings.json`.

Use the maintained IPK helper for reproducible local delivery. Builds can produce IPKs in
temporary directories. Copy the artifact to a persistent location before closing
the environment, and report the actual generated file. Build/packaging and
installation are separate steps; documentation, review, or build tasks do not
automatically authorize TV installation.

## UI, preferences, localization

- Use LVGL v8 fragments. Preserve ownership, callbacks, and focus groups. Cancel
  asynchronous callbacks retaining a fragment when destroying it. Validate
  Back/Escape and focus restoration after dropdowns, popups, and overlay close.
- `entries[]` in `settings.controller.c` defines accessible panes: Stream, Input,
  Host, Other. Compiling a pane does not expose it. Video/bitrate belong in Stream
  (`basic.pane.c`); language, logs, Game Mode, and reset in `other.pane.c`.
  Do not recreate inaccessible video/audio panes.
- New preferences need a field, default, INI read/write, accessible UI, and a
  real consumer. Decide profile/global scope and check both conversion directions
  in `profile_manager.c`. A saved checkbox does not prove an effect.
  `app_configuration` references `&app->settings`; sessions receive an effective copy.
- Distinguish live changes from next-session settings. Set `needs_stream_reconnect`
  when needed and explain effective clamped values. Do not add options without
  demonstrated capability/effect in the selected backend.
- When removing a key, preserve compatibility in the shared ignored legacy-key
  block in `app_settings.c`. Do not delete preferences/pairing to bypass migration.
- UI strings use `locstr`/`translatable` and `src/i18n/` catalogs. Update affected
  translations, especially pt-BR, preserving placeholders and the desktop gettext /
  webOS cstrings pipeline. For existing-language updates/review, use
  [aurora-update-languages](.agents/skills/aurora-update-languages/SKILL.md): adapt to
  each language's context, validate both formats, and inspect the UI with the
  language actually loaded, including fonts and truncation.

## Session and latency invariants

- Preserve main-loop cadence: `app_process_events`, `lv_task_handler`, `SDL_Delay(1)`,
  and `LV_INDEV_DEF_READ_PERIOD=1`. Longer intervals affect input.
- Event dispatch runs through `SDL_FilterEvents` under the SDL queue mutex. Do not
  perform network/disk I/O, joins, or long waits there. Use the existing bus for
  cross-thread work and respect data lifetime.
- Video uses `CAPABILITY_DIRECT_SUBMIT`: blocking feed also blocks RTP reception.
  webOS audio avoids direct submit so Opus 5.1 transcoding cannot monopolize the
  receive thread. Do not change this as an optimization without measurement.
- Decode-unit and Limelight timestamps use microseconds with a shared epoch in
  the current `PltGetMicroseconds`. Recheck units/epoch on moonlight-common-c
  updates; NDL receives PTS in milliseconds.
- Stop/join ABR before `LiStopConnection`. Do not restore bitrate for an ended
  session or send adjustments after `/cancel`. Preserve backend ceiling, host
  clamp, and effective configuration during renegotiation.
- Disconnect leaves the application on the host; End session requests `/cancel`
  with the existing confirmation. Remote Monitor/Input are special roles: use
  UUID/role and host rules, not card titles. Interpret HTTP 200 and XML 410 at
  protocol level; never automatically repeat Terminate.
- Preserve `surroundParams=642014523` and matching Opus 5.1 passthrough. Do not add
  Sunshine `surround-params` or force PCM/RemapPcm51 as a generic fix. Consult
  streaming history for C5 + eARC/channel conventions; validate other models/audio
  paths separately.
- Game Mode and priority/root adjustments have session entry/restoration lifecycles.
  Preserve cleanup on error paths. This is distinct from HDMI ALLM.

## webOS diagnostics and compatibility

Record model, firmware, backend, effective codec, resolution, FPS, HDR,
requested/applied bitrate, connection, and host configuration in experiments.
C5 reports do not validate C2; HDMI capability does not prove internal decoder
support. Host/virtual-display VRR, pacing, V-Sync, and physical VRR are distinct.

Overlay FPS measures submission, not scanout. `Play` duration is not decode time;
estimated queues are not physical latency. Compare motion with a reproducible
workload and isolate variables. Read [pacing/AV1 history](docs/WEBOS_STREAMING_HISTORY.md)
before another PTS experiment; check current environment and activation conditions.

Persistent logs use `src/app/util/file_log.*`, bounded queues, and writes outside
the feed path. Do not log input keys, credentials, or complete secret-bearing
launch URLs. Preferences/logs use `$HOME/conf` when writable, with temporary
fallback `/tmp/moonlight-tv/conf`; confirm the startup path. Logs do not replace
stack traces or alone establish why a host went offline.

User preference: **every installation must start with clean logs**. Every IPK
includes `aurora-logs-reset.pending`, consumed on first launch before writers:
it clears only `aurora.log`, `aurora-crash.log`, `aurora-run.state` in persistent
and temporary diagnostic directories. This includes reinstalling the same
IPK/version. Later launches preserve current-installation logs, including the
last crash. Do not use version change as the sole trigger or delete settings,
profiles, or pairing. The IPK helper validates the marker.

## Dependencies and project comparisons

LVGL is mariotaku's v8 fork; v9 migration is not routine maintenance. SS4S/commons
and moonlight-common-c have application contracts; changes must identify commits
and local patches. Do not raise SS4S video caps solely from panel specifications.
The commons dimensions parser supports H.264/HEVC; absent AV1 dimensions in stats
do not prove stream failure.

Use the relevant skill for substantial compatibility comparisons/investigations:

- [aurora-compare-vibeshine](.agents/skills/aurora-compare-vibeshine/SKILL.md): client/host
  contracts, catalog, remote sessions, HDR/SDR, input/audio, bitrate.
- [aurora-compare-nonary](.agents/skills/aurora-compare-nonary/SKILL.md): Qt client
  differences and viable webOS/C2 adaptations, presentation, codecs, telemetry.

Read the selected SKILL.md. Usual references are `../vibeshine` and an identified
Nonary/moonlight-qt checkout; do not depend on old `/tmp` clones. Capture local
commits/diffs/submodules with `scripts/dev/comparison_snapshot.py`. Routine edits
do not require comparing all three projects. Analysis is not a validated feature.

## Completing work

Run scope-appropriate checks: docs/links for documentation, simulator for UI,
fixtures for protocol, builds for source integration, physical tests for real
streaming claims. Record passed and unexecuted checks; compilation, policy tests,
and simulation do not prove scanout, physical audio, or host cleanup on a real TV.

When changing behavior, resolving an issue, or updating a reference, update the
relevant area of [CURRENT_STATE.md](docs/CURRENT_STATE.md). Keep invariants and
working guidance here; mutable versions/results/issues belong in the baseline,
detailed experiments in reports. The final response must state what changed,
why, validation, and material limitations. Do not claim unverified deployment,
installed version, or hardware compatibility.
