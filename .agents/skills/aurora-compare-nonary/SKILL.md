---
name: aurora-compare-nonary
description: "Compare current Aurora with Nonary/moonlight-qt to assess behavior, features, and viable webOS/LG C2 adaptations, including pacing, VRR, codecs, and diagnostics; update the baseline with evidence and limitations."
---

# Compare Aurora with Nonary Moonlight Qt

Assess desktop features, existing Aurora equivalents, and adaptations justified
by the real LG backend. Reference fork: `https://github.com/Nonary/moonlight-qt`,
rather than only upstream Moonlight Qt.

## Identify both states

Locate Aurora root through `AGENTS.md`/`CMakeLists.txt`; read
[AGENTS.md](../../../AGENTS.md), [CURRENT_STATE.md](../../../docs/CURRENT_STATE.md),
relevant [NONARY_MOONLIGHT_WEBOS_REVIEW.md](../../../docs/NONARY_MOONLIGHT_WEBOS_REVIEW.md)
sections. Current state is a dated baseline, not proof of unchanged remote HEAD.

Use the user-provided reference checkout; otherwise use `../moonlight-qt` if it
belongs to Nonary, or an isolated clone in `.cache/references/nonary-moonlight-qt`.
Do not depend on old `/tmp` paths or reset/pull over existing work. If latest is
requested, query remote default-branch SHA and inspect an isolated checkout of
that commit. For local comparisons, record that remote freshness was not checked.
Always report the commit actually read.

From Aurora root with the real reference path:

```sh
python3 scripts/dev/comparison_snapshot.py --reference nonary=../moonlight-qt --output .cache/comparisons/nonary-state.json
```

The helper records uncommitted changes/submodules; it does not update remote references.

## Compare behavior and platform dependencies

Trace option → storage → effective configuration → protocol → decoder →
presentation. Identify defaults, mutually exclusive options, missing profile
fields, fallback, and next-connection changes.

| Topic | Nonary fork | Aurora |
| --- | --- | --- |
| Preferences/UI | `app/gui/SettingsView.qml`, `app/settings/streamingpreferences.*` | `src/app/app_settings.c`, `src/app/profile/`, `src/app/ui/settings/settings.controller.c`, `panes/` |
| Session/protocol | `app/backend/nvhttp.cpp`, `app/streaming/session.cpp`, `moonlight-common-c/` | `core/libgamestream/`, `core/moonlight-common-c/`, `src/app/stream/session{,_worker}.c` |
| VRR/presentation | `app/streaming/vrrratepolicy.*`, `app/settings/vrrtimingoptions.h`, `app/streaming/video/ffmpeg-renderers/ivrrframepresenter.h`, `pacer/`, concrete renderers | `src/app/stream/video/`, `third_party/ss4s/modules/webos/ndl/`, `smp/` |
| Telemetry | `app/diagnostics/`, `docs/vrr-diagnostics.md`, current `docs/` studies | `src/app/stream/video/session_video.c`, `src/app/ui/streaming/`, `docs/FRAME_DIAG.md` |
| Codecs/new paths | `docs/pyrowave-protocol.md`, implementation/negotiation in `app/` | `session.c` negotiation, SS4S capabilities/feed |

Compare concepts/contracts; do not propose copying QML/C++/DXGI directly into
LVGL/C11/NDL. Read concrete backends/activation conditions alongside UI labels.
Check whether options described in older studies have been removed.

Classify each feature: already equivalent; viable adaptation; host configuration
dependency; backend research required; incompatible with current path. State
latency/memory/network cost, fallback, observations needed. Desktop benefit
remains a hypothesis until measured on LG.

Specific cautions:

- Pacing, V-Sync, VRR, host capture refresh are distinct. Require a presentation
  contract webOS backend can actually fulfill.
- 120 submits/s do not prove 120 distinct displayed frames. `Play` duration is not
  decoding time; estimated queues are not physical latency.
- Respect historical attempts/AGENTS.md invariants: direct submit, 5.1 audio,
  rejected C5 PTS experiments. C5 results do not test C2; explain what makes a
  new hypothesis different.
- HDMI/codec/NDL capabilities and compositor rates differ. Do not raise SS4S
  limits solely from panel specifications.
- If conclusions depend on SDR Main10, synthetic controls, launch flags, inspect
  VibeShine; use the sibling skill for substantial host investigation. Flag names
  alone do not define effects.

## Evidence and baseline maintenance

Record both clients' commits/local state, files/symbols, effective behavior,
webOS compatibility, justified priority. Run relevant tests when they verify
contracts; algorithm/UI simulator tests do not validate physical presentation.
Without TV testing, describe the missing hardware test without claiming validation.

Update inspected `docs/CURRENT_STATE.md` areas: date, references, changes since
baseline, actual results, limits, next steps. Explicitly correct superseded
premises; preserve VibeShine comparison evidence/conclusions. Extensive reviews
may live in `docs/comparisons/YYYY-MM-DD-nonary.md`, summarized/linked from current
state. Analysis must not appear as an implemented/installed feature.
