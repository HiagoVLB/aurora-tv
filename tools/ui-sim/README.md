# Aurora visual simulator

Runs **Aurora's real SDL/LVGL UI** on Linux in Docker, with a dummy SS4S decoder,
keyboard/mouse input, and 1920 × 1080 PNG captures. Works on Mac with Docker
Desktop; validated on Linux ARM64 inside Docker on Mac.

The architecture follows the [PlxNative simulator](https://github.com/GLinnik21/plx-native/blob/3ac9e03fd4ef8b3c3bf1061c6eb372d970a6a3da/docs/building.md):
a separate development executable, isolated per-run state, commands, captures,
and automated validation. The reference was examined at commit
`3ac9e03fd4ef8b3c3bf1061c6eb372d970a6a3da`, including `tools/sim-smoke.py`.
This adapter is an independent Aurora implementation; it does not run the Plex
client or LG firmware.

## Run on Mac

With Docker Desktop running, from the repository root:

```sh
./tools/ui-sim/run.sh
```

To review translations at 1920×1080 and 1280×720, use the localization scenario:

```sh
AURORA_UI_SIM_LANGUAGE=pt-BR ./tools/ui-sim/run.sh tools/ui-sim/localization.json
```

The wrapper accepts existing catalog codes, prepares the corresponding UTF-8
locale, and uses compiled MO files in `/build/mo`. The image includes CJK fonts
for Japanese, Korean, and Chinese inspection. `expect`, `absent`, `click_text`,
and `scroll_text` strings are translated through the actual test catalog;
a missing MO fails the run. The report records language/locale, and the app log
identifies the loaded catalog. Inactive `uk` and `zh-TW` catalogs can also be
examined; this does not enable them in the selector or webOS package.

Linux fonts do not establish glyph availability on the TV. Open captures to
check truncation and glyphs; text expectations alone do not validate layout.

The first run builds the Ubuntu 24.04 image and application. Later runs reuse
the image and an architecture-specific build volume. Ubuntu 24.04 supplies
SDL 2.30; Ubuntu 22.04's SDL lacks some controller types Aurora uses.

The command prints the results directory, `.cache/ui-sim-run-*/artifacts/`:

- `index.html`: visual report with one capture per step;
- `report.json`: action/check results, including shutdown failures;
- `step-NNN.png`: capture from the real renderer;
- `step-NNN.json`: visible elements, text, coordinates, focus, disabled state;
- `process.log`: process stdout/stderr;
- `state/`: per-run preferences, profiles, and `aurora.log`.

Image/build logs are one level above, in `image-build.log` and `compile.log`.
The default scenario checks Stream, Input, Host, and Other settings, cancelling
Add PC, and cancelling Exit with mouse and Escape. Stream options occupy the
main settings list; the other three open secondary panes.

Runtime uses `--network none`, read-only source/build mounts, and no TV devices
or credentials. These tests cannot send `/cancel` to a real host. The application
and its backend still execute: for example, mDNS failure without a network is
exercised through the actual implementation.

## Streaming options

`tools/ui-sim/stream-options.json` exercises SDR YUV, HEVC slices, periodic
refresh, ABR, codec changes, logs, and confirmed/cancelled reset. It requires
`AURORA_UI_SIM_VIDEO_FIXTURE=1` in the runtime container: the driver advertises
synthetic HEVC/AV1/HDR to test enablement and preferences. Output remains dummy;
this does not validate decoding. JSON trees include checkbox `checked` values
to verify transitions, alongside `disabled` state.

`tools/ui-sim/dependent-options.json` checks dependency explanations, AV1/HEVC
switching, refresh and ABR, Home/Numpad help, and focus across the four session
menu actions. It also requires `AURORA_UI_SIM_VIDEO_FIXTURE=1` at runtime.
The `stream_menu` action mounts the real view and navigation over a black
background without statistics or a session. Buttons have no action callbacks:
it does not start streaming or send Disconnect/End session to a host. Escape
closes the fixture and restores the previous focus group. This validates
presentation/navigation; `test_remote_home` exercises webOS Home/Numpad mapping.
Physical key delivery from the TV still requires hardware validation.

## Scenarios

```sh
./tools/ui-sim/run.sh /path/to/scenario.json

# Initialize UI and fonts at the chosen resolution (up to 3840x2160).
AURORA_UI_SIM_SIZE=1280x720 ./tools/ui-sim/run.sh
```

Example:

```json
{
  "name": "settings",
  "steps": [
    {"action": "settings"},
    {"expect": "Resolution"},
    {"click_text": "Other"},
    {"expect": "Show logs"},
    {"key": "Down"},
    {"key": "Escape"}
  ]
}
```

Scenario commands:

| Command | Argument |
| --- | --- |
| `action` | `settings`, `add_host`, `help`, or `exit`; clicks the real toolbar button. `stream_menu` opens the isolated session-menu fixture |
| `click_text` | Exact English text; scrolls to and clicks the item, including dialog buttons |
| `scroll_text` | Scrolls to text without clicking; useful for inspecting sections/messages |
| `click` | Coordinates, for example `"100 200"` |
| `key` | SDL name: `Down`, `Up`, `Return`, `Escape`, etc. |
| `text` | Short SDL text event, up to 31 UTF-8 bytes, for the focused field |
| `wait` | 0–10,000 ms; keeps processing the UI loop |
| `wheel` | SDL wheel steps; negative scrolls down, positive up |
| `resize` | New window area, for example `"1280 720"`; preserves the open menu |
| `expect` | Text that must be visible |
| `absent` | Text that must not be visible |

Each action produces a capture and element tree. Failures return a nonzero
process status and preserve results; they are never converted to passes.
Captures must be valid PNGs at the run resolution (or last `resize`) with
nonuniform content. Expectations check actual LVGL state, beyond pixel
comparisons. The process must exit normally for a run to pass.

## Local Linux build

With Dockerfile dependencies installed:

```sh
cmake -S . -B build/ui-sim -DBUILD_UI_SIM=ON -DBUILD_TESTS=OFF
cmake --build build/ui-sim --target aurora-ui-sim
python3 tools/ui-sim/run.py --binary build/ui-sim/tools/ui-sim/aurora-ui-sim \
  --output /tmp/aurora-ui-run
```

The output directory must be empty. The runner requires Python, Pillow, Xvfb,
and xauth. The executable opens no HTTP server/control port: the driver uses
stdin/stdout. `BUILD_UI_SIM` defaults to OFF and is rejected for webOS builds;
simulator commands are excluded from the IPK.

## Limits and discovered defect

This environment validates navigation, focus, panes, dialogs, and lifecycle.
It does not validate NDL/Starfish decoding, AV1/HDR, video/audio synchronization,
jail permissions, or streaming/termination against real VibeShine. Native UI
is controlled through the SDL adapter; there are no Playwright DOM selectors.

The initial scenario passed its actions but hung on shutdown. mDNS had failed
because there was no network interface; its thread freed `discovery_task_t`,
while `discovery->task` still pointed to the freed memory. Stop accessed that
structure's mutex. The previous version reproduced
`AddressSanitizer: heap-use-after-free` in `discovery_worker_stop`.

The fix keeps tasks owned by the manager until their threads are joined.
Normal stop remains asynchronous; final destruction waits for threads before
freeing callback dependencies. `test_discovery_lifecycle` forces early exit,
repeated stop, restart, and destruction with an active callback. This defect
was reproduced in the test environment; it does not establish the cause of the
failure previously observed on C2.
