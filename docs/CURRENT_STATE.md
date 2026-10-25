# Aurora current state

Baseline reviewed on **2026-10-25**, São Paulo time. This is the maintained entry
point for project status and future comparisons. Detailed reports remain in
[VibeShine integration](VIBESHINE_INTEGRATION.md),
[Nonary/webOS comparison](NONARY_MOONLIGHT_WEBOS_REVIEW.md), and
[log investigation](DIAGNOSTIC_LOG_REVIEW.md).

## Documentation and repository cleanup (2026-10-25)

Technical documentation, repository instructions, bundled skill instructions/UI
metadata, and remaining Portuguese source/build comments were standardized in
English. Application translation catalogs and native language names were
preserved. Historical validation below remains historical evidence, not checks
rerun during this documentation task.

Removed obsolete implementation plans/specs under `docs/superpowers/` and the
standalone `idr-throttle-input-latency-v1.7.3.patch` (corrupted text and obsolete
capability assumptions), two unused branding previews, and a duplicate logo
identical to the retained splash source. Removed three obsolete WSL/SDK helpers
(`run_wsl_build.py`, `wsl_build_once.sh`, `fetch_sdk.sh`) tied to a former
machine's absolute paths, plus two ignored root build logs.
Current behavior, experiments, and limitations remain
in the maintained reports. Game Mode source documentation now points to the
experimental-options review. Ignore rules cover future scratch patches,
implementation-plan output, editor backups, and diagnostic logs. Necessary build
patches, fixtures, generated resource headers used by builds, and existing local
SS4S changes are preserved.

Validation for this cleanup: 82 local Markdown links across 22 files passed;
all historical commit/package hashes in translated reports were retained; five
skill schemas/metadata passed validation; eight disposable and six required-path
ignore cases passed. Nine release-helper tests, version consistency (1.3.3),
maintained Bash entry-point syntax, and `git diff --check` passed. No IPK build,
TV installation, or physical streaming test was performed for this cleanup.

## Version 1.3.5 (2026-10-25)

At the user's request, bumped the central source version from **1.3.4** to
**1.3.5** for the selective upstream stability fixes below. The version contract
check confirms appinfo, `APP_VERSION` and the ELF marker derive from that
central value. Build/release helper tests passed after the bump; the earlier
compile, sanitizer and UI results below were obtained before this version-only
change. No new IPK, tag, release or TV installation was requested or produced.
Pre-existing dirty SS4S source edits remain local and outside this commit.

## Selective Aurora upstream fixes (2026-10-25)

Compared local `bb693bf9` with freshly fetched GuiDev1994/Aurora `main`
`916f3810` (v1.3.2). Ported the nanors packet-recovery buffer-overrun fix
(moonlight-common-c `9ae7cbb0` → `b86ffb75`, exactly one commit), webOS
translation cache/lifetime fix, main-thread connection notices, quit-worker
server snapshot/client cleanup, SDL/NDL window-ID self-copy guard, embedded
settings Close/Back fixes, and CMake argument quoting. The review used local
version 1.3.4; the subsequent version bump is recorded above. Existing dirty
SS4S patch is preserved byte-for-byte; audio, pacing,
protocol/session policy, remote mappings, settings migration, SDK and theme
changes were not imported.

Linux Debug and webOS ARM RelWithDebInfo builds passed. Four new CTests passed;
packet-recovery and translation regressions reproduced against old code and
passed ASan/UBSan/leak checks after the port. Full CTest passed 35/37; existing
`test_pref_fps`/`test_pref_res` fixture failures also reproduce with the pre-port
settings controller. Ten Python helper tests passed. No new IPK or TV deployment.
The SDL/LVGL simulator passed 124 steps in English/pt-BR at 1080p/720p for Close,
dropdown Back, pane dismissal and reopening; captures inspected.
Commit selection, deferred changes, validation and limits are recorded in the
[upstream review](UPSTREAM_REVIEW_2026-10-25.md).

## Identity of the inspected version

The following identities record the original comparison/automation inspection;
they do not identify every later working-tree change described in this file.

| Project | Local state used |
| --- | --- |
| Aurora | `d208961e43e76a7cabebad3ab8cfd1f1a443c88a`, **with uncommitted IPK/version automation changes**. SHA alone does not represent the implementation described here. |
| Nonary/VibeShine | `97835dbfb14faafd8b4f13d98bc00cc921d45296`, checkout `../vibeshine`, clean at inspection. Origin: `https://github.com/Nonary/vibeshine`. |
| Nonary/moonlight-qt | `3ae5f3b0e8dfb7538422f2920d38126385d9efb4`, clean checkout examined during the review. Origin: `https://github.com/Nonary/moonlight-qt`. Copy was in `/tmp`; that path is not a permanent dependency. |

Reference SHAs identify the code read. Remote HEAD was not rechecked during skill
creation; these are not guaranteed latest versions. `CMakeLists.txt` declares
**1.3.5**; that alone does not prove which IPK is installed. User target:
**LG C2**. Historical **C5 + eARC** evidence in
[streaming history](WEBOS_STREAMING_HISTORY.md) does not validate C2.

