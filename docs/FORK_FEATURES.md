# Fork features and upstream port ledger

This fork (`HiagoVLB/aurora-tv`) diverged from
[GuiDev1994/aurora-tv](https://github.com/GuiDev1994/aurora-tv) at `4e604455`
(upstream v1.3.1). Upstream is never merged wholesale: each upstream commit is
reviewed and either ported, recorded as already covered, or rejected with a
reason. This file exists so that a later port **never silently removes a local
feature or re-imports a change that was rejected on purpose**.

Read it before any upstream sync, and update it in the same change.

## 1. Local features that must survive every port

If an upstream hunk touches one of these areas, port it by hand on top of the
local code; never resolve a conflict by taking upstream's side.

| Feature | Where it lives | How to check it still works |
| --- | --- | --- |
| Bounded persistent diagnostic log (`aurora.log`), readable by Dev Manager | `src/app/util/file_log.*`, calls in `app.c`, `session*.c`, `adaptive_bitrate.c`, `session_video.c` | `[VideoDiag]` lines every 2 s in `conf/aurora.log` while streaming; Dev Manager can open the file |
| Crash log v2 with libunwind backtrace, abort message and thread names (`aurora-crash.log`, `aurora-run.state`) | `src/app/util/crash_log.*`, `src/main.c` | crash-log CTests; `aurora-run.state` says `running` while open |
| Clean logs on every install (`aurora-logs-reset.pending`) | `src/app/util/diagnostic_reset.*`, IPK helper | IPK helper validates the marker; first launch after install starts with empty logs, settings and pairing kept |
| Frame diagnostics in SS4S (`aurora_frame_diag`) | `third_party/ss4s/modules/webos/common/aurora_frame_diag.*` | see [FRAME_DIAG.md](FRAME_DIAG.md) |
| **Uncommitted SS4S NDL patch** (NDL start on load, video rect, Opus silence fill for eARC, delayed LOADCOMPLETED audio prime) | `third_party/ss4s` working tree on top of `49918fc5` | `git -C third_party/ss4s diff \| shasum -a 256` must be `f340e71e80d017415edf5bde6d41b1f466a7aa840b776097003c6cc9879c05f3` unless deliberately changed and recorded here |
| 5.1 audio: `surroundParams=642014523` with Opus 5.1 passthrough | `session_worker.c`, SS4S audio | [WEBOS_STREAMING_HISTORY.md](WEBOS_STREAMING_HISTORY.md) "5.1 audio" |
| IDR refresh, Game Mode via Luna backend, HTTP lifecycle hardening | `src/app/stream/video/idr_refresh.*`, `src/app/platform/webos/game_mode*.c`, `core/libgamestream/src/http.c` | their CTests |
| Decode-unit helpers and NDL feed serialization | `src/app/stream/video/decode_unit.*`, `session_video.c` | decode-unit / NDL serialization CTests |
| VibeShine contracts: launch UUID, `/resume` incl. HDR, synthetic Monitor/Input controls, HTTP 200 / XML 410 handling, `/cancel` only on explicit End session | `core/libgamestream/src/{client,xml}.c`, `session_worker.c`, `streaming.controller.c` | `tests/core/run_vibeshine_protocol.sh`; [VIBESHINE_INTEGRATION.md](VIBESHINE_INTEGRATION.md) |
| **HDR / 10-bit SDR launch contract:** any 10-bit format sends `hdrMode=1`; 10-bit SDR is selected by the VibeShine device/app `prefer_10bit_sdr` policy | `gs_start_app_ex` in `core/libgamestream/src/client.c` | [NONARY_MOONLIGHT_WEBOS_REVIEW.md](NONARY_MOONLIGHT_WEBOS_REVIEW.md) "10-bit SDR, HDR, precedence" |
| Fractional refresh hints (`clientRefreshRateX100`, `displayRefreshRateMillihertz`), Custom FPS text field (119.88 etc.); on desktop, `settings_sync_refresh_rate` lets fps follow the fractional rate | `client.c`, `pref_fps.c`, `app_settings.c` | `test_settings` (`testSyncRefreshRate`), `test_pref_fps` (known fixture failure: fixed tap rows vs. scrolled list) |
| Extra resolution preset 3.6K (3584x2016) and Native/Custom rows | `src/app/ui/settings/panes/pref_res.c` | `test_pref_res` |
| **Remote buttons (user decision 2026-11-12; do not revert to upstream):** Home disconnects the stream first (game keeps running) and opens the LG home (luna `com.webos.app.home`) when the stream has closed, 3 s fallback; the TV sends Aurora no background event, so a stream left running behind the LG home came back corrupted (2026-11-12); Green = Windows key (both edges, not in view-only); Yellow opens the stream menu (Yellow again closes it); Red asks "Disconnect from PC?" (Cancel returns to the game; also from the menu); Blue opens the on-screen keyboard; digits -> Numpad; focused session-action highlight. The on-screen log is the stream-menu button "On-screen log: Off/Live/Frozen" (and Settings > Other > Show logs), no longer a remote key | `keyboard_webos.c` (stream), `lv_drv_sdl_key.c` `read_webos_key` (menu/UI path), `app_webos.c` (`app_webos_go_home`, called back from `root.c` on `USER_STREAM_FINISHED`), `streaming.controller.c` (`ask_disconnect`, `toggle_log`, `update_buttons_layout`), `streaming.view.c` (`log_btn`), `USER_ASK_DISCONNECT` | `test_remote_home`; help dialog; simulator `stream_menu` expects "On-screen log: Off" |
| VibeShine host warnings (`clientWarning` + optional `clientWarningMessage`) shown for 12 s in the stream notice; known code `virtual_display_disabled_by_client` uses local text, other codes the sanitized host text; never changes whether the stream starts | `xml_launch_warning` (`xml.c`), `gs_launch_warning` (`client.c`), `session_report_host_warning` (`session_worker.c`), `streaming_host_warning_show` | `tests/core/run_vibeshine_protocol.sh` |
| VibeShine hardware metrics (CPU/GPU/encoder/temperature) in the stats overlay ("PC load", compact "PC ..."); separate GS_CLIENT, polled only while the overlay is shown, ≥ 2 s and ≥ host `interval_ms`, stops after 3 failures; needs host `hardware_metrics` | `gs_get_hardware_metrics` (`client.c`), `src/app/stream/host_metrics.*`, `format_host_load` (`streaming.controller.c`) | `tests/core/run_vibeshine_protocol.sh` (parser); physical check pending |
| Stream dropped by an error after video was showing (network, video stall, decoder): dialog "Stream interrupted" with a localized reason and **Reconnect**, which resumes the same app on the same host like the launcher (the game stays on the PC); errors before video keep "Failed to start streaming" | `streaming_reconnect_target` (`session.c` `session_interrupt`), `session_error`/`session_reconnect` (`root.c`) | `test_stream_reconnect`; simulator action `stream_dropped` |
| Settings panes Stream / Input / Host / Other (language, logs, Game Mode, reset in Other); conditional-option guidance | `settings.controller.c` `entries[]`, `basic.pane.c`, `other.pane.c` | simulator `smoke.json`, `stream-options.json` |
| Settings teardown via `obj_will_delete_cb` + owned `stream_pane`; text chevron instead of private glyph | `settings.controller.c` | simulator `upstream-fixes.json` |
| Full local translation catalogs (incl. reviewed pt-BR) | `src/i18n/` | never replace with upstream catalogs; merge strings only |
| UI simulator and release-helper tests | `tools/ui-sim/`, `tests/scripts/` | `./tools/ui-sim/run.sh`; `python3 -m unittest discover -s tests/scripts` |

## 2. Upstream review ledger

Status: **ported** (in tree), **covered** (local code already does it),
**rejected** (do not import; reason given), **deferred** (may be worth it later
with the stated evidence).

Reviews: [2026-10-25](UPSTREAM_REVIEW_2026-10-25.md) (up to `916f3810`, v1.3.2),
[2026-11-11](UPSTREAM_REVIEW_2026-11-11.md) (up to `dcf4daa5`, after v1.3.3).
Last upstream commit reviewed: **`dcf4daa5`** (2026-11-07). The next review
starts at `git log dcf4daa5..upstream/main`.

| Upstream commit | Subject | Status | Reason / note |
| --- | --- | --- | --- |
| `b86ffb75` (moonlight-common-c) | nanors remaining-row scaling | ported 2026-10-25 | buffer overrun fix |
| `0cf66539`, `6cb3aa80` | webOS locstr leak / cache | ported 2026-10-25 | |
| `8eaf2907` | connection notice on main thread | ported 2026-10-25 | |
| `efa8f7b6` (parts) | quit worker snapshot, window-id self copy, settings Close/Back | ported 2026-10-25 | only these hunks |
| `7f9f045e` | quote `$@` in easy_build.sh | ported 2026-10-25 | |
| `5950fd3d`, `380f7960` | game grid setting, `aurora.ini` migration | deferred | settings-file migration needs its own review |
| `3c36701c` and other catalog imports | translations | rejected | local catalogs are newer; merge strings only |
| `487db143` | drop SPS rewriter | deferred | no runtime gain |
| `ba09901a`, `bdc581cb`, `97629247`.. `41d62e77`, `1ad4d70a`, `a4773394` | NDK/GCC 16 bump, app-id overrides, Docker build wrappers | deferred | pinned SDK and IPK helper already work |
| `e04d6065` | release held D-pad when a gamepad disconnects | **ported 2026-11-11** | |
| `d3689c26` | D-pad can leave the Custom FPS field | **ported 2026-11-11** | |
| `1b2b04f1`, `578be892` | pointer drag presses where it started | **ported 2026-11-11** | |
| `21fccab1` | cover corners turning blue after Settings | **ported 2026-11-11** | |
| `8e67e22c` | space fake key presses past the 50 ms duplicate filter | **ported 2026-11-11** | makes `test_pref_res` pass |
| `408656c4` | desktop `testSyncRefreshRate` expects fps to win | **rejected** | local desktop sync deliberately lets fps follow the fractional rate (119 + 11988 -> 120); the upstream test fails here |
| `9cee2a43` | res test taps one row lower | **rejected** | assumes upstream rows; the original local taps pass with `8e67e22c` |
| `a9b7c210` | LONG_DOT labels must not use static text | covered | local launcher/server popup already use `lv_label_set_text` |
| `231fe195` | send `hdrMode=0` for 10-bit SDR | **rejected** | written for Rubylight. VibeShine reads `hdrMode=1` as the client's 10-bit signal and picks 10-bit SDR from its own `prefer_10bit_sdr` policy; changing it breaks the documented contract (section 1) |
| `12101157`, `5de156cb` | Rubylight link hint / link test, moonlight-common-c bump | rejected | Rubylight-only protocol; host here is VibeShine; would move the moonlight-common-c submodule |
| `f1f93da6` | stats pin by gamepad, compact stats toggle | deferred | regenerates the icon font and rewrites the stream overlay that holds local shortcuts; port only by hand if wanted |
| `071f0a15`, `b5854456`, `6f9afcb5` | Bluetooth LE Steam Controller wake-up | deferred | no Steam Controller in use; runs `blegatt` on the TV |
| `8d6f2f46`, `9c289e14`, `74f84f61` | NDL frame drop threshold (added then reverted) | rejected | net effect is only ignoring a key this fork never wrote |
| `905741c9` | SS4S `videoRenderQueueThreshold` on SMP | rejected | SMP backend; this TV uses NDL and SS4S carries the local patch |
| `07a778db` | CI webOS 5 compat check | deferred | CI/packaging metadata only |

## 3. Port checklist

1. `git fetch upstream`; list `git log --no-merges <last reviewed>..upstream/main`.
2. Work on a branch; record `git -C third_party/ss4s diff | shasum -a 256` before
   and after. Never `git submodule update` or reset the submodule.
3. For each commit, decide ported / covered / rejected / deferred and add a row
   above. Check every hunk against section 1.
4. Apply with `git cherry-pick --no-commit` or by hand. Never take "theirs" in a
   conflict inside a section 1 area.
5. Linux Debug build + full CTest in `aurora-ui-sim:local`; UI simulator for UI
   changes; VibeShine protocol fixture for protocol changes; webOS ARM build.
6. Write `docs/UPSTREAM_REVIEW_<date>.md`, update [CURRENT_STATE.md](CURRENT_STATE.md)
   and "Last upstream commit reviewed" above.
