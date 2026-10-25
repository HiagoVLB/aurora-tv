# webOS streaming history — C5/NDL

Preserved from AGENTS.md on 2026-10-18. This document collects earlier hardware
reports and experiments; it is not a new test or a universal webOS specification.
These records concern C5/NDL and the C5 + eARC audio setup. LG C2 requires its
own validation.

See current invariants in [AGENTS.md](../AGENTS.md) and the baseline in
[CURRENT_STATE.md](CURRENT_STATE.md). HEAD in older records refers to the state
described there, not any future checkout. A new experiment should identify what
changed in the hypothesis, backend, firmware, or workload instead of repeating
the same PTS changes under another name.

Bitrate figures are observations from the tested setup, not new capability
limits. The historical 120/119.94 Hz test also does not redefine the current
59.94/119.88 Hz NTSC presets in code.

## 5.1 audio (C5 + eARC) — verified, do not invert

Three conventions exist: GFE native, Moonlight WAVE (`FL FR C LFE RL RR`), and
webOS NDL passthrough `{0,1,4,5,2,3}` (FL FR SL SR FC LFE packed as
`surroundParams=642014523`).

What is live on HEAD (same as upstream moonlight-tv; user validated original
Moonlight on their eARC setup — Aurora's client PCM + RemapPcm51 path was wrong):

| Path | What it does | Result |
|------|----------------|--------|
| `surroundParams=642014523` in `session_worker.c` | Host advertises the mapping `IsOpusPassthroughSupported()` already expects | Required. Nulling it (v1.2.0–1.2.3) forced `opus_fix` re-encode and wrong speakers. |
| NDL Opus passthrough | Opus preferred when SS4S advertises it and host mapping matches | Correct 5.1 on user's setup. Do **not** force client PCM or RemapPcm51. |

Do **not** send Sunshine `surround-params` a second time on top of `642014523`.
Do not "fix" 5.1 by deleting `surroundParams` again.

## webOS frame pacing — read before attempting

Pan microstutter on the C5 has resisted a long series of attempts. What is already
ruled out, so nobody repeats it:

| Attempt | Result |
|---------|--------|
| PTS grid snapping (`SS4S_SMOOTH_PACING`) | no change |
| Host PTS mapping (`SS4S_SMOOTH_PACING_HOST_ONLY`) | no change |
| Presentation offset (`SS4S_PRESENTATION_OFFSET_US`) | no change |
| Panel-phase clock (`SS4S_PANEL_PHASE_PACING`) | no change |
| Holding the Feed call on a frame grid ("V-Sync gate") | no change, removed |
| Future PTS / render-buffer V-Sync (`render_queue_frames`) | no change on C5 (buffer stays empty; PTS ignored); option removed — made PAN hitch worse |
| Clearing `CAPABILITY_DIRECT_SUBMIT` (decoder thread) | **worse**: decoded FPS 115–118 and real stutter; restore direct submit |
| 4K 120 @ 270 Mbps | **accumulating delay** (USB2 + decode queue). 80 Mbps 4K is stable; 120 Mbps slight drift |
| Bitrate 270 → below 100 Mbps (3.6K pan hitch) | no change |
| 120 Hz vs 119.94 Hz | no change |

Facts that matter and are easy to get wrong:

- `lowDelayMode` is set in `smp_player.c` only. The **NDL module never sets it**, so any
  reasoning that starts "Starfish runs in lowDelayMode" does not apply to the C5 path.
- `session_worker.c` forces `SS4S_SMOOTH_PACING=0` and `SS4S_PANEL_PHASE_PACING=0`, which
  makes `SS4S_NDL_webOS5_NextVideoPts` return plain wall clock and leaves the entire
  smooth-pacing/offset machinery below it unreachable. Check the env before concluding a
  code path was tested.
- The NDL v2 video API is only `Play(buf, size, pts)`, `GetRenderBufferLength`,
  `FlushRenderBuffer` and `SetFrameDropThreshold`. PTS is milliseconds, which cannot
  represent an 8.333 ms period at 120 fps exactly.
- `NDL_DirectVideoGetRenderBufferLength` is the one real observable: if the renderer holds
  no frames it cannot align anything to vsync, and presentation phase just follows arrival.

## AV1 120 Hz (C5 NDL) — verified, do not chase with PTS

NDL feeds 120 AV1 frames/s (overlay FPS is submit rate). UFOTest 120 looks identical
to 60, with stutter; HEVC 120 is clearly sharper. PTS-now, unique-ms, 8 ms future
grid, no frame-drop threshold, and an AV1 feed thread did not change that.

HEVC on the same NDL path ignores PTS and presents on arrival. AV1 honors the
decoder clock and presents at 60 Hz. That is the TV AV1 DirectVideo path, not
bitrate. Do not "fix" it by changing HEVC. For 120 fps, use HEVC.