Relevant direct submodules:

| Dependency | Commit |
| --- | --- |
| moonlight-common-c | `9ae7cbb05f6f0fbed516043c9a0a3231b217d8ab` |
| SS4S | `49918fc58f0f78fd342ed6e891172a2b401a9a2c` |
| commons | `e778249c460bbf52f943e41c7e505823d9932508` |
| LVGL | `185ea1fc61dd01fac61867d2d6b56892e80c6058` |

Baseline includes VibeShine protocol, ABR/teardown, catalog/overlay,
navigation/settings, discovery, tests/UI simulator. Automation changes generate
IPK/version updates and derive the ELF marker from APP_VERSION. Not all areas
have hardware validation; do not assume release readiness. Earlier comparison
reviews did not change streaming.

## Option usability and TV remote (2026-10-25)

- Stream keeps conditional controls visible and explains requirements alongside
  them: HEVC/AV1 exclusion, HDR/10-bit SDR, SDR YUV, HEVC-only slices/refresh,
  refresh interval, adaptive bitrate profile. AV1/HDR also explain backend
  unavailability. Saved values/effective session policy unchanged.
- Session menu highlights focused actions with 6.25% enlargement, brighter
  background, white outline, including pointer focus. Reopening starts at
  Full keyboard. Four shortcut colors and End session confirmation still match
  existing actions.
- Remote Home sends left Windows press/release in interactive sessions, opening
  host Start. Idle/view-only retain TV Home; physical/virtual keyboard Home keeps
  navigation semantics. Digits 0–9 through webOS SDL become Numpad 0–9. USB
  keyboards captured through evdev keep physical mapping; SDL fallback cannot
  distinguish remote digits from uncaptured keyboards. Result also depends on
  host Num Lock. Help updated for both shortcuts.
- Ten new strings translated in all 16 existing catalogs without enabling
  languages. Gettext/cstrings audit passed, no missing/empty/fuzzy entries.
  Linux build and CTests `test_settings`, `test_session_options`,
  `test_remote_home` passed. UI passed 34 runs/986 steps: English + 16 catalogs,
  1080p/720p, text/menu capture inspection. `uk`/`zh-TW` remain inactive in the
  product, loaded only in simulator. Pointer focus and no-AV1/HDR backend notes
  also passed in English/pt-BR. Evidence: `.cache/usability/`,
  `ui/matrix-all.json`, `ui/contacts/`. No IPK generated/installed for this change.
  Home, Numpad, physical C2 presentation still require TV checks.
- README expanded with pairing, first session, navigation, settings/profiles,
  accessible Stream/Input/Host/Other option tables. Explains dependencies,
  practical usage, issue comparisons without extensive HDR/codec theory.
  Labels/consumers checked in code; local links, anchors, Markdown checked.
- Documented issue: `quitappafter` is displayed/persisted but has no teardown
  consumer. **Quit the game when the stream ends** does not implement automatic
  closing; explicit **End session** remains the way to request host `/cancel`.

## Experimental options and menus review (2026-10-24)

- All former Experimental options traced to persistence/consumer. Video/HEVC
  refresh/ABR are in **Stream**; language/logs/Game Mode/reset in **Other**
  (pt-BR: Outros), translated in all 16 catalogs.
- Fixed fragment ownership/cleanup, uninitialized FPS memory, reset/language UI
  rebuild, preference cleanup on reset, session snapshots, repeated ABR samples,
  Game Mode rollback/HDR. Details/limits:
  [EXPERIMENTAL_FEATURE_REVIEW.md](EXPERIMENTAL_FEATURE_REVIEW.md).
- CTest and ASan/UBSan fixtures passed; localized UI English + 16 languages at
  1080p/720p, actual options/reset scenario with sanitizers at both resolutions,
  all 16 gettext/cstrings catalogs, ARM compilation passed. Test IPK built/audited,
  installed on C2; startup checked at 19:57, pairing hashes preserved, NDL
  video/audio. Streaming cases remained pending at that point. Per-option criteria:
  [C2_EXPERIMENTAL_VALIDATION.md](C2_EXPERIMENTAL_VALIDATION.md).
- Real round opened Desktop in AV1/HEVC, requested SDR 2560×1440/90, NDL stereo,
  applied 95 Mbps ceiling. Three ABR modes applied host-accepted adjustments and
  completed stop; HEVC slices changed advertisement from 1 to 6. Packet loss/audio
  gaps observed; no crash/feed error in captured segments. Physical presentation
  remains unvalidated.
- Found/fixed frame-count-based HEVC refresh: now monotonic time, including
  variable FPS/pauses, without FPS multiplication overflow. Six relevant CTests
  and new ASan/UBSan fixture passed; new IPK built/installed at 20:08. On C2,
  periodic requests followed elapsed time; off/AV1 produced none. SDR full range
  and HEVC HDR opened without feed error/crash; host HDR signaling and NDL
  PQ/BT.2020 metadata confirmed. Physical colors/audio pending. Temporary
  preferences restored; profiles/hosts/pairing preserved.
