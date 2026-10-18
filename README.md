# Aurora

Unofficial fork of [Moonlight TV](https://github.com/mariotaku/moonlight-tv) for **LG webOS** (C1–C5 and compatible sets), focused on high-quality streaming on OLED TVs with a remote- and gamepad-friendly UI.

> Rights to the original project belong to [mariotaku/moonlight-tv](https://github.com/mariotaku/moonlight-tv) and the Moonlight community. Provided without warranty.

## Highlights

- **AMOLED layout** — pure black background, dark surfaces, violet accent.
- **3.6K (3584×2016)** recommended on LG C5 (stable quality without native-4K cumulative delay); **4K** when the set handles it.
- **HDR10 (PQ)** over HEVC Main10 (when supported).
- Bitrate slider up to **300 Mbps**; above **250 Mbps** there is usually no visible gain and packet loss becomes more likely as the link becomes less stable.
- **Performance stats overlay**, **full on-screen keyboard**, and **virtual mouse** during streaming.
- **Redesigned settings** — row-based D-pad navigation, fixed combobox/checkbox remote handling, modal Host/Input/Experimental panes.
- **On-screen log overlay** (Experimental) — punktfunk-style live log tail; Magic Remote **Yellow** cycles Live → Frozen → Off.
- **Rooted game mode** (Experimental, rooted TVs) — TV picture/sound and motion/energy pack tuned for streaming; restored on exit.
- **Select server** popup uses home-screen style app tiles instead of a plain list.

## Screenshots

| Home | Settings |
|:---:|:---:|
| ![Home screen](docs/images/home.png) | ![Basic settings](docs/images/settings.png) |

| Performance stats | On-screen keyboard |
|:---:|:---:|
| ![Performance stats](docs/images/performance-stats.png) | ![On-screen keyboard](docs/images/keyboard.png) |

![Streaming with keyboard and stats](docs/images/keyboard-streaming.png)

## Quick start

| Setting | Suggestion |
|---------|------------|
| Resolution | **3.6K** (3584×2016) on LG C5; **4K** if your set is stable at native 4K |
| FPS | 60 or 120 |
| Codec | HEVC (H.265) |
| Bitrate | Start at **120–180 Mbps**; stay at or below **250 Mbps** for a stable link. Higher values rarely help and can increase packet loss. |

## Streaming controls

**Open overlay:** Magic Remote **RED** / **EXIT**, or gamepad **LB + RB + Back + Start** (hold, then release).

| Gesture | Action |
|---------|--------|
| **Hold Select/Back 4s** | Toggle pinned performance stats |
| **Y / Triangle** (virtual mouse on) | Open on-screen keyboard |

Full keyboard: stream overlay, Magic Remote **BLUE**, or gamepad **Y** while virtual mouse is active. With the keyboard open: **Y** = Space, **LT** = abc/`&123`, **LB/RB** = Left/Right. Virtual mouse: stream overlay button (or enable in Settings → Input to start enabled); right stick = cursor, left stick = scroll, LT/RT = mouse buttons.

Details, hotkey layout, and stats field reference: [webOS build guide](docs/BUILD_WEBOS.md).

## Install

- [webOS Homebrew Channel](https://github.com/webosbrew/webos-homebrew-channel) — repo: `https://raw.githubusercontent.com/GuiDev1994/aurora-tv/main/repo.json`
- [Device Manager](https://github.com/webosbrew/dev-manager-desktop) — install the latest `.ipk` from [Releases](https://github.com/GuiDev1994/aurora-tv/releases)
- [webOS TV CLI](https://webostv.developer.lge.com/develop/tools/cli-installation) — `ares-install com.aurora.gamestream_*_arm.ipk` ([build guide](docs/BUILD_WEBOS.md))

## Build from source (developers)

Cross-compile Aurora for webOS using Docker. **Prerequisites:** Docker Desktop (Windows/macOS) or Docker Engine (Linux).

**Windows (PowerShell):**

```powershell
.\scripts\webos\build_with_docker.ps1
```

**Linux / macOS (Docker):**

```bash
docker run --rm \
  --dns 8.8.8.8 --dns 1.1.1.1 \
  -v "$(pwd):/build" \
  -v "$(pwd)/scripts/webos/docker_build_inner.sh:/docker_build.sh" \
  -w /build -e CI=1 -e DOCKER_SKIP_SUBMODULES=1 \
  ubuntu:22.04 \
  bash -c "sed 's/\r$//' /docker_build.sh | bash"
```

The `.ipk` is written to `dist/com.aurora.gamestream_<version>_arm.ipk`. To generate a Homebrew manifest locally (optional), install `webosbrew-gen-manifest` once; official [releases](https://github.com/GuiDev1994/aurora-tv/releases) build and publish the manifest via GitHub Actions.

Full build, install, and troubleshooting guide: [docs/BUILD_WEBOS.md](docs/BUILD_WEBOS.md).

## Credits

- Base: [mariotaku/moonlight-tv](https://github.com/mariotaku/moonlight-tv)
- Components: [moonlight-embedded](https://github.com/irtimmer/moonlight-embedded), [moonlight-common-c](https://github.com/moonlight-stream/moonlight-common-c)

## License

This project is licensed under the [GNU General Public License v3.0](LICENSE) (GPL-3.0-or-later).

Copyright and attribution details are in [COPYRIGHT](COPYRIGHT).

Aurora is a fork of [moonlight-tv](https://github.com/mariotaku/moonlight-tv), which is also licensed under GPL-3.0.


### Diagnostic log on the TV

Aurora automatically appends INFO, warning and error messages to **`aurora.log`**
in its configuration directory, even when **Show logs** is off. On webOS this is
`$HOME/conf/aurora.log`, where `$HOME` is the application's runtime home (not the
SSH account's home). It is the same `conf` directory as Aurora's settings. For a
standard Developer Mode installation, look in
`/media/developer/apps/usr/palm/applications/com.aurora.gamestream/conf/aurora.log`
using webOS Dev Manager's file browser. The startup `FileLog` message reports the
actual path and whether storage is persistent. If that directory or the log file is not writable,
the existing configuration-path fallback is `/tmp/moonlight-tv/conf/aurora.log`;
that fallback does not survive a TV reboot.

The single file is capped at **2 MiB (2,097,152 bytes)**. At the limit, Aurora keeps
approximately the newest 1 MiB of complete lines in place, then continues writing
to the same file. It does not create dated files or `.1` backups, and restarting
Aurora preserves recent history. Download the file after reproducing the issue,
preferably after leaving the stream so pending records have time to be written.

Records include UTC timestamps for comparison with host logs and monotonic
timestamps for comparison with the on-screen overlay. Diagnostics cover:

- Version, stream configuration, codec/backend and existing application errors.
- Video samples every two seconds: receive/submit FPS, frame-number gaps, maximum
  time inside the player feed call, NOT_READY/error counts and sample age. A
  repeated sample with growing `age_ms` means no new statistics were published;
  `feed_active=1` with growing `feed_elapsed_ms` points to a player feed call that
  has not returned. `have_submit=1` with growing `last_submit_age_ms` means the
  video submit callback has not been entered recently. `sample_valid=0` means
  the statistics publisher was busy; the diagnostic skips waiting on the UI thread;
  repeated FPS values alone do not prove the stream is still receiving frames.
- ABR loss estimates, RTT, target/allowed bitrates and request failures.
- Interruption origin (`user`, `host`, `network`, `decoder`, `watchdog`, etc.) and
  progress/timing through connection stop, app quit and cleanup.

`player_latency_est_ms` is the player's estimate, not a measured decoder execution
time. `frame_gaps` can include frames lost before submission and should not be
interpreted as a definitive count of packets lost on the network.

Disk writes and trimming run on a separate thread with a bounded queue. If the
queue fills, records are dropped and a counter is written; video reception does
not wait for disk I/O. DEBUG/VERBOSE records (which can contain request URLs and
credentials) are excluded. The worker has a 128 KiB stack and its file descriptor
is closed on exec. Normal shutdown drains the queue; abrupt termination
can lose queued records. This log does not replace a native crash stack trace.

The writer has a standalone test registered as `test_file_log` in CTest. It can
also be checked without the complete app dependencies:

```sh
cc -std=c11 -D_POSIX_C_SOURCE=200809L -DFILE_LOG_MAX_BYTES=4096 \
  -Wall -Wextra -Werror -pthread -fsanitize=address,undefined \
  -Isrc/app -Ithird_party/commons/util/logging \
  tests/app/test_file_log.c src/app/util/file_log.c -o /tmp/aurora-test-file-log
/tmp/aurora-test-file-log
```

The webOS compatibility audit and upstream comparison are recorded in
[Diagnostic log review](docs/DIAGNOSTIC_LOG_REVIEW.md).
