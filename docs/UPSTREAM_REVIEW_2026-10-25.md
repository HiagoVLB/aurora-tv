# Upstream Aurora review — 2026-10-25

Compared local `bb693bf9` (1.3.4) with
[GuiDev1994/aurora-tv main](https://github.com/GuiDev1994/aurora-tv/commit/916f3810)
at `916f3810` (upstream v1.3.2), fetched on 2026-10-25, São Paulo time.
The common ancestor was `4e604455`. Selection is based on source comparison;
upstream's lower release number does not mean its fixes were already present.
Changes are ported selectively into the working tree, without merging upstream's
release, preferences, or dependency changes wholesale.

## Imported changes

| Source | Applied behavior and reason |
| --- | --- |
| moonlight-common-c `b86ffb75` | Advance `9ae7cbb0` by exactly one commit: nanors scales only the remaining matrix row (`W - x`), avoiding writes past its packet-recovery work buffer. No protocol/API/timestamp changes are present in that commit. |
| Aurora `0cf66539`, `6cb3aa80` | Cache webOS translation results per locale, serialize lookup/reload, and free bundle probes. Repeated stats/UI lookups no longer allocate a new retained string each time. Keep cached strings alive because LVGL retains static label/button pointers across locale reloads. |
| Aurora `8eaf2907` | Queue connection notices through the application bus; translate and modify LVGL on the main thread instead of the video receive thread. Unknown statuses and unattached sessions are ignored. |
| Relevant hunks of Aurora `efa8f7b6` | Clone the host under the manager lock before the quit worker performs network I/O, release its client, and transfer/free the snapshot on success/error. Discovery can replace its own server during the request. |
| Relevant hunk of Aurora `efa8f7b6` | Avoid copying `last_window_id` onto itself in the SDL/NDL exported-window shim. |
| Relevant settings hunks of Aurora `efa8f7b6` | Put the embedded settings Close button in the active modal group; Left can focus it. Expire dropdown Back suppression so a subsequent intentional Back closes the popup. Preserve the local main-pane suppression guard and existing fragment teardown. |
| Aurora `7f9f045e` | Preserve CMake argument boundaries with `"$@"` in `easy_build.sh`, including values containing spaces. |

No catalog text, session cancellation semantics, remote Home/Numpad mappings,
ABR policy, frame timestamps, codec limits, version, or package identity changed.

## Deferred or already covered

- **Audio/SS4S, Bluetooth sniff control and 8BitDo bridge:** require separate
  device/audio-path evidence. Preserve the existing local NDL serialization and
  diagnostics patches. SS4S stays at `49918fc5`; its pre-existing dirty patch
  SHA-256 remains
  `f340e71e80d017415edf5bde6d41b1f466a7aa840b776097003c6cc9879c05f3`.
- **New SDK/GCC, app identity overrides and build wrappers:** broader packaging
  changes overlap the maintained IPK helper and are unnecessary for these fixes.
  The existing SDK successfully cross-compiles this selection.
- **Grid selection, settings-file migration, theme, server picker and extra
  backend controls:** wider behavior/UI changes need their own compatibility,
  migration and localization review. Existing preferences remain readable.
- **Translation catalog replacement:** local catalogs already include newer
  strings/reviews. Importing upstream's files would lose local coverage.
- **Settings teardown crash:** already addressed locally using
  `obj_will_delete_cb` and owned `stream_pane`; preserve that implementation.
- **Settings arrow:** the local text chevron already avoids the missing private
  glyph. Preserve it instead of replacing generated fonts.
- **App-loader cancellation reset:** a pending-cancel fix also affects active
  tasks and their queued callbacks. Defer until task ownership/ordering can be
  tested together; avoid introducing overlapping loads with a partial port.
- **Dead SPS rewriter removal:** no runtime benefit; unrelated cleanup deferred.

## Validation

- Full Linux Debug application/simulator build passed. All four new CTests
  (`test_nanors`, `test_i18n_webos`, `test_connection_notice`, `test_quit_app`)
  passed. Packet recovery checks **501 loss patterns**, including all data
  shards lost and recoverable from parity, with a guarded work allocation.
- The packet-recovery and translation tests fail against the original source
  and pass against the imported source under ASan/UBSan with leak detection.
  Locale tests cover copied keys, repeated lookups, retained labels, switching
  back, failed reloads and concurrent lookup/reload.
- Full CTest: **35/37 passed**. `test_pref_fps` (custom/100 FPS selection) and
  `test_pref_res` (custom keyboard entry) fail in the existing coordinate/input
  fixtures. Rebuilt/reran both with the pre-port settings controller in a
  separate build volume: the same assertions fail. They are not counted as
  passing; this port does not repair those fixtures.
- Ten Python build/release helper tests passed, including CMake arguments with
  spaces. `bash -n scripts/webos/easy_build.sh` and `git diff --check` passed.
- Real SDL/LVGL simulator: **124 steps passed**, English and pt-BR, each at
  1920×1080 and 1280×720. Remote Up → OK closes settings; dropdown Back closes
  only the list, then the next Back closes its pane; settings reopen with usable
  focus. Captures and JSON state confirm the Close focus and open/closed
  dropdown states. Reviewed the 1080p Close and 720p translated popup captures.
  The existing 25-step smoke scenario also passed, covering Input/Host/Other,
  Add PC cancellation and safe Exit cancellation (149 simulator steps in total).
- Full webOS ARM RelWithDebInfo compile/link passed using the existing pinned
  SDK and preserved dirty SS4S implementation.

Logs, sanitizer binaries and simulator captures are under
`.cache/upstream-review/`. No IPK generated, TV installed, or physical streaming
test performed. Compilation and mocked/simulator checks cannot establish C2
runtime behavior or guarantee the absence of every regression.
