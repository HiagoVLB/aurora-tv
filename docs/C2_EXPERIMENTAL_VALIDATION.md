# Physical validation of reviewed options on C2

Status on 2026-10-24: **round started**. Updated IPK installed on C2; application
first reopened at **19:57 São Paulo**, with a second installation at 20:08 to
correct the HEVC refresh interval. AV1/HEVC, three ABR modes, slices, refresh,
and HDR activation were exercised on the real client. User observation of
image, audio, colors, and navigation remains pending; this round does not yet
certify stability of every option.

## Artifact and starting point

- File: `aurora.ipk` at the Aurora repository root, **1.3.3**, ARM, RelWithDebInfo.
- SHA-256: `dc2e20ccf82a56d462c980937983dcfab8c83da4e2783d26539ce5dfca584c51`.
  Second package of the round, generated at 20:08 São Paulo, 2,385,978 bytes.
- Receipt: `.cache/ipk/run-hfluctai/package-feature-audit.json`,
  `.cache/ipk/latest.json`. Other menu and all 14 packaged bundles checked,
  including `Other → Outros` in pt-BR; version/ELF/log marker passed.
- SSH access obtained through Docker at the registered endpoint. Native Mac
  process failure `errno 65` at 19:43 did not mean the TV was unavailable.
- Before updating, exported logs, settings, and pairing-file hashes. Restricted
  local backup: `.cache/experimental-physical/before-update.tar`; old package kept.
- Installation service returned `state=installed`, `update=true`.
  Installed ELF matches the package:
  `d1c5e52a694d7089824a4f1f4629444f87bf24b053ef8455df06ddf96a2bb548`.
- Launch succeeded; initial package PID 9739, corrected-timer package PID 11808,
  and PID 12641 after preference restoration. Startup identifies Aurora 1.3.3.
  Reset marker consumed; new log confirms per-installation cleanup. Crash log
  empty in captures. Pairing-file hashes identical. Temporary round settings
  restored; profiles/host list match preserved files byte for byte. AV1 changed
  to `true` between initial startup and first session; origin undetermined.
  The value in use before the matrix was preserved.
- Installation receipt/evidence:
  `.cache/experimental-physical/timer-fix-installation-receipt.json`,
  `timer-package-runtime-review.json`, `install-timer-fix.txt`. Initial install
  and ABR/slices tests preserved separately in `installation-receipt.json` and
  `first-package-runtime-review.json`. Build receipt remains distinct from
  runtime validation.

## Round data

Observed across both installations and their sessions:

| Field | Observed value |
| --- | --- |
| C2 model / firmware | OLED42C2PSA / 33.31.68; webOS SDK 10.3.1 |
| Installed Aurora / package hash | 1.3.3, IPK and ELF checked against hashes above |
| SS4S backend selected in log | Video/audio `ndl-webos5`; load/playing and first feed confirmed |
| Host/version / encoder | DESKTOP-HO7UDGJ; `serverinfo` advertises protocol `7.1.431.-1`, GFE compatibility `3.23.0.74`; actual host/encoder version pending |
| Network / audio path | TV through `wlan0`, Ethernet inactive; NDL stereo session; physical audio output pending |
| Effective codec / resolution / FPS / HDR | AV1 8-bit SDR and HEVC SDR/HDR; requested 2560×1440 at 90 FPS. Desktop often sent roughly 16 FPS; scanout unmeasured |
| Requested/applied bitrate / backend ceiling | Profile 400000 kbps; session clamped to 95000 kbps by NDL; ABR varied below that ceiling |
| Homebrew/root availability | Homebrew service absent; root SSH :22 refused; Game Mode unavailable in this installation |

Public HTTP `serverinfo` does not use client identity; its `PairStatus=0` is not
evidence of lost pairing. File copies and Aurora host status are separate
evidence. Authenticated catalog and Desktop sessions opened successfully.

Keep the same scene and other settings, changing one option at a time. Start
with a known working configuration on this TV; confirm the effective codec.
Repeat normal exit/reconnection. On failure, retain client/host logs, time, and
triggering step. Overlay submit FPS does not prove physical presentation.

## Cases and criteria