- Physical stability remains pending: dummy backend proves neither decoding,
  colors/HDR, nor firmware permissions/restoration. Periodic refresh remains
  optional/default-off and can cause IDR hitches.

## IPK generation and versioning

- **2026-10-25**: central version changed from **1.3.3** to **1.3.4** at the
  user's request, together with the documentation/repository cleanup. Version
  helper verified appinfo/APP_VERSION/ELF-marker derivation; all nine
  release-helper tests passed. Existing IPK artifacts retain their original
  version until rebuilt. This version change did not build an IPK or install
  anything on the TV.

- **2026-10-24, 20:08 São Paulo**: **1.3.3** IPK regenerated with HEVC timer fix,
  **2,385,978 bytes**, SHA-256
  `dc2e20ccf82a56d462c980937983dcfab8c83da4e2783d26539ce5dfca584c51`.
  Receipt `.cache/ipk/run-hfluctai/`. Installed on C2; ELF verified:
  `d1c5e52a694d7089824a4f1f4629444f87bf24b053ef8455df06ddf96a2bb548`.
  Per-installation log reset observed; previous copy kept. Other menu and 14
  bundles match prior audit. Refresh, SDR/HDR, AV1 gating exercised on client;
  physical image/audio/remote and actual UI observations pending. Evidence:
  `.cache/experimental-physical/timer-fix-installation-receipt.json`,
  `timer-package-runtime-review.json`. Round configuration restored.
- **2026-10-24, 19:42 São Paulo**: **1.3.3** regenerated with Experimental/Other
  review fixes, central version unchanged. `aurora.ipk`, **2,386,028 bytes**,
  SHA-256 `94dc5af224cf1e2d0744f47e6396ac7f394e888b627734b907f5bf9fcf016506`.
  Receipt `.cache/ipk/run-2k2ho7b_/` (`latest.json` pointed there at that time).
  ARM ELF, identity, version, log marker passed; additional audit confirmed Other
  menu, old Experimental menu absence, new rollback code, 14 packaged bundles.
  After user confirmed availability, installed through SSH/Docker and reopened
  at 19:57 on **OLED42C2PSA**, firmware **33.31.68**, webOS **10.3.1**. ELF matches;
  settings/pairing preserved; old logs saved before reset. pt-BR startup with NDL
  video/audio, no crash in initial capture. Homebrew absent; root Game Mode
  unavailable. Streaming/scanout/HDR pending at that point. Receipt:
  `.cache/experimental-physical/installation-receipt.json`.
- **2026-10-24, 11:35 São Paulo**: **1.3.3** rebuilt with NDL callback-context
  lifetime protection and HTTP fixes, no increment. ARM/RelWithDebInfo,
  metadata/ELF, per-installation reset passed. SHA-256
  `7f2b74c21926432ba35b7ef04d81bb3b2f6ec7caff47fdfa7f69fa2f194111a1`;
  receipt/log/symbols `.cache/ipk/run-16ak3gfd/`. New symbols confirm
  `CallbackContextLock`, `http_init`; NDL module SHA-256
  `b264106440c7f8cb223841df721518075c330a5b341a6ebdec867769f45b3817`.
  Desktop build, four relevant CTests, HTTP/NDL/decode ASan/UBSan fixtures passed.
  New callback case failed before patch. Package not installed/app not run,
  per user restriction for that task.
- **2026-10-22, 21:36 São Paulo**: **1.3.3** rebuilt with local NDL concurrency
  and video-fragment validation fixes, no increment. ARM/RelWithDebInfo,
  metadata/ELF, per-installation reset passed. SHA-256
  `7d40301f90207ecab6971cf32c49277e017f9d33509210c628c9a5e326b0de0d`;
  receipt/symbols `.cache/ipk/run-s4iicwal/`. Full desktop build, two video-safety
  CTests, ASan/UBSan fixture passed. Installation/physical validation of that
  package remained pending.
- **2026-10-22**: central version **1.3.2 → 1.3.3**, interpreting requested
  `1.333` as next `X.Y.Z` patch. Helper checked appinfo/APP_VERSION/ELF derivation.
  **1.3.3**, ARM/RelWithDebInfo via Docker, generated/validated with diagnostics
  v2 and per-installation log reset. SHA-256
  `74129148808e5a3ed2ca9cede0eb408ff4ec2568647fcca56cbca1cc7b382e32`.
  Sources `454e6b25888201816fa52ffb4720c67bd92cbdfb` plus local version/docs
  changes. Metadata, ARM ELF, reset marker passed; receipt/log/symbols
  `.cache/ipk/run-wyjwcgps/`. No installation/C2 runtime validation in that task.
- **2026-10-22**: requested central bump **1.3.1 → 1.3.2**; helper verified
  derivation. **1.3.2** rebuilt/validated ARM/RelWithDebInfo with fatal capture
  and installation cleanup marker. Clean sources at
  `7a9e9f76b52d362d9ad1ec260863b02f353045cd`. SHA-256
  `b4eb9eb6b36c59af0bde4f9faa5ea7b6d42dede6067622df3181a7858b61e3ae`.
  Receipt/log/symbols `.cache/ipk/run-vyajzlsw/`; no installation/physical C2 test.
