# Upstream Aurora review — 2026-11-11

Compared local `ffede615` (1.3.5) with
[GuiDev1994/aurora-tv main](https://github.com/GuiDev1994/aurora-tv/commit/dcf4daa5)
at `dcf4daa5` (after upstream v1.3.3), fetched on 2026-11-11, São Paulo time.
The previous review stopped at `916f3810`; this one covers the 24 non-merge
commits in `916f3810..dcf4daa5`. Work branch: `upstream-review-2026-11-11`.
Decisions are also recorded in the [fork ledger](FORK_FEATURES.md).

## Imported

| Upstream | Behavior and reason |
| --- | --- |
| `e04d6065` | A gamepad that powers off or drops Bluetooth never sends button-up; LVGL kept repeating the held D-pad direction. Release the key and stick state on `SDL_JOYDEVICEREMOVED` / `SDL_CONTROLLERDEVICEREMOVED`. Runs in the existing event-filter case, no I/O. |
| `d3689c26` | Custom FPS is a text field (fractional rates); it swallowed every arrow, so the remote could not reach OK/Cancel. DOWN now focuses the dialog buttons, like Custom Resolution. Test updated to type the value. |
| `1b2b04f1` + `578be892` | Touch/pointer drag in a stream pressed at the point where the drag was detected, not where the finger went down, so drags missed window edges/items. Press at the start position; the next motion carries the drag. |
| `21fccab1` | Cover corners baked after visiting Settings came out blue (shared SDL corner mask kept the last color mod). Reset the mask before each cover draws. |
| `8e67e22c` | Test helper: space fake key presses past the 50 ms Magic Remote duplicate filter. Fixes the long-failing `test_pref_res`. |
| version | 1.3.5 -> **1.3.6** with `scripts/dev/aurora_version.py`. |

## Not imported

| Upstream | Decision |
| --- | --- |
| `231fe195` hdrMode=0 for 10-bit SDR | **Rejected.** Written for Rubylight. VibeShine treats `hdrMode=1` as the client's 10-bit signal and selects 10-bit SDR through its own `prefer_10bit_sdr` device/app policy (`vibeshine/src/nvhttp.cpp`, `rtsp.cpp`). Sending 0 changes that contract. The TV currently has `hdr=false`, `force_10bit=false`, so no 10-bit launch happens anyway. |
| `a9b7c210` LONG_DOT static text crash | Covered: local launcher and server popup already use `lv_label_set_text`; remaining static labels are not LONG_DOT. |
| `408656c4` desktop sync test | **Rejected.** Local desktop `settings_sync_refresh_rate` deliberately lets fps follow a fractional rate (119 + 11988 -> 120); the upstream test asserts the opposite and fails here (`Expected 119 Was 120`). Desktop-only; no webOS effect. |
| `9cee2a43` res test tap rows | **Rejected.** Assumes upstream's preset rows; with `8e67e22c` the original local taps pass. |
| `12101157`, `5de156cb` | Rejected: Rubylight link hint/test and a moonlight-common-c bump; host is VibeShine. |
| `f1f93da6` stats pin / compact stats | Deferred: regenerates the icon font and rewrites `streaming.view.c`, which holds local shortcuts. |
| `071f0a15`, `b5854456`, `6f9afcb5` | Deferred: BLE Steam Controller wake-up, unused here. |
| `8d6f2f46`, `9c289e14`, `74f84f61` | Rejected: net effect only ignores a key this fork never wrote. |
| `905741c9` | Rejected: SMP-only SS4S change; this TV uses NDL; SS4S carries the local patch. |
| `07a778db` | Deferred: CI/packaging metadata. |

## Preserved

- SS4S stays at `49918fc5` with the local uncommitted NDL patch; SHA-256 before
  and after: `f340e71e80d017415edf5bde6d41b1f466a7aa840b776097003c6cc9879c05f3`.
- moonlight-common-c, LVGL, translation catalogs, HDR/resume/cancel contracts,
  remote mappings, ABR, logs and crash log untouched.

## Validation

- Linux Debug build (`aurora-ui-sim:local`, Ubuntu 24.04 arm64) with tests and
  the UI simulator target: passed.
- Full CTest: **33/34 passed**. `test_pref_res` now passes (it failed in the
  2026-10-25 review). `test_pref_fps` still fails, pre-existing: the dropdown
  list opens scrolled to the selected value and "Custom" sits below the visible
  list (list y 101–449, rows 69 px from y 119), so fixed tap coordinates land on
  other rows. Fixing it needs index-based option taps; not part of this port.
- `tests/core/run_vibeshine_protocol.sh`: passed.
- webOS ARM IPK: see [CURRENT_STATE.md](CURRENT_STATE.md) "Version 1.3.6".
- No physical C2 test of the imported behaviors (gamepad disconnect, drag,
  cover corners) was done in this review.

Logs: `.cache/upstream-review-2026-11-11/`.
