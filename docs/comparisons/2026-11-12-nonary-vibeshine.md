# Nonary moonlight-qt and VibeShine review — 2026-11-12

Analysis first; implementation status is in the "Decisions" section at the end
and in [CURRENT_STATE.md](../CURRENT_STATE.md).

## References read

| Project | Commit read | Previous baseline | New commits (no merges) |
| --- | --- | --- | --- |
| Nonary/moonlight-qt | `19758e5e` (2026-11-08), isolated clone `.cache/references/moonlight-qt` | `3ae5f3b0` (2026-10-18) | 38 |
| Nonary/VibeShine | `9c02f262` (2026-11-02), isolated clone `.cache/references/vibeshine` | `97835dbf` (2026-10-18) | 298 |
| Aurora | branch `upstream-review-2026-11-11`, `0f37dd2b` + uncommitted Home-disconnect fix | | |

Remote default branches were fetched on 2026-11-12. The sibling checkout
`../vibeshine` is older (`af1a0842`, dirty submodules) and was not used.
The host at `192.168.0.70` answers plain `/serverinfo` with
`ServerCodecModeSupport=0x7830301` (H.264, HEVC, HEVC Main10, AV1 Main8/Main10) and
`HardwareMetricsVersion=0`; its exact VibeShine build is not exposed there.

## Worth doing (client side)

| Idea | Source | Aurora today | Cost / limits |
| --- | --- | --- | --- |
| **Put PC to sleep** from the launcher | VibeShine `9bf0802a`, `docs/power-actions.md` (`GET /power-actions/v1`, `POST /power-actions/v1/sleep?token=`); Nonary client `696afb07` | Has Wake-on-LAN, no sleep | Paired HTTPS with client cert (libgamestream already does it). Host grant `allow_sleep` is off by default per device. Host refuses while a game is retained for resume, so the user must End session first. One confirmation per attempt, never auto-retry. Windows host only. |
| **Show host warnings** (`clientWarning`, `clientWarningMessage`) | VibeShine `e261a059` (`virtual_display_disabled_by_client`); Nonary `d54e6c04` | Ignored | Small: parse from launch/resume XML, show once as a notice; never change whether the stream starts. VibeShine `9c02f262` sends only the code, not the message. |
| **Host CPU/GPU/encoder load in the stats overlay** | VibeShine `5745ca5c`, `docs/hardware-metrics.md` (`GET /api/hardware-metrics`, opt-in `hardware_metrics`) | Client-side stats only | Poll ≥ `interval_ms` off the video path, one request in flight; stop on 403/409. Useful to tell a host encoder stall from a TV problem (the 2026-11-11 crash was a host encoder failure). |

## Configuration or testing only (no Aurora code)

- **AV1 at 90 fps.** The C5 history ([WEBOS_STREAMING_HISTORY.md](../WEBOS_STREAMING_HISTORY.md))
  shows NDL presenting AV1 at 60 Hz while HEVC presents every frame. The C2
  stream on 2026-11-12 was AV1 8-bit 2560×1440@90. Not measured on C2: compare
  HEVC and AV1 at 90 fps (UFO test) before deciding the default.
- **Update VibeShine on the PC.** Host-side fixes since the baseline that match
  problems seen here: `73a00e80` (retry recovery IDRs after reference
  invalidation), `40c5f841` (NVENC ends the stream when output picture identity is
  lost instead of sending a broken stream), `4c5c58de` (requested keyframe not
  delayed behind a repeated frame), `fbc8f6e3` (flush graceful disconnect).
- **Host adaptive packet pacing** (`07366c50`, `host_adaptive_pacing`, default off,
  experimental). Aurora's moonlight-common-c already sends `SS_FRAME_FEC_STATUS`
  (`RtpVideoQueue.c`), which this controller consumes. Only try it if Wi-Fi/packet
  loss shows up in the stats.

## Not worth it here

- **Dolby AC-3 / E-AC-3 audio** (VibeShine `a60d9995`, `x-ss-audio-codec-offer` /
  `x-ss-audio-codec-accept`, `X-SS-Audio-Codec`): needs moonlight-common-c changes,
  a new SS4S/NDL audio type and testing on the eARC path that already carries Opus
  5.1. AC-3 frames are 1536 samples (32 ms), which adds latency compared with Opus.
- **Nonary VRR buffer/pacing work** (most of the 38 commits): desktop presenters;
  NDL has no VRR control (see section "Gaps and pending decisions").
- **DualSense speaker PCM, Steam Controller 2026, Guide-key mapping**: not in use.
- **Bitrate/FEC overlay** (`fc395b1b`): reverted upstream (`04ad823f`).

## Aurora gaps still open (from CURRENT_STATE)

`quitappafter` has no teardown consumer; the UI allows 400 Mbps without showing
the applied ceiling; a host terminate/replace confirmation still shows as a
generic error. Home during a stream no longer leaves NDL running behind the LG
home (fix 2026-11-12, see CURRENT_STATE).

## Decisions (2026-11-12, user)

| Item | Decision / status |
| --- | --- |
| Host warnings | **Implemented**, installed on C2 2026-11-12: parsed in libgamestream, shown 12 s in the stream notice. Fixture-tested; not seen on C2 (current host sends the code only when this device's virtual display is off). |
| Host CPU/GPU load in stats | **Implemented**, installed on C2 2026-11-12: "PC load" row and compact "PC C/G/E/°" segment. Parser fixture-tested; needs host `hardware_metrics = enabled` (default off; the host answered `HardwareMetricsVersion=0`). |
| AV1 at 90 fps | Already selectable; nothing to build. The TV is a C2 (`OLED42C2PSA`); whether it shows 90 distinct AV1 frames is unmeasured (C5 showed 60). |
| Dolby AC-3/E-AC-3 as an option | **Not possible on the C2's current path** (confirmed on the TV: `libNDL_directmedia_impl.so.1` audio strings name only Opus, PCM, MP3). **Blocked on the TV API.** The C2 path is `ndl-webos5` (NDL DirectMedia). The known webOS 5 header (`third_party/ss4s/mocks/include/libndl-media/NDL_directmedia_types.h`) lists only `NDL_AUDIO_TYPE_PCM`, `MP3`, `OPUS`; AC-3 exists only in the older webOS 4 DirectAudio module. The community headers in [webosbrew/webos-userland](https://github.com/webosbrew/webos-userland) (`00a5f87`, read 2026-11-12) agree: `include/libndl-media/NDL_directmedia_types.h` has only PCM/MP3/OPUS for DirectMedia v2, while the separate ESPlayer API (`include/ndl-directmedia2/media-types.h`) has `NDL_ESP_AUDIO_CODEC_AC3`/`EAC3` but no AV1 video and no Opus, so it cannot replace the current path. Next step: check the TV's own `libNDL_directmedia` for an AC-3 audio type before writing the moonlight-common-c `x-ss-audio-codec-accept` patch. Firmware images (EPK v3) need keys dumped from a TV to decrypt with epk2extract, so reading the library on the C2 itself is the practical source. |
| DualSense | Rumble, trigger rumble, adaptive triggers (with fallback), motion sensors, RGB LED, touchpad and battery are already forwarded (`session_gamepad.c`, `input_gamepad.c`, `session_connnection.c`). Nonary's new speaker PCM needs raw HID audio reports to the controller, which SDL on the TV's Bluetooth stack does not expose; not planned. |