- **2026-10-22**: another **1.3.2** IPK with diagnostics **v2**, locally modified
  sources, ARM/RelWithDebInfo. SHA-256
  `fd15de7413a23f921b79ea01848dbd5b17225f5de08ea43ac38fe092fc0f2c64`.
  Metadata/ELF, v2 strings, reset marker, symbol archive checked; ARM unwind tables
  present, no mandatory libunwind dependency. Receipt/log/symbols
  `.cache/ipk/run-2wcjcrr9/`. New capture not installed/physically validated there.
- Skills: `aurora-build-ipk`, `aurora-bump-version`, referenced in AGENTS.md.
- Build command: `python3 scripts/webos/build_ipk.py`. Single delivery path:
  **`aurora.ipk` at Aurora root**, Git-ignored. Prior package replaced only after
  ID/version/control/architecture/ARM ELF marker validation.
- Logs/receipt: `.cache/ipk/run-*/build.log`, `source.json`,
  `.cache/ipk/latest.json`; generated binary is not versioned source.
- Single version source: `MOONLIGHT_VERSION` in CMakeLists.txt. Manifest,
  APP_VERSION, ELF `aurora-v` marker derive from it.
- Default increment: `python3 scripts/dev/aurora_version.py --part patch --apply`.
  Without `--apply`, dry run only. Version at skill creation: **1.3.1**; current
  central version **1.3.5**. Build/increment are separate operations.
- Generating a package neither installs it nor proves C2 operation.
- **2026-10-19** validation: real Linux/amd64 Docker build, ARM target,
  RelWithDebInfo, helper completed. `aurora.ipk` **1.3.1**, metadata/ELF/version
  marker checked. SHA-256
  `b34761ff1ec175094bfa8976b8f42517b317728e3be933c3dd70f2c2462df9b4`.
  Latest local build receipt `.cache/ipk/latest.json`; package not installed then.
- **2026-10-22** build with textual fatal capture below: **1.3.1** ARM/RelWithDebInfo,
  metadata/ELF/version marker/capture strings verified. SHA-256
  `33c0f87dbfb376ff59134e9f92fe580a331b5b2cc34381aa13690664dc62e1d4`.
  Receipt/full `bin/aurora` symbols `.cache/ipk/run-nsvt2f47/`
  (`_dbgsym.tar.gz` in `packages/`). Locally modified sources; no bump/install.
- **2026-10-22** regenerated with per-installation cleanup, same **1.3.1**,
  ARM/RelWithDebInfo. Marker/reset code checked; receipt includes
  `installation_log_reset_verified=true`. SHA-256
  `23a867609d39371718cad6532b445a51f14fe6805a7f6c530af71100f9efa1a1`.
  Receipt/log/symbols `.cache/ipk/run-eu54hyum/`; not installed then.

## Confirmed C2 SIGABRT and v2 capture (2026-10-22)

- **2026-10-24** review of three supplied files: fatal at **11:18:29 São Paulo**,
  PID **16316**, `moonlight-io`, `munmap_chunk(): invalid pointer`.
  `aurora-run.state` matches. Full log (8967 lines) has five startups marking
  previous runs `crashed`; last fatal follows session cleanup. Own frame
  `0x4fe40` resolves to `http_destroy` in local 2026-10-22 IPK symbols; remainder
  passes through libc/c-ares/libcurl. This locates detection, not original write
  or current installed-binary identity. A new isolated test reproduced an invalid
  NDL `LOADCOMPLETED` callback write during player destruction. Local SS4S patch
  protects context publication/removal with a mutex separate from firmware calls,
  avoiding post-destruction access and synchronous-callback deadlock. Network
  now initializes curl before threads, checks HTTP buffer sizes/OOM, uses correct
  API types, clears temporary references after requests. Evidence/limits:
  [DIAGNOSTIC_LOG_REVIEW.md](DIAGNOSTIC_LOG_REVIEW.md). No app/streaming session
  ran during this investigation. HTTP and NDL/decode ASan/UBSan fixtures, desktop
  app build, four relevant CTests passed, including 20000 callback cycles.
- New v2 capture at **21:18:36 on 2026-10-22** confirms **heap corruption**:
  `corrupted size vs. prev_size`, PID 5052, VideoRecv, AV1 1440p90. Exact C2
  libraries resolve stack to `g_malloc` called from LG pipeline feed through
  NDL_DirectVideoPlay. This locates detection, not original write. Local sources
  now serialize video/audio feed and unload for one NDL player, avoid callback/
  stats deadlock, and validate fragment-copy sizes. Previous driver reproduced
  overlap; concurrency, synchronous callback, late prime, error, buffer-limit
  tests pass ASan/UBSan. Physical crash resolution/C2 latency effects pending.
  Details in [DIAGNOSTIC_LOG_REVIEW.md](DIAGNOSTIC_LOG_REVIEW.md). Local SS4S
  patches on `49918fc58f0f78fd342ed6e891172a2b401a9a2c`; submodule not updated.
  Later inspection found another fatal in the old installed module: PID 7197,
  **21:30:25**, `double free or corruption (out)`, `video-src:src`. Exact libc
  locates detection at thread-exit tcache free; this neither identifies original
  write nor tests corrected package. Old/new hashes recorded in diagnostic report.
