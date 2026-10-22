# Aurora

Aurora is an unofficial [Moonlight TV](https://github.com/mariotaku/moonlight-tv) fork for **LG webOS**, built for PC game streaming with a TV remote or gamepad. It connects to a GameStream-compatible host, including Sunshine and VibeShine.

The app uses a black OLED-friendly interface, an in-stream menu, a full on-screen keyboard, a virtual mouse, and performance diagnostics. **LG C2 is the current target for validation.** Results from other TVs, including earlier C5 experiments, do not establish compatibility with every model or streaming mode.

## Features

- **TV navigation:** D-pad and gamepad navigation, app tiles, and Stream, Input, Host, and Experimental settings.
- **Video options:** resolution and frame-rate selection, HEVC, AV1 when advertised by the decoder, and HDR when supported by the full host/TV path.
- **Bitrate control:** manual selection up to 400 Mbps in the UI and adaptive bitrate control. The selected decoder and host can apply a lower limit.
- **Streaming tools:** pinned performance stats, a full on-screen keyboard, virtual mouse, and configurable gamepad touchpad behavior.
- **Session controls:** resume a running application, disconnect while leaving it running, or request that the host end the session.
- **Diagnostics:** an on-screen log overlay and a bounded persistent `aurora.log` file.
- **Experimental webOS integration:** picture/sound and performance adjustments for rooted TVs, with session cleanup and restoration.

Available settings describe client capabilities; they do not guarantee that every codec, HDR mode, resolution, or frame rate will work on a particular TV. Physical VRR on the TV is not an implemented or validated feature.

## Screenshots

| Home | Settings |
|:---:|:---:|
| ![Home screen](docs/images/home.png) | ![Stream settings](docs/images/settings.png) |

| Performance stats | On-screen keyboard |
|:---:|:---:|
| ![Performance stats](docs/images/performance-stats.png) | ![On-screen keyboard](docs/images/keyboard.png) |

Screenshots illustrate the interface and may differ from the current build.

## Get started

1. Set up a GameStream-compatible host on your PC and expose the games or desktop you want to stream.
2. Install Aurora on the TV using one of the methods below.
3. Open Aurora, select a discovered PC or add its IP address, then complete pairing on the host.
4. Open Settings and choose a resolution, frame rate, codec, and bitrate that your TV and network can sustain.
5. Select an application to start streaming. Open the **?** button on the home screen for the built-in controls guide.

Start with **60 FPS** and a moderate bitrate, then increase one setting at a time. Higher bitrate is useful only when the network, host, and decoder can sustain it. Compare 60 and 120 FPS where supported; a high-refresh panel alone does not establish support for decoding every stream at 120 FPS. The older 3584 × 2016 recommendation came from C5 experiments and is not a universal C2 preset.

## Install

- **Homebrew Channel:** add `https://raw.githubusercontent.com/GuiDev1994/aurora-tv/main/repo.json` to [webOS Homebrew Channel](https://github.com/webosbrew/webos-homebrew-channel).
- **Device Manager:** install an IPK from [Releases](https://github.com/GuiDev1994/aurora-tv/releases) with [webOS Dev Manager](https://github.com/webosbrew/dev-manager-desktop).
- **webOS CLI:** install a locally built package with `ares-install aurora.ipk`. Release downloads may use a versioned filename; pass that filename instead.

Building a package and installing it on the TV are separate steps. See the [webOS build guide](docs/BUILD_WEBOS.md) for additional setup and troubleshooting.

## Controls and shortcuts

The **?** button opens a scrollable guide inside Aurora, with controls first and version, decoder, audio backend, and system information at the bottom. Use **Up/Down** or the mouse wheel to scroll, and **Back / B / Escape** to close it.

### TV remote during streaming

| Control | Action |
| --- | --- |
| **Hold Back** on the Magic Remote, or press **Red / Exit** | Open the stream menu. Holding Back sends the remote's Exit action. |
| **Back**, short press | Send Escape to the PC. Close the on-screen keyboard when it is open. |
| **Blue** | Open the on-screen keyboard. |
| **OK**, short press | Send Enter to the PC. |
| **Hold OK for at least 0.6 seconds**, then release | Right-click on the PC. |
| **Channel Up / Down** | Send Page Up / Page Down to the PC. |
| **Yellow** | Cycle the diagnostic log overlay: Off → Live → Frozen → Off. |

### Gamepad during streaming

| Control | Action |
| --- | --- |
| **LB + RB + Back + Start**, then release all buttons | Open the stream menu. On PlayStation layouts, use L1 + R1 and the corresponding Select/Share and Start/Options buttons. |
| **Hold Select / Back alone for 4 seconds** | Toggle pinned performance stats. This button is also forwarded to the PC; pressing another button cancels the pending shortcut. |
| **Y / Triangle** while virtual mouse is enabled | Open the on-screen keyboard. |

**TV remote Back and gamepad Select/Back have different shortcuts:** holding remote Back opens the menu; holding gamepad Select/Back toggles stats.

### Virtual mouse and on-screen keyboard

Enable **Virtual Mouse** from the stream menu, or enable **Settings → Input → Virtual mouse** to start streams with it enabled.

| Mode | Control | Action |
| --- | --- | --- |
| Virtual mouse | Right stick | Move the pointer. |
| Virtual mouse | Left stick | Scroll vertically or horizontally. |
| Virtual mouse | LT / RT | Left-click / right-click. |
| On-screen keyboard | D-pad + A / remote OK | Navigate and press a key. |
| On-screen keyboard | Y / X / Start | Space / Backspace / Enter. |
| On-screen keyboard | LT | Switch between letters and symbols. |
| On-screen keyboard | LB / RB | Move the text cursor left / right. |
| On-screen keyboard | B / remote Back | Close the keyboard. Gamepad B also sends Escape to the PC. |

Gamepad keyboard labels above use the standard ABXY layout; the button-swap setting affects navigation mappings. The stream menu's **Full keyboard** button also opens the keyboard.

### Leave or end a session

- **Disconnect:** return to Aurora while leaving the application running on the PC for a later resume.
- **End session:** ask the host to close the application or desktop session, after confirmation. This action is unavailable when connected directly through Remote Monitor or Remote Input.

## Build from source

Aurora uses **C11, CMake, LVGL v8, and SS4S**. The webOS build uses SDL3 through sdl2-compat; the Linux development build uses SDL2.

Initialize a fresh checkout with its nested submodules:

```sh
git clone --recurse-submodules https://github.com/GuiDev1994/aurora-tv.git
cd aurora-tv
```

For an existing checkout, inspect local changes before running `git submodule update --init --recursive`.

### webOS ARM package

With Docker Desktop on macOS/Windows or Docker Engine on Linux running, use:

```sh
python3 scripts/webos/build_ipk.py
```

The helper builds and validates the webOS ARM package and delivers **`aurora.ipk` at the repository root**. It preserves the previous package if the build or validation fails. Logs and receipts are stored under `.cache/ipk/`, with the latest receipt in `.cache/ipk/latest.json`.

The cross-build uses Linux/amd64 tools, including on Apple Silicon. It does not increment the version or install anything on the TV. `MOONLIGHT_VERSION` in `CMakeLists.txt` is the version source for package metadata, the UI, and the ELF version marker.

### Linux desktop

Install the dependencies listed in [desktop CI](.github/workflows/build-desktop.yml), including gettext, then run:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DTARGET_DESKTOP=ON
cmake --build build
xvfb-run ctest --test-dir build --output-on-failure
```

### UI simulator on macOS/Linux

With Docker running:

```sh
./tools/ui-sim/run.sh
```

This builds and exercises the real SDL/LVGL interface with dummy audio/video backends and isolated preferences. Reports, screenshots, and logs are written to `.cache/ui-sim-run-*/artifacts/`. The runtime has no network access. It validates UI behavior, not LG firmware, hardware decoding, HDR, or real streaming. See the [simulator guide](tools/ui-sim/README.md) for custom scenarios.

## Diagnostics

Aurora records INFO, warning, and error messages to **`aurora.log`**, even when the on-screen log overlay is off. The startup `FileLog` message reports the actual path and whether storage is persistent.

On webOS, the usual location is the app's own `$HOME/conf/aurora.log`, alongside its settings. For a standard Developer Mode installation, look in:

```text
/media/developer/apps/usr/palm/applications/com.aurora.gamestream/conf/aurora.log
```

Use Dev Manager's file browser to download it after reproducing an issue, preferably after leaving the stream. If persistent storage is unavailable, the fallback is `/tmp/moonlight-tv/conf/aurora.log`, which does not survive a TV reboot.

The file is capped at **2 MiB**. At the limit, Aurora retains approximately the newest 1 MiB of complete lines and continues in the same file. Restarting the app preserves recent history. Disk writes run on a separate thread with a bounded queue; DEBUG/VERBOSE records are excluded.

**Every installation starts a new diagnostic history.** On the first launch after installing or reinstalling an IPK (including the identical package/version), Aurora removes `aurora.log`, `aurora-crash.log` and `aurora-run.state` from both the persistent diagnostic directory and its temporary fallback, before starting the writers. The IPK supplies a one-time `aurora-logs-reset.pending` marker, which is consumed after successful cleanup; subsequent launches preserve that installation's logs and crash report. Only these three diagnostic files are removed. Settings, profiles and pairing data are preserved. The startup record `Diagnostic logs reset after installation` confirms cleanup; a failure is reported and keeps the marker for retry. Download any logs you need before installing another package.

Logs include UTC and monotonic timestamps, version/configuration, video receive/submit samples, adaptive bitrate decisions, and session interruption/cleanup progress. When reporting a problem, include the TV model, firmware, decoder backend, codec, resolution, FPS, HDR state, requested bitrate, network connection, and host version.

On webOS, two additional diagnostic files live in the same directory:

- **`aurora-crash.log`**: the last captured fatal signal, with version, PID, boot ID, UTC/monotonic seconds, signal code, fault address when available, ARM PC/SP/LR registers and executable module mappings. It is capped at **64 KiB**, remains empty until a fatal signal is captured, and survives normal app restarts. It is a small text report, not a full memory dump or stack trace.
- **`aurora-run.state`**: a bounded marker identifying the current/last run. `running` means startup was recorded; `clean` means normal teardown completed; `crashed` means the fatal handler ran. On the next launch, `aurora.log` warns if the previous run did not finish cleanly. Match the PID and boot ID before associating a retained crash report with that run.

The fatal handler bypasses the normal logging queue and uses fixed buffers and signal-safe I/O. It handles SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT and SIGTRAP, then re-raises the original signal. The main thread has an alternate signal stack; worker-thread stack overflow is not guaranteed capturable. SIGKILL, power loss, termination before diagnostics initialize, storage failure and severe process corruption can prevent a report. An unclean marker alone does not establish OOM, a decoder bug or a native crash. Full core dumps remain disabled in non-Debug webOS builds; changing the terminal's `ulimit` does not enable them in the app.

After another unexpected close, download **`aurora.log`**, **`aurora-crash.log`** and **`aurora-run.state`** through Dev Manager. The startup `CrashLog v1 armed` entry confirms that text crash capture initialized and identifies its actual directory. SDL lifecycle/exit requests and normal shutdown start/end are also recorded. These diagnostics do not depend on Dev Manager's Crash Reports list or root access. Persistence still depends on writable storage; `/tmp` fallback files do not survive reboot. Preserve the matching build's debug-symbol archive for resolving instruction addresses.

The performance overlay measures submission to the video backend, not physical display scanout. Player latency estimates and frame gaps are diagnostic signals, not direct measurements of decoder execution time or network packet loss. Persistent logs do not replace a native crash stack trace.

Implementation details and the compatibility audit are recorded in the [diagnostic log review](docs/DIAGNOSTIC_LOG_REVIEW.md).

## Project documentation

Some internal engineering documents are currently written in Portuguese.

- [Current implementation and validation status](docs/CURRENT_STATE.md)
- [webOS build and troubleshooting guide](docs/BUILD_WEBOS.md)
- [VibeShine integration and session contracts](docs/VIBESHINE_INTEGRATION.md)
- [Nonary Moonlight / webOS comparison](docs/NONARY_MOONLIGHT_WEBOS_REVIEW.md)
- [Historical streaming experiments](docs/WEBOS_STREAMING_HISTORY.md)

## Credits and license

Aurora is based on [mariotaku/moonlight-tv](https://github.com/mariotaku/moonlight-tv) and uses [moonlight-embedded](https://github.com/irtimmer/moonlight-embedded), [moonlight-common-c](https://github.com/moonlight-stream/moonlight-common-c), and other open-source components. Rights and attribution remain with their respective authors.

Licensed under [GNU GPL v3.0 or later](LICENSE). See [COPYRIGHT](COPYRIGHT) for attribution details. Provided without warranty.
