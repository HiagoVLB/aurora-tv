# Experimental options review — 2026-10-24

Scope: all options formerly in the Experimental pane, their consumers,
persistence, lifecycle, and organization. Reviewed the working tree while
preserving existing HTTP, diagnostic, and SS4S changes. Central version unchanged.

## Organization and behavior

| Option | Current location | Review |
| --- | --- | --- |
| Language | Other, in builds with a selector | Reapplying and rebuilding fragments preserves display/input devices. Catalogs and bundles checked; webOS selector still requires TV testing. |
| Show logs | Other | Still immediate. Producer/UI signals use atomics; filter/ring access is serialized. Reopening settings does not unfreeze capture. |
| Game Mode | Other, only when Homebrew/root is detected | Preference copied into the session. Previous values and rollback memory are obtained before writes; failure/timeout preserves the snapshot. HDR is applied by the worker without sharing a state pointer with callbacks. |
| Full range YUV | Stream → Video | Consumer remains SDR-only. UI disables it with effective HDR; a test checks resulting colorRange. |
| HEVC sliced frames | Stream → Video | SDP capabilities derive from effective session configuration, including resolved resolution/FPS. UI requires HEVC support and disables it with AV1 selected. It remains a host hint. |
| Periodic HEVC refresh and interval | Stream → Video | Value copied into the session, with no mutable preference reads from the video thread. Interval uses monotonic milliseconds independently of actual host FPS. Both toggle directions announce reconnection. Interval/state persist; slider depends on codec/toggle. |
| Adaptive bitrate and mode | Stream → Bitrate | One decision per new sample containing frames; absent/repeated stats do not trigger increases. Host/backend clamp, stop-before-transport, and backoff preserved. Deadlines tolerate clock wrap; proportional arithmetic avoids overflow. |
| Restore defaults | Other → Reset | Frees old strings, preserves directory/persistence and pairing files, updates UI. Dialog is destroyed with its fragment; confirmed/cancelled reset exercised. |

INI preferences retain their names, defaults, and previous global scope; no
destructive migration or incidental profile expansion. Language/logs need no
reconnection; video, ABR, and Game Mode apply next session. Normal resolution,
FPS, bitrate, codec, and HDR changes also announce reconnection.

## Failures reproduced and fixed during validation

- UBSan found an uninitialized `fallback` array in FPS options. Initialized it
  and fixed dropdown selection with fallback index `-1`.
- ASan found a settings-container read after free. Cleanup moved to
  `obj_will_delete_cb`, with explicit Stream-fragment ownership: submenu
  `user_data` stores classes rather than fragments.
- Back could free the focused object before LVGL delivered CANCEL after KEY.
  Closing is deferred until dispatch ends, cancelled only while pending
  (a running callback does not cancel/free itself).
- Confirmed reset reproduced a crash while replacing display/input during UI
  update. Rebuilding now preserves existing display/input.
- The simulator could click the Reset heading behind the dialog with the same
  name. Search traverses objects front to back; the scenario requires the
  confirmation to disappear. Saved states/checkboxes were checked alongside actions.
- A fixture reproduced invalid ABR JSON with decimal-comma locales. Feedback
  now uses a locale-independent dot and rejects NaN/Inf. The fractional FPS
  editor also parses its accepted dot independently of LC_NUMERIC, preserving
  59.94/119.88.
- Luna backend double-freed a reply in the `getConfigs` fallback. Fixed;
  restoration serializes JSON values and escapes apostrophes in the argument
  sent to the Homebrew shell.
- During real HEVC testing, a 2000 ms refresh still counted
  `negotiated FPS × interval` frames. Desktop sent roughly 16 FPS in a 90 FPS
  negotiated session, extending the interval. Policy now measures monotonic
  time since the last IDR/request, resets per session, throttles repeated
  requests, and tolerates clock wrap. HEVC-only gating and default-off preserved.

## Local verification

- CTest: `test_settings`, `test_game_mode`, `test_adaptive_bitrate`,
  `test_session_options`, `test_fractional_fps`, `test_idr_refresh` passed.
  Coverage includes round-trip/reset/pairing; picture→sound failures/side effects;
  repeated HDR; timeout after write; loss/backoff/clamp/overflow; snapshots and
  HEVC/AV1/SDR/HDR capabilities; IDR intervals with variable FPS/pauses/wrap;
  large INI values.
- Policy fixtures ran with AddressSanitizer + UndefinedBehaviorSanitizer.
- Real SDL/LVGL UI: English + 16 catalogs, **76 steps per language**, 1920×1080
  and 1280×720, in `.cache/experimental-visual/`. Other and Stream-control
  captures opened to inspect layout/glyphs.
- `tools/ui-sim/stream-options.json`: **35 steps at each resolution**, ASan/UBSan
  and dummy backend, in `.cache/experimental-final-ui/`. Covers D-pad/mouse,
  slider, reopening, HDR/AV1 gating, ABR, logs, real reset. HEVC/AV1/HDR are
  explicit synthetic capabilities of this UI fixture.