- **1.3.3** installation log at **20:17:46** confirms cleanup and
  `CrashLog v2 ... persistent=1 backtrace=1` on C2. v2 startup validated; later
  actual fatal stack described above. The following incident has audio gaps/
  overflow, no new submits, control disconnect at **20:19:43**, cleanup, app still
  active; no new SIGABRT. User clarified Windows remained responsive and only
  game image went black. Independent ABR defect confirmed then: stale 0%-loss
  sample permitted increase probes after video stopped. Fix pending at that
  inspection (later sample-policy fixes are recorded in the 2026-10-24 review);
  it does not establish black-screen cause. Installed VibeShine log unavailable.
  Evidence: [DIAGNOSTIC_LOG_REVIEW.md](DIAGNOSTIC_LOG_REVIEW.md), local comparison
  [VIBESHINE_INTEGRATION.md](VIBESHINE_INTEGRATION.md).
- New **v1** capture confirms **SIGABRT** at **20:02:58**, PID **9531**, thread
  **9617**, 95 seconds after launch. Matching `aurora-run.state` is `crashed`.
  libc-relative PC/LR repeat **0x21ca6 / 0x5e7c4** from first report; abort origin
  unknown. Last video had zero feed errors at 20:02:57.197. Extended log also
  shows NDL failure/SDL_QUIT shutdown at 20:01 in previous PID **7483**, followed
  by startup reporting `crashed`; its fatal report was overwritten by the last
  crash. Separate events. Local evidence:
  `.cache/crash-diagnostics/sigabrt-2026-10-22-200258/`; details in report.
- Supplied report identifies Aurora **1.3.2**, PID **3474**, thread **5373**,
  **SIGABRT (6), si_code=-6**, **2026-10-22 19:29:27 São Paulo**. PC/LR belong
  to TV libc; v1 did not capture call origin. Next startup confirms `crashed`.
  Supplied `running` state is a new run, PID **7483**. This validates persisted
  v1 fatal capture/previous-run identification on C2, not root cause.
- **v2** adds thread/signal-sender name, up to 32 local stack addresses through
  optional libunwind, glibc fatal message when available (512 characters).
  Probing/symbol resolution occurs at startup; handler uses fixed buffers/signal
  I/O. Correct ARM GP context conversion, own unwind tables retained, only
  DWARF/EXIDX metadata requested. Missing libc/firmware tables can yield partial stack.
- Basic report synced before unwind. Unwinder failure preserves original signal
  and records `backtrace_fault_signal`; missing library preserves basic capture.
  libc message safely read through `/proc/self/mem`, without raw-memory dump.
  Original SIGABRT cause remains pending; details in diagnostic report.
- Linux CMake tests passed: file/crash/reset, unwind available/absent, deliberate
  unwinder fault. ARM32 fixture passed under QEMU + Ubuntu glibc/libunwind 1.6.2,
  including abort with partial stack/assertion reason. This does not emulate LG
  firmware. macOS/Linux ASan/UBSan tests also passed; simulator built. v2 build/IPK
  completed/validated per receipt above; physical v2 validation was pending then.
  Central version was **1.3.3** after that later bump; per-installation cleanup retained.

## Log cleanup on every installation (2026-10-22)

- Permanent user preference in AGENTS.md: every installation starts with three
  clean diagnostic files, including reinstalling the exact same IPK/version.
  Reopening the same installation preserves history.
- Every IPK includes `aurora-logs-reset.pending` at app root. First launch removes
  only `aurora.log`, `aurora-crash.log`, `aurora-run.state` from `$HOME/conf` and
  fallback `/tmp/moonlight-tv/conf` before writers, then consumes marker. Version/
  build date is not the sole trigger. Logs success/error; failure retains marker
  to retry cleanup.
- Settings, profiles, pairing, other files preserved. Exact names, no following
  log/directory symlinks or truncating shared inodes. IPK verifier requires valid
  marker; publication fixtures reject missing marker without replacing old installer.
- Validation: reset fixture passed macOS/Linux with warnings as errors; nine
  Python release/publication tests passed. CTest `test_diagnostic_reset`,
  `test_crash_log`, `test_file_log` passed; `aurora-ui-sim` built. ARM build/IPK
  validated, including marker/reset code in packaged executable. Actual C2 cleanup
  had not run at that stage; new-package installation was not automatic.

## C2 unexpected-exit diagnostics (2026-10-22)

- Supplied log identifies Aurora **1.3.1**, webOS **10.3.1**, NDL, AV1 8-bit
  2560×1440@90, HDR off, ABR ceiling 95000 kbps. Abrupt interruption at
  **2026-10-21 21:27**, restart, `/resume` are distinct from explicit AV1 feed
  failure at **22:46**, teardown, `quit_app=0`. First exit cause undetermined.
  Detailed evidence: [DIAGNOSTIC_LOG_REVIEW.md](DIAGNOSTIC_LOG_REVIEW.md).