| Case | Action and required evidence | Completion criterion | Result |
| --- | --- | --- | --- |
| Language | In Other, switch pt-BR/English, navigate, close/reopen, restart app. Check fonts/full text with the real remote. | Language persists, consistent labels, no focus loss, missing glyphs, or UI-rebuild crash. | Pending |
| Logs | Toggle Show logs; exercise Live/Frozen/Off with Yellow while streaming; reopen settings. | State/capture responds to remote; frozen state survives reopening; no hang or perceptible feed regression. | Streaming with show_logs=true opened and continued feeding without a captured crash; Yellow/Live/Frozen/Off cycle and screen readability pending |
| Game Mode | Only with root/Homebrew: record initial picture/sound, enable, reconnect, switch SDR/HDR, exit. Keep application/restoration logs. | Settings with valid snapshots apply/restore; no changes without previous values; HDR does not crash. Without root, record unavailable rather than pass. | Unavailable on this C2: no Homebrew service; physical rollback untested |
| Full range SDR | Compare off/on after reconnecting with the same black/white-level image; record host negotiation/visual result. Repeat with HDR. | Preference affects compatible SDR without improper clipping; control disabled in HDR, HDR session remains limited range. | HEVC SDR with preference on and HDR session opened without feed errors/crashes; range negotiation covered by fixture. On-screen levels/colors pending |
| HEVC slices | Enable, reconnect in HEVC, retain SDP capability log. Compare same scene; select AV1, then HEVC again. | Hint matches resolved mode, no feed/decode failure; HEVC controls disabled in AV1. Host ignoring a hint does not prove latency benefit. | SDP changed 1→6 slices/frame in HEVC; feed accepted. AV1 advertised 1 slice/no RFI with preference persisted. Actual encoder slicing/physical effect unmeasured |
| HEVC refresh | Enable a known interval, reconnect, observe IDR requests/presentation. Change/disable and reconnect; repeat AV1. | Interval/off apply next session; no periodic AV1 requests. Record hitches: periodic IDR can cause them and is not proof of smoothness. | Frame-count dependency fixed; new package requested at ~2.04–2.12 s in a 2000 ms sequence, also with longer gaps; policy resets reference on received IDRs. Off/AV1: zero periodic requests in collected segments. Physical hitches pending |
| ABR | Exercise Balanced/Quality/Low latency after reconnecting. Compare known scene/network conditions, sample/decision logs, host-accepted value. Exit/start another session. | Respects host/backend; no increases without samples; no post-session adjustments, teardown hang, or stale state reuse. | Three modes adjusted host-accepted values and completed Stop, without later ABR activity in captures. 95000 ceiling respected; network/audio loss still observed. Controlled motion/quality comparison pending |
| Reset | Change options, cancel reset, check preservation. Then confirm, reopen/restart, check defaults/pairing. | Cancel preserves values; confirm updates UI/persistence; pairing survives; Back/reopen works without crashing. | Pending |
| Menus/remote | Traverse Stream/Input/Host/Other, sliders, dropdowns, popups, confirmations with actual D-pad/pointer. | Video/ABR in Stream; general settings in Other; accessible, predictable focus/scroll/Back. | Pending |

## Evidence and limits

The first package (`94dc5af2…`, ELF `ef8a0e52…`) recorded ABR/HEVC slices
integration. The second (`dc2e20cc…`, ELF `d1c5e52a…`) added only the later HEVC
timer fix and repeated refresh/off, SDR full range, HDR, AV1 gating, and final
launch with restored configuration. Six CTests and timer fixture ASan/UBSan
passed; earlier localized UI/protocol results remain in the review report.

HDR logs confirm host signaling and NDL metadata:
`transferCharacteristics=16`, `colorPrimaries=9`, `matrixCoeffs=9`.
This does not measure on-screen colors. Frame/audio loss occurred on this
network; cause not isolated. The user had not yet answered the request to
confirm image, audio, remote, and Other menu; no such results were assumed.

## Recording results

For every failure, record effective configuration, minimal reproduction, and
logs; fix and repeat the affected case and exit/reopen navigation. Observations
without effective backend/codec do not validate an option. Missing/inconclusive
results remain pending.

Local results/code fixes are in
[EXPERIMENTAL_FEATURE_REVIEW.md](EXPERIMENTAL_FEATURE_REVIEW.md). Physical
stability can only be signed off after this round is executed and reviewed.
