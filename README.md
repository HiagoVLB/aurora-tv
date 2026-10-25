# Aurora

Aurora is an unofficial [Moonlight TV](https://github.com/mariotaku/moonlight-tv) fork for **LG webOS**, built for PC game streaming with a TV remote or gamepad. It connects to a GameStream-compatible host, including Sunshine and VibeShine.

The app uses a black OLED-friendly interface, an in-stream menu, a full on-screen keyboard, a virtual mouse, and performance diagnostics. **LG C2 is the current target for validation.** Results from other TVs, including earlier C5 experiments, do not establish compatibility with every model or streaming mode.

## Features

- **TV navigation:** D-pad and gamepad navigation, app tiles, and Stream, Input, Host, and Other settings.
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

### Pair the TV with your PC

1. Start a GameStream-compatible host on the PC, such as Sunshine or VibeShine. Add the games or desktop you want to access to its application list.
2. Install Aurora on the TV using one of the [installation methods](#install) below. For the first connection, put the PC and TV on the same local network.
3. Open Aurora and choose your PC from **Select server**. If it does not appear, use the **+** button and enter the PC's local IP address.
4. Start pairing. Aurora displays a PIN on the TV; enter that PIN in the host's pairing screen on the PC.
5. After pairing, select the PC to see its applications. Open **?** for the controls guide or the gear button for **Settings**.

### Start a stream and adjust it

1. In **Settings → Stream**, choose a resolution and frame rate. Keep **Video backend** on **Auto** unless you are testing a specific decoder.
2. Choose HEVC or AV1 if supported, or turn both off to request H.264. Selecting HEVC turns AV1 off, and vice versa.
3. Set **Bitrate** and **Audio**. Use the channel layout that matches your speakers or audio receiver. Leave optional video tweaks off for the first comparison.
4. Close Settings to save, then select a game or Desktop from the PC's application list.
5. During the stream, hold **Back** on the Magic Remote, press **Red / Exit**, or use the [gamepad shortcut](#gamepad-during-streaming) to open the stream menu. Use **Left/Right** to select an action and **OK / A** to activate it; **Back / B** returns to the stream.
6. Choose **Disconnect** to return to Aurora and resume later, or **End session** to ask the host to close the application after confirmation.

Start with **60 FPS** and a moderate bitrate, then increase one setting at a time. Higher bitrate is useful only when the network, host, and decoder can sustain it. Compare 60 and 120 FPS where supported; a high-refresh panel alone does not establish support for decoding every stream at 120 FPS. The older 3584 × 2016 recommendation came from C5 experiments and is not a universal C2 preset.

### Save changes and resume

Settings are saved when you close the settings screen. Stream format and session options generally take effect on the **next connection**. If Aurora offers **Reconnect streaming**, use it to leave the current stream, then open the application again with the new settings. **Later** keeps the current session running with its existing configuration.

Changing **Virtual Mouse** in the stream menu affects the current session. The option in **Settings → Input** chooses whether it starts enabled in future sessions. **Show logs** takes effect immediately; changing language rebuilds the interface and may also prompt you to reopen Aurora.

The profile selector on the home screen recalls saved resolution, frame rate, bitrate, HDR and HEVC preferences. Those values are saved to the active profile when Settings closes. Other settings, including AV1, are shared between profiles. The **Profile** under adaptive bitrate is a separate choice, explained below.

## Settings guide

These descriptions explain what to change in Aurora and when to use it. Labels below use the English interface; translated menus have the same organization. **Stream** is the main settings list; **Input**, **Host** and **Other** open additional panels. Some controls depend on the TV, selected decoder or connected device.

### Stream: picture, sound and bitrate

| Option | What it does / when to use it |
| --- | --- |
| **Resolution** | Requests the stream's image size. Lower it to reduce the workload or compare stability. The host's desktop or game may need a matching resolution. |
| **Frame rate** | Requests the streaming FPS. Increase it only after the lower rate works reliably; available choices depend on the TV and decoder. |
| **Bitrate** | Sets the requested video data rate in Mbps. Higher values give the encoder more room for image detail, but also require more bandwidth. The decoder or host can limit the applied value. With adaptive bitrate enabled, this is the starting request, not a fixed ceiling. |
| **Video backend** | Chooses Aurora's video decoder implementation. **Auto** lets Aurora select one. Change it when comparing compatibility or investigating playback problems. |
| **HEVC / AV1** | Selects a preferred video format. Only one can be enabled at a time; AV1 is available only when the decoder advertises support. With both off, Aurora requests H.264. The host must also support the chosen format. |
| **HDR** | Requests HDR streaming. Requires HEVC or AV1 and support from the decoder and host setup. Turn it off when checking an SDR setup or investigating incorrect colors. |
| **10-bit SDR** | Requests 10-bit video while HDR is off. Requires HEVC or AV1 and host/decoder support. It does not turn HDR on. |
| **Audio** | Chooses stereo, 5.1 or 7.1 where offered. Match the channel layout supported by the entire TV/speaker/receiver path. |
| **Full range YUV (SDR only)** | Requests full-range SDR video. Use it when the host and display path expect that range; compare black and white detail after changing it. Disabled while HDR is enabled. |
| **HEVC sliced frames** | Requests that HEVC frames be encoded in smaller sections. Use it to compare HEVC delivery and latency on a compatible host; improvement is not guaranteed. Unavailable with AV1 or HEVC off. |
| **Periodic decoder refresh (HEVC)** | Periodically requests a fresh full frame. Use it to investigate recurring HEVC image corruption; each refresh can cause a brief hitch. It is off by default and has no effect with AV1. |
| **Refresh interval** | Sets the time between periodic refresh requests. Requires HEVC and periodic decoder refresh enabled. Shorter intervals request refreshes more often. |
| **Adaptive bitrate** | Adjusts bitrate during streaming as conditions change. Requires a host that accepts live bitrate adjustments. Leave it off for a fixed-bitrate comparison. |
| **Profile** under adaptive bitrate | **Balanced** uses a general adjustment range; **Quality** keeps a higher minimum and recovers upward sooner; **Low latency** allows a lower minimum and limits upward growth more. These change bitrate policy, not resolution or FPS. Requires adaptive bitrate enabled. |

**Why is an option grayed out?** Check the explanation below it. For example, enabling AV1 disables HEVC slices and periodic HEVC refresh; HDR needs HEVC or AV1; the refresh interval needs periodic refresh enabled; the bitrate profile needs adaptive bitrate enabled. A disabled control can retain its saved checkmark or value without being active in that mode.

### Input: remote, mouse and gamepad

| Option | What it does / when to use it |
| --- | --- |
| **View-only mode** | Shows the stream without forwarding your controls to the PC. Turn it off to interact with the game or desktop. |
| **Capture system keys** | Forwards supported system keys, such as Windows/Meta, from the keyboard to the PC. Availability also depends on the TV's keyboard handling. The remote's Home shortcut is handled separately. |
| **Use mouse hardware**, when available | Reads a supported mouse through the direct device path. Requires device access; unavailable builds omit this control. Enabling it disables the separate absolute mouse mode option. |
| **Absolute mouse mode** | Sends pointer positions instead of relative movement. Useful for desktop pointing; relative movement is usually needed for games that capture the mouse. |
| **Analog stick deadzone** | Ignores small movements around the stick's center. Increase it if the pointer or game moves while the stick is untouched; higher values require more movement before input begins. |
| **Virtual mouse** | Starts each stream with gamepad mouse controls enabled. The stream menu can toggle them for the current session. See the [mouse controls](#virtual-mouse-and-on-screen-keyboard). |
| **Swap ABXY buttons** | Swaps A/B and X/Y mappings, including Aurora's gamepad navigation, to suit your controller layout. |
| **Gamepad touchpad → Mode** | **Send as touchpad** forwards controller touch contacts for the host to handle. **Send as mouse** turns the touchpad into a pointer with local mouse gestures. Requires a controller with a supported touchpad. |
| **Pointer speed** | Changes touchpad pointer sensitivity in **Send as mouse** mode. It does not change analog stick sensitivity. |
| **Multi-touch gestures** | Allows multiple touch contacts. In mouse mode, this enables gestures such as two-finger scrolling; native touchpad mode forwards the supported contacts to the host. |
| **Natural scrolling** | Changes touchpad scrolling direction in mouse mode. Toggle it if scrolling feels reversed. |
| **Report battery to host** | Sends available controller battery information to a compatible host. Requires a controller that exposes its battery level. |
| **Detect connected gamepads** | Refreshes the controller list and its detected capabilities. Use it to check whether Aurora sees a connected gamepad. |
| **Test rumble** | Sends a short vibration test to the first connected gamepad. A successful send does not guarantee that the controller physically vibrated. |

### Host: PC and session behavior

| Option | What it does / when to use it |
| --- | --- |
| **Optimize for streaming (SOPS)** | Asks the host to apply its streaming optimizations. The host determines whether this request is supported and what it changes. |
| **Mute PC while streaming** | Requests that audio play through the stream without also playing locally on the PC. Turn it off if you want local PC audio too; behavior depends on the host. |
| **Quit the game when the stream ends** | Currently saved but not connected to automatic session cleanup. Use **End session** in the stream menu to close the application on the host. |
| **Resume a running game automatically** | Attempts to resume an application already running on the host when Aurora opens or returns to the foreground. Turn it off if you prefer to choose an application manually. |
| **Stats overlay on start** | Shows pinned performance stats when streaming begins. |
| **Compact stats** | Uses a slim stats bar instead of the larger performance panel. Applied when the streaming interface is created. |
| **NTSC refresh (59.94 / 119.88)**, on webOS | Uses fractional refresh targets for the corresponding 60/120 FPS choices. Use it when matching a 59.94/119.88 setup; it does not enable VRR. |

### Other: language, logs and reset

| Option | What it does / when to use it |
| --- | --- |
| **Language** | Chooses the interface language. **System Language** follows the TV/system language. |
| **Show logs** | Shows live diagnostic messages over the interface. Turn it on while reproducing a problem; turning it off hides the overlay while file logging continues. **Yellow** can also cycle its display mode. |
| **Game mode**, on supported rooted webOS setups | Tries to apply game picture/sound settings for the session and restore the previous values afterward. Appears only when the required root service is detected; it does not enable HDMI ALLM or VRR. |
| **Reset all settings to defaults** | Restores preferences after confirmation, keeping PC pairing. This also resets language and stream settings; reconnect an active stream to use the defaults. |

### Make a useful comparison

Use the same game or desktop scene, change one setting, reconnect, then compare. This makes it easier to tell whether a change helped.

| What you notice | What to try in Aurora |
| --- | --- |
| Movement breaks up or the stream stalls | Lower bitrate first. Compare a lower frame rate or resolution separately. |
| Playback works with one codec but fails with another | Return to the working codec. Check the selected video backend and the host's codec support. |
| Blacks look washed out or dark detail disappears | For SDR, compare the full-range YUV setting with the host/display range configuration. For HDR, check the HDR setup before changing SDR-only controls. |
| HEVC repeatedly develops a corrupted image | Compare periodic decoder refresh, keeping the rest of the settings unchanged. Check whether the refresh hitch is preferable to the original problem. |
| The controller is connected but input does nothing | Check **View-only mode**, then use **Detect connected gamepads**. Check whether **Virtual Mouse** is enabled if you expected normal gamepad input. |

For a reproducible problem, use **Show logs** and follow the [diagnostics guide](#diagnostics). The **?** guide also shows the app version and selected backends, which help identify the setup being tested.

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
| **Home** | Send the Windows key to the PC during an interactive stream. Outside an interactive stream, keep the TV's Home behavior. |
| **0–9** | Send Numpad 0–9 to the PC. The host's Num Lock state affects how applications interpret them. |
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

- **`aurora-crash.log`**: the last captured fatal signal, with version, PID, boot ID, UTC/monotonic seconds, signal code/sender, thread name, fault address when available, ARM PC/SP/LR registers and executable module mappings. Diagnostic v2 additionally attempts a backtrace of up to 32 instruction addresses and records glibc's fatal assertion/allocator message when available. It is capped at **64 KiB**, remains empty until a fatal signal is captured, and survives normal app restarts. It is a small text report, not a full memory dump; the backtrace can be partial or unavailable.
- **`aurora-run.state`**: a bounded marker identifying the current/last run. `running` means startup was recorded; `clean` means normal teardown completed; `crashed` means the fatal handler ran. On the next launch, `aurora.log` warns if the previous run did not finish cleanly. Match the PID and boot ID before associating a retained crash report with that run.

The fatal handler bypasses the normal logging queue and uses fixed buffers and signal-safe I/O. Optional local libunwind entry points are resolved and initialized during startup; the handler performs no symbol lookup. On ARM, it uses kernel GP registers and metadata-based unwinding, with the unsafe legacy APCS heuristic disabled. Missing libraries/symbols leave basic capture active. Missing unwind metadata stops the trace; a fault in the unwinder is reported as `backtrace_fault_signal` while preserving the original fatal signal and basic report. The glibc message is probed without a hard dependency on its private symbol and read through a preopened `/proc/self/mem` descriptor, capped at 512 printable characters. Raw stack/heap contents are not recorded. It handles SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT and SIGTRAP, then re-raises the original signal. The main thread has an alternate signal stack; worker-thread stack overflow is not guaranteed capturable. SIGKILL, power loss, termination before diagnostics initialize, storage failure and severe process corruption can prevent a report. An unclean marker alone does not establish OOM, a decoder bug or a native crash. Full core dumps remain disabled in non-Debug webOS builds; changing the terminal's `ulimit` does not enable them in the app.

After another unexpected close, download **`aurora.log`**, **`aurora-crash.log`** and **`aurora-run.state`** through Dev Manager. The startup `CrashLog v2 armed` entry confirms that text crash capture initialized, identifies its actual directory, and reports `backtrace=1` when local unwinding is available (`0` otherwise). SDL lifecycle/exit requests and normal shutdown start/end are also recorded. These diagnostics do not depend on Dev Manager's Crash Reports list or root access. Persistence still depends on writable storage; `/tmp` fallback files do not survive reboot. Preserve the matching build's debug-symbol archive for resolving instruction addresses.

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