- Installed process is `aurora`; TV soft/hard core limits confirmed **0/0**.
  Temporary folders held no accessible reports after TV reboot. This does not
  exclude a crash.
- New textual `aurora-crash.log` (**64 KiB**, last fatal preserved on reopening)
  and `aurora-run.state` (**511 bytes**) alongside `aurora.log`. Records fatal
  signals, times, version/PID/boot ID, ARM PC/SP/LR, executable mappings. Next
  startup warns of incomplete prior shutdown. Preopened descriptors, fixed
  buffers, signal-safe I/O; no logger/malloc/unwind/mutex in v1 handler.
- Exportable through Dev Manager, nonregular files/links rejected, bounded
  content. Independent of root/Crash Reports list; same log directory/fallback,
  startup reports persistence. Full core remains disabled outside Debug:
  textual failure report, no memory dump/full stack. Does not recover prior incident.
- SDL lifecycle, exit-request origin, shutdown begin/end logs help distinguish
  normal exit, captured failure, incomplete run. SIGKILL/power/storage/severe
  corruption can prevent reports; marker alone proves neither OOM nor cause.
  Only main thread gets alternate stack; worker stack-overflow capture not guaranteed.
- Isolated macOS/Linux tests passed: actual fatal signals, main/worker memory
  faults, termination with original signal, SIGKILL without invented report,
  cross-process markers, last-fatal preservation, limits, permissions, link
  rejection. Linux also passed ASan/UBSan with sanitizer fatal interception
  disabled to exercise the actual handler.
- Linux CMake integration passed: CTest file/crash logs, simulator build. webOS
  ARM RelWithDebInfo/IPK validation completed; executable contains new markers,
  matching symbols preserved (build receipt above). Version held at **1.3.1**
  then; no TV installation/physical capture validation in that task. Original
  exit was neither fixed nor reproduced; work prepared next-incident diagnostics.

## Language-update skill (2026-10-18)

- Created [aurora-update-languages](../.agents/skills/aurora-update-languages/SKILL.md)
  with contextual language adaptation, terminology/placeholder review, gettext
  synchronization, webOS JSON and localized UI checks.
- Included read-only auditor: POT coverage, fuzzy, duplicates,
  `msgfmt --check --check-format`, real AWK-output comparison. Initial report:
  `.cache/i18n/skill-creation-audit.json` (ignored local artifact).
- Initial inventory: 16 catalogs, 13 in `I18N_LOCALES`; `ko` in selector but not
  packaging, `uk`/`zh-TW` catalog-only. All catalogs had pending entries against
  existing POT. Skill creation did not synchronize/translate or enable languages.
- Simulator then fixed English preference; desktop initialized locale from
  environment, settings reapplied preference. Needed alignment and locale/MO/fonts.
  Skill requires loaded-language proof and PNG inspection at two resolutions;
  no language was visually/physically validated during skill creation.
- Tool validation: structure/metadata passed; ten gettext/real-AWK fixtures
  passed, covering placeholder errors, fuzzy, duplicates, unsupported contexts/
  plurals, accents, long lines.

## Language update and visual review (2026-10-19)

- All 16 existing catalogs have **211 current messages translated**, no empty/
  fuzzy entries. POT re-extracted; comments/credits/obsolete translations retained.
  Includes help/shortcuts, settings, errors, stats, Disconnect vs host End session.
  Native-resolution indicator localized.
- Korean added to packaging because already selectable. Korean/Dutch native
  names corrected. Ukrainian/Traditional Chinese remain outside selector/package;
  Japanese remains DEBUG-only. Updating catalogs did not change those conditions.
- Simulator accepts `AURORA_UI_SIM_LANGUAGE`, compiles all 16 MO catalogs,
  prepares locales/CJK fonts. `tools/ui-sim/localization.json` uses actual
  translated text at two resolutions. Checkbox/dropdown text can be asserted;
  `scroll_text` inspects without clicking.
- Fixed 720p toolbar overflow hiding long-language buttons and dropdown state
  consuming next Back. Server selector uses copied text because LVGL ellipsis
  modifies buffer and cannot write gettext read-only memory.
- Validation: gettext/webOS JSON passed all 16; **16 × 80 UI steps** 1080p/720p,
  plus **16 × 8** localized native-resolution steps. Captures opened for layout/
  glyph inspection. Eight release-helper tests passed. Initial coverage/limits:
  [I18N_REVIEW.md](I18N_REVIEW.md).
- No C2 installation/IPK/version bump. TV fonts/bundle loading, webOS-specific
  selector, actual streaming overlay remain dependent on physical validation.

## Implemented in the current client

