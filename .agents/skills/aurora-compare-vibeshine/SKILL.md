---
name: aurora-compare-vibeshine
description: "Compare current Aurora with the Nonary/VibeShine host to review session, catalog, video, audio, input, and bitrate contracts; record differences and update the project baseline. Use for VibeShine compatibility or integration reviews."
---

# Compare Aurora with VibeShine

Produce a client/host diagnosis with reproducible versions, evidence from both
sides, and next steps for webOS. These projects are not equivalent clients.

## Find project and reference

Locate Aurora root through `AGENTS.md`/`CMakeLists.txt`, independently of cwd.
Read [AGENTS.md](../../../AGENTS.md), [CURRENT_STATE.md](../../../docs/CURRENT_STATE.md).
Use [VIBESHINE_INTEGRATION.md](../../../docs/VIBESHINE_INTEGRATION.md) for integration
history and VibeShine sections in
[NONARY_MOONLIGHT_WEBOS_REVIEW.md](../../../docs/NONARY_MOONLIGHT_WEBOS_REVIEW.md)
for HDR, remote sessions, capture.

Usual reference: `../vibeshine`, `https://github.com/Nonary/vibeshine`. Accept
another user-provided checkout. If absent, use isolated clone
`.cache/references/vibeshine`; never replace existing checkout. For latest remote
requests, check default branch/SHA and obtain that commit in an isolated checkout.
Otherwise compare available checkout and report no remote-freshness verification.
An old baseline SHA does not establish current remote HEAD.

Capture actual local state from Aurora root:

```sh
python3 scripts/dev/comparison_snapshot.py --reference vibeshine=../vibeshine --output .cache/comparisons/vibeshine-state.json
```

Adjust reference path. Helper is local/offline and records HEAD, diff hashes,
new files, submodules. Omitting uncommitted files would misidentify current version.

## Trace behavior

Select requested flows; follow UI/configuration → launch/resume/SDP → host
policy → encoder/capture → response → client effect. Search symbols/parameters
with `rg`; textual C/C++ diffs do not establish compatibility.

| Topic | Aurora | VibeShine |
| --- | --- | --- |
| Catalog, UUID, XML, 410 | `core/libgamestream/src/{xml,client}.c`, `src/app/ui/launcher/apps.controller.c` | `src/nvhttp.cpp`, `src/remote_session.{h,cpp}`, `src/process.cpp` |
| Disconnect, terminate, retained monitor | `src/app/stream/session{,_worker}.c`, `src/app/ui/streaming/streaming.controller.c` | `src/stream.cpp`, `src/rtsp.cpp`, `src/remote_display_topology.cpp` |
| HDR, Main10, color, host mode | `src/app/stream/session.c`, `src/app/app_settings.c` | `src/hdr_request_policy.h`, `src/rtsp.{h,cpp}`, `src/video_colorspace.cpp`, device/app preferences in `nvhttp.cpp`/`process.cpp` |
| FPS, capture, queues | `src/app/stream/video/`, `third_party/ss4s/modules/webos/` | `src/framegen_policy.h`, `src/config.h`, `src/platform/windows/display_wgc.cpp`, `wgc_capture_policy.h`, `display_vram.cpp` |
| ABR | `src/app/stream/adaptive_bitrate.c`, `core/libgamestream/src/client.c` | `/bitrate`, `/api/abr/capabilities` in `src/nvhttp.cpp` |

Check defaults, application/device/global precedence, permissions, errors, resume.
These points previously led to incomplete conclusions:

- `hdrMode=1` can also negotiate Main10 SDR; follow `prefer_10bit_sdr`,
  `effective_hdr_requested`, `effective_10bit_sdr_requested` to final colorspace.
- `currentgame=0` can be a secondary-client projection. Identical titles can
  have distinct UUIDs/roles. Input/Monitor have different lifecycles/capture/audio;
  verify the inspected version.
- HTTP 200 does not ensure XML success. XML 410 can mean completion or confirmation.
  Do not repeat a destructive action to test a hypothesis.
- Host virtual-monitor/capture refresh, stream FPS, LG panel refresh are distinct.
  `clientVrrRequested` does not prove TV VRR.
- Host administrative preferences do not automatically use GameStream pairing auth.

## Record conclusions and validation

For each finding, state Aurora behavior, host rule, file/symbol/commit evidence,
webOS impact, proposed action. Classify demonstrated bug, intentional difference,
host configuration, or pending hypothesis. Verify previous-document premises in
code; explicitly correct superseded conclusions and retain change references.

Select relevant existing tests: Aurora
`tests/core/run_vibeshine_protocol.sh`; VibeShine
`tests/unit/test_remote_session.cpp`, `test_hdr_request_policy.cpp`,
`test_framegen_policy.cpp`, `test_video_colorspace.cpp`, as appropriate.
Compilation/fixtures/replay do not prove actual WGC execution or C2 scanout.
Record command, environment, result, limits. Comparison does not imply deployment.

Update only inspected areas of `docs/CURRENT_STATE.md`: date, HEADs/dirty state,
baseline differences, evidence, pending work. Proposed fixes are not implemented
fixes. Preserve the other comparison's conclusions and existing hardware evidence.
For extensive reviews, use `docs/comparisons/YYYY-MM-DD-vibeshine.md` and a
baseline summary/link; small reviews can update baseline directly.