- Full pt-BR scenario repeated with ASan/UBSan after lifecycle fixes, at both
  resolutions, in the same final UI directory.
- `tests/core/run_vibeshine_protocol.sh` passed with ASan/UBSan, including pt-BR
  decimal payload and NaN/Inf rejection.
- Gettext auditor + real AWK conversion to `cstrings.json`: **16 catalogs passed**,
  no missing/fuzzy/format errors or bundle differences;
  `.cache/experimental-i18n-audit.json`. Pane rename translated in every catalog.
- ARM webOS executable cross-build passed; verification logs:
  `.cache/experimental-checks-complete.log`, `.cache/experimental-webos-complete.log`.

## Requirements for physical stability sign-off

Renaming the pane to Other organizes the menu; it does not certify firmware or
hardware. The current 1.3.3 package was generated on 2026-10-24 at 20:08 São Paulo,
SHA-256 `dc2e20ccf82a56d462c980937983dcfab8c83da4e2783d26539ce5dfca584c51`.
ELF/identity/log marker, new menu, and 14 bundles checked. Installed on
OLED42C2PSA (firmware 33.31.68, webOS 10.3.1), installed ELF identical to package.
Pairing preserved; logs reset per installation. pt-BR startup and NDL sessions
observed; exercised cases detailed below. Scanout, physical audio, SDR/HDR color
calibration, and real navigation still require observation/controlled comparison.
Game Mode is unavailable on this C2 without Homebrew; physical rollback untested.
UI testing excludes the webOS-specific language selector. `uk`/`zh-TW` were
checked as existing catalogs without enabling them in the product.

Periodic refresh remains default-off: IDR requests can inherently cause hitches
and should be used for HEVC diagnostics. Slices depend on the host encoder.
C5 history does not validate C2; PTS, pacing, decoder caps, 5.1 mapping, and host
termination policy were unchanged.

Next evidence: physical observation in a controlled C2 round, with a known scene,
effective configuration recorded, and one option per comparison. Language/log/reset
with the real remote and rooted firmware restoration remain untested. Without
this evidence, there is no basis for declaring every option fully stable.

The [C2 checklist](C2_EXPERIMENTAL_VALIDATION.md) records the current artifact,
criteria, and partial results. Docker access overcame the Mac's native routing
failure. Both installation receipts and streaming captures are in
`.cache/experimental-physical/`; a build receipt does not replace runtime results.

## First C2 runtime round

The first reviewed package (`94dc5af2…`) opened authenticated Desktop in AV1 and
HEVC, requested SDR 2560×1440 at 90 FPS, and NDL stereo audio. The profile
requested 400 Mbps; the session applied the backend's 95 Mbps ceiling. Desktop
traffic was around 16 FPS; this does not establish scanout, motion, or physical
latency. Packet loss/audio gaps occurred without NDL feed errors or crashes in
the collected files; cause undetermined.

All three ABR modes applied real host adjustments: Balanced 95000→66500→69825,
Quality 95000→66500→47500→49875, Low latency down to 16764 kbps. All three exits
logged `Stop begin restore=0` then `Stop complete`, with no later ABR activity
in the collected segment. HEVC slices changed SDP advertisement from 1 to 6
and feed remained accepted; latency benefit and actual encoder slicing were
not established. Refresh testing exposed the frame-count defect fixed above.
Temporary settings/profiles were restored.

Round records: `.cache/experimental-physical/first-package-runtime-review.json`
and its listed case files. Physical observation, SDR/HDR colors, actual
language/log/reset controls, and Game Mode rollback remain pending.

## Interval fix validated on the real client

Second 1.3.3 IPK generated at 20:08 São Paulo, SHA-256
`dc2e20ccf82a56d462c980937983dcfab8c83da4e2783d26539ce5dfca584c51`,
installed and ELF checked (`d1c5e52a…`). The 2000 ms sequence recorded requests
roughly every 2.04–2.12 s across several segments. Receiving an IDR resets the
reference, so request gaps can also be longer. Refresh off and AV1 (even with
2000 ms persisted) emitted no periodic requests. AV1 also advertised only
1 slice/no RFI with sliced frames persisted; enabled overlay caused no crash
in the captured session.

HEVC SDR with full range persisted and HEVC HDR opened with accepted feed,
without crashes. Host signaled HDR and NDL received PQ/BT.2020 (transfer=16,
primaries/matrix=9). Color/level calibration and physical image/audio/remote
confirmation remain pending. TV uses `wlan0`; frame/audio loss recorded with
cause not isolated. Round preferences restored; profiles/host list match byte
for byte, pairing hashes unchanged. Data:
`.cache/experimental-physical/timer-package-runtime-review.json` and second
installation receipt. No commit, push, or version increment was performed.