| Area | Existing behavior | Evidence |
| --- | --- | --- |
| Platform | C11, CMake, LVGL v8. webOS SDL3 via sdl2-compat; desktop SDL2. SS4S video/audio. | `CMakeLists.txt`, `third_party/commons/cmake/ExternalSDL3ForWebOS.cmake`, `src/app/stream/` |
| Catalog/launch | Preserves UUID/ArtVersion, sends launch UUID with numeric fallback; recognizes VibeShine synthetic controls by UUID. | `core/libgamestream/src/{xml,client}.c`, `src/app/ui/launcher/apps.controller.c` |
| Resume | Current application resumes through `/resume`, including HDR. | `gs_start_app_ex` in `core/libgamestream/src/client.c` |
| Exit | Disconnect ends transport; End session confirms `/cancel`. Overlay blocks End session on direct Monitor/Input entry. | `src/app/ui/streaming/streaming.controller.c` |
| 410 controls | Known completions with matching UUID/message return to launcher without RTSP; confirmation remains a message without automatic repeat. | `gs_start_app_ex`, `src/app/stream/session_worker.c` |
| Bitrate | Session respects backend ceiling; ABR respects it/host-applied bitrate, stops before transport teardown. | `session.c`, `adaptive_bitrate.c`, `session_worker.c`, `gs_set_bitrate_ex` |
| Statistics | New sessions clear old samples; output FPS measures backend submissions, not physical presentation. | `src/app/stream/video/session_video.c` |
| Settings | Registered panes: Stream, Input, Host, Other. INI persistence/local profiles exist. | `src/app/ui/settings/settings.controller.c`, `src/app/app_settings.c`, `src/app/profile/` |

## README and built-in controls guide (2026-10-18)

- The README now reflects the current settings, bitrate limits, local `aurora.ipk`
  build helper, diagnostics, and session controls. Its English copy distinguishes
  C5 experiments from C2 validation and removes blanket compatibility claims.
- The home screen's **?** dialog opens **Controls & shortcuts**, with English
  source copy for TV remote, gamepad, virtual mouse, on-screen keyboard, and
  Disconnect / End session. It replaces the upstream wiki launch buttons and
  keeps version, decoder, audio backend, and system details at the bottom.
- The guide wraps long text and supports D-pad scrolling, mouse wheel, and
  Back/B/Escape dismissal. In particular, remote Hold Back opens the stream menu;
  gamepad Select/Back held alone for four seconds toggles pinned stats.
- New help catalog entries initially retained English fallback; all existing
  catalogs were completed in the 19/10 language review above. The app's language
  default and existing user language preferences are unchanged. A streaming hint now
  advertises the stats shortcut instead of pointing to removed wiki content.
- Validation: the Docker SDL/LVGL simulator built successfully; 33 steps passed,
  covering opening the guide, D-pad and wheel scrolling, reaching About, resizing from
  1920 × 1080 to 1280 × 720, Escape and close-button dismissal, reopening, and
  opening Settings afterward. Local README links and gettext catalogs also passed
  validation. Screenshots were inspected at both resolutions. This validates UI
  behavior with dummy backends, not remote key delivery or streaming on a TV.

## Clarified VibeShine contracts

- **Remote Input:** input transport with synthetic black video, no desktop
  capture/audio. Role released at transport teardown.
- **Remote Monitor:** captures the exact virtual monitor; retains it for Resume
  after disconnect by default. Host flags can remove display at stream end or
  disconnect. Keeps host playback; muting transmitted monitor audio is separate.
- **Duplicate Desktop:** secondary catalog intentionally contains resume shortcut
  and configured application. `currentgame=0` does not prove no active host game.
- **Terminate:** preserves remote roles. Secondary clients normally confirm with
  another action within 60 s; game owner can terminate on first request.
- **10-bit SDR:** supported by host device/application preference. `hdrMode=1`
  also negotiates Main10. SDR preference suppresses effective HDR; application
  preference and explicit HDR override take precedence. Aurora's local option
  alone does not configure this administrative policy.
- **ABR:** host advertises adaptive decisions unsupported; `/bitrate` applies
  client decisions, returns applied value, may clamp it.
- **Host VRR:** capture/virtual-display policy, including 1000 Hz request and
  smaller queues. Independent of physical LG VRR.

Code tracing/references are in the
[VibeShine review addendum](NONARY_MOONLIGHT_WEBOS_REVIEW.md).

## Gaps and pending decisions

| Priority | Current situation | Next verifiable step |
| --- | --- | --- |
| High | `quitappafter` persisted/visible, no teardown consumer found. | Define/fix semantics without turning Disconnect into unintended `/cancel`. |
| High | Profiles only store resolution, integer FPS, bitrate, rotation, HDR, HEVC. AV1/other options global. | Explain scope or expand profiles, testing switching/migration. |
| High | UI permits 400 Mbps; session applies lower ceiling without explaining effective value. | Show requested/applied/limit from selected backend. |
| Medium | Stream now announces reconnection for video/bitrate/ABR; NTSC remains in Host without that callback. | Review remaining Host preferences separately. |
| High | Backend-advertised codec/FPS does not prove every C2 mode. | Measure HEVC/AV1 per resolution/FPS and physical presentation, preserving SS4S caps. |
| Medium | Local 10-bit SDR label does not explain host-policy dependency. | Improve guidance and validate actual Main10 SDR on C2. |
| Medium | Terminate/replacement confirmation appears as generic error. | Handle confirmation in UI while requiring another explicit user action. |
| Research | Physical VRR not implemented/demonstrated in current NDL; no validated PyroWave/4:4:4 path on this TV. | Require API/backend and hardware proof before offering capabilities. |

Most promising Nonary ideas: per-stage/frametime diagnostics, configuration
notices, calibration. DXGI/Vulkan/Metal renderer is not directly portable to NDL.
Do not import 116/100 FPS presets as guaranteed smoothness on fixed 120 Hz output.

## Available validation evidence

| Evidence executed in the comparison reviews | Result | Limit |
| --- | --- | --- |
| `tests/core/run_vibeshine_protocol.sh`, Docker `aurora-ui-sim:local`, ASan/UBSan | Passed | Parsing/protocol with mocked network/crypto; no real TV/host. |
| VibeShine `test_remote_session.cpp`, `test_hdr_request_policy.cpp`, `test_framegen_policy.cpp`, Clang/macOS, ASan/UBSan | 48 passed | Policies; no actual Windows driver execution. |
| `test_wgc_source_admission.py` replay, Clang/macOS | 794 checks, zero failures | Modeled WinRT/clocks; no physical capture. |
| `python3 -m unittest discover -s tests/scripts -p test_release_helpers.py -v` | 8 passed | Temporary-copy increments/IPK fixtures; old-installer preservation, mismatched metadata/architecture rejection. |
| `python3 scripts/webos/build_ipk.py --backend docker --jobs 6`, RelWithDebInfo | ARM build and `aurora.ipk` 1.3.1 delivery completed | Package/ELF validated; no C2 installation/runtime validation. |

These comparison reviews performed no new C2 physical test/deployment. Available
log includes NDL webOS5 1440p90 at 93 Mbps but alone identifies neither model/
firmware nor cause of each hitch. 90 FPS on fixed 120 Hz is a judder hypothesis
to isolate by comparing 60/120 on the same pan. Earlier C5 experiments remain
in [streaming history](WEBOS_STREAMING_HISTORY.md), referenced by AGENTS.md.
Later installation rounds are recorded separately above.

## Maintaining this state

Use [AGENTS.md](../AGENTS.md) skills for the corresponding comparison. Update
only inspected scope, retain date/commits/dirty state, link detailed reports,
distinguish observation, inference, plan. Explicitly record corrections to older
conclusions. Routine tasks do not require revalidating all three repositories.

`scripts/dev/comparison_snapshot.py` produces local JSON evidence:

```sh
python3 scripts/dev/comparison_snapshot.py --reference vibeshine=../vibeshine --reference nonary=../moonlight-qt --output .cache/comparisons/current.json
```

Reference paths are examples; use real checkouts. JSON records HEAD, status,
diff/new-file/submodule hashes without copying source contents. Identifier
changes with local state even without a new commit. Snapshot is a manifest,
not a working-tree backup or remote-freshness check. Use `.cache` to avoid
including output itself in the next manifest.

## Baseline corrections

- 2026-10-18: initial review read `hdrMode=1` as evidence of 10-bit SDR conflict.
  Host inspection confirmed explicit support/administrative preference dependency;
  detailed comparison corrected.
- 2026-10-18: confirmed Input releases with transport; Disconnect Input exists
  in protocol but is not published in current catalog. Default retention is Monitor.
- 2026-10-18: ABR differences in historical log investigation already had
  working-tree fixes; do not repeat old diagnosis as present state. webOS build
  already uses SDL3 with SDL2 compatibility.
- 2026-10-18: AGENTS.md reorganized as architecture/implementation/build/test/
  comparison guide. Prior C5 pacing/AV1/audio reports preserved in
  WEBOS_STREAMING_HISTORY.md; streaming unchanged.
- 2026-10-18: IPK/version skills and executable helpers created. Old fixed
  `aurora-v1.3.0` marker replaced with APP_VERSION derivation while project held
  at 1.3.1. Installer gets fixed root delivery name; validation failures preserve
  old installer.
- 2026-10-18: updated COPYRIGHT in English to identify
  `https://github.com/GuiDev1994/aurora-tv/`, distinguish Aurora contributions
  from inherited Moonlight TV code, and retain GPL-3.0-or-later and dependency
  attribution. Checked against the referenced repository and local license files;
  no license terms or application behavior were changed.
- 2026-10-18: compared `.gitignore` with mariotaku/moonlight-tv `main` and added
  missing `.virtualenv`/`.mcp.json` exclusions. Aurora also ignores local webOS
  packages/SDK downloads, root build logs, Visual Studio data, Python bytecode/
  environments, Ninja/CPack output. Removed 13 tracked local artifacts (Visual
  Studio data, two build logs, ignored `.superpowers` report) from index while
  preserving on disk. Sources, required patches, shared VS Code settings, project
  skills remain trackable. Positive/negative ignore cases and local hashes checked.
- 2026-10-25: standardized maintained technical documents/instructions in English
  and removed obsolete implementation artifacts. Preserved historical evidence,
  localized catalogs, required build resources, and pre-existing SS4S modifications.
