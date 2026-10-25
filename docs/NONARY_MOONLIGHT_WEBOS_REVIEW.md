# Review: Nonary/moonlight-qt and Aurora on LG C2

Reviewed on 2026-10-18. Remote reference: `Nonary/moonlight-qt`, branch
`master`, commit `3ae5f3b0e8dfb7538422f2920d38126385d9efb4`.
Aurora base: `d241136e3b0de3aaa446a6be899b986181d802db`, including pre-existing
local changes. Local VibeShine: `97835dbfb14faafd8b4f13d98bc00cc921d45296`.

Scope: source inspection of all three projects, LG documentation, launcher
photo, and `../aurora.log`. Compatibility conclusions distinguish implemented
support, improvement hypotheses, and behavior requiring a real TV. This review
neither changes streaming nor installs anything on the TV.

An additional in-depth local VibeShine inspection on the same date covered
HDR/SDR policies, WGC capture, remote sessions, and existing tests. Conclusions
below incorporate corrections from that second inspection.

## Result

The fork is worth following as a timing, diagnostics, and VibeShine protocol
reference. Its desktop renderer is not directly portable to Aurora's NDL path.
The immediate C2 opportunities are fixing inconsistent options, choosing an
appropriate cadence, and measuring the correct pipeline stages.

Panel VRR is not implemented in the current webOS path. LG documents VRR/G-Sync
as requiring HDMI. The fork controls presentation through native desktop APIs;
Aurora submits compressed video to NDL without an equivalent adaptive
presentation API.

## Photo buttons and VibeShine sessions

These items come from the host's `/applist`. The Qt fork handles them through
normal application infrastructure; role semantics mostly live in VibeShine.
Local Aurora already adds reserved-UUID handling.

| Item | Behavior verified in code | Aurora status |
| --- | --- | --- |
| Configured Desktop / Steam | Starts the configured host application. The same running application can be resumed. | `gs_start_app_ex` uses `/resume` when the ID matches the current application, including HDR. |
| Additional Desktop | For secondary clients, the host intentionally publishes a resume duplicate beside the configured entry, with its own ID, UUID ending `07`, and leading spaces for sorting. | The photo matches this catalog composition. `project` and `SecondaryCatalogueKeepsConfiguredRunningAppBesideInvisibleResumeDuplicate` confirm intentional duplication. Runtime catalog is still needed to prove photographed card IDs. Do not deduplicate by title. |
| Remote Monitor | Activates/reuses a per-client virtual monitor and streams it. Host manages creation, mode, position, retention. | UUID ending `05`; can open while another app runs. Does not select an existing physical monitor. |
| Remote Input | Forwards input without capturing desktop or transmitting audio. Produces encoder-compatible black frames to maintain protocol-required video transport. | UUID ending `06`; black output is expected for this role. Uses normal client session flow. |
| Resume | Resumes the active game or retained monitor according to client role. | UUID ending `01`; host arbitrates target/permissions. |
| Terminate | Ends game transports, application, and display lifecycle while preserving Remote Monitor/Input. Game owner can terminate on first request. Secondary clients must normally repeat within 60 seconds; `remote_monitor_terminate_on_first_request=true` allows one request. | UUID ending `04`. Known success returns to launcher; first confirmation still appears as an error message and needs a second manual action. Confirmation binds client, generation, application. |
| Disconnect Monitor / Input | Explicitly releases the role. Retained-monitor owner's catalog includes Resume and Disconnect Monitor. Disconnect Input exists in the protocol but `project` does not publish it; Input releases when transport ends. | UUIDs ending `02`/`03`; documented successes recognized without RTSP launch. Do not tell users to find an unadvertised Disconnect Input card. |
| Overlay Disconnect | Stops client transport, leaving application on host. Input loses its role when transport ends; Monitor is retained by default, subject to two host removal policies. | `session_interrupt(..., false, ...)`. Monitor removal flags detailed below. |
| Overlay End session | Requests host `/cancel`, potentially closing configured app/restoring display. | Confirmation required. Disabled on direct Remote Monitor/Input entry because `/cancel` does not release those roles. |
| Default | Selects a local Aurora profile. | Currently covers only some preferences; see findings. |

VibeShine uses **XML 410** for completed controls and confirmation requests.
Aurora currently distinguishes UUID and completion message. Do not treat every
410 as success or automatically repeat Terminate.

Local references:

- `core/libgamestream/src/xml.c`: `xml_app_is_remote_control`, `xml_app_is_remote_display`.
- `core/libgamestream/src/client.c`: `gs_start_app_ex`.
- `src/app/ui/launcher/apps.controller.c`: `item_click_cb`, `open_context_menu`.
- `src/app/ui/streaming/streaming.controller.c`: `exit_streaming`, `suspend_streaming`.
- `../vibeshine/src/remote_session.cpp`: `project`, `dispatch`, `synthetic_running_game`, `successful_control_completion`.
- `../vibeshine/src/nvhttp.cpp`: Terminate confirmation and remote-role activation.

## Findings in the reviewed options

These findings describe the 2026-10-18 inspection. Consult
[CURRENT_STATE.md](CURRENT_STATE.md) for later fixes.

1. **Quit the game when the stream ends has no runtime consumer.**
   `quitappafter` appears in declaration, default, INI read/write, and Host
   checkbox, but not teardown. Effective behavior follows `session_interrupt`'s
   argument. The UI promises behavior the code does not execute. Fixing/removing
   it must preserve explicit Disconnect semantics and remote roles.

2. **Incomplete streaming profiles.** `profile_from_settings` and
   `profile_to_settings` in `src/app/profile/profile_manager.c` only copy
   resolution, integer FPS, bitrate, rotation, HDR, HEVC. AV1, 10-bit SDR,
   fractional/NTSC refresh, audio, backend, ABR are outside profiles. Switching
   to an HEVC profile can leave global AV1 enabled; `session_config_init`
   prioritizes AV1 and ignores HEVC in that case. Possible from static analysis;
   not yet reproduced on TV.

3. **Displayed and applied bitrate can differ.** UI permits 5–400 Mbps, while
   `session_config_init` clamps to `video_cap.maxBitrate`. NDL webOS5 advertises
   95 Mbps for SoCs classified high-end and 65 Mbps otherwise, with 45/35 Mbps
   suggestions. UI does not explain the clamp. Showing requested/limit/applied
   values would help. Local ABR already respects backend and host-returned clamps.

4. **Not every change announces reconnection.** Backend/audio call
   `module_changed_cb`, setting `needs_stream_reconnect`. Resolution/FPS call
   the then-empty `on_res_fps_updated`; HEVC/AV1/HDR/10-bit update visuals without
   the flag. Sessions copy configuration on creation. Next-connection notices
   should be consistent. Later Stream corrections are recorded in the baseline.

5. **Capabilities are not codec/mode-specific.** NDL `GetCapabilities`
   advertises H.264/HEVC/VP9/AV1, queries dimensions through HEVC, and supplies
   no `maxFps`. This proves neither AV1 120 nor every resolution/FPS combination.
   Policy needs codec-specific limits and measurements on the target model.

6. **Host supports 10-bit SDR; the local preference alone cannot select it.**
   Correction to initial analysis: `hdrMode=1` does not prove incorrect
   implementation. VibeShine also uses it to identify a Main10-negotiated client.
   Device `prefer_10bit_sdr=true` or application `prefer-10bit-sdr=true` suppresses
   effective HDR while retaining 10-bit encode. Application overrides device;
   explicit host HDR override has priority. Aurora requests Main10/`hdrMode=1`
   but does not send this administrative preference. UI should explain the
   corresponding VibeShine setting. With that preference off, normal HDR policy
   applies. Host implementation/policy tests exist; C2 NDL playback needs a
   physical test.

## What the settings do

| Option/group | Implementation and C2 implication |
| --- | --- |
| Resolution / FPS | Enter launch mode/protocol configuration. webOS caps requests at 120 FPS. Requested/submitted FPS does not prove displayed FPS. |
| HEVC / AV1 | Checkboxes exclude each other when clicked; H.264 remains fallback. AV1 wins if both flags arrive enabled through configuration/profiles. For 120 testing, prioritize HEVC and validate C2 presentation. |
| HDR | Depends on Main10 codec/backend capability; TV receives HDR10 metadata. HDR10 does not imply Dolby Vision. |
| 10-bit SDR | Requests Main10. Also configure VibeShine device/application SDR preference to obtain Main10 SDR. Host separates bit depth from effective HDR. |
| Full range YUV | SDR override; HDR uses limited range. NDL advertises `fullColorRange=false` and notes oversaturated-color risk. Keep off as a baseline until levels are validated. |
| Video backend | Selects an SS4S group. NDL/Starfish behave differently; switching adds no VRR. |
| Audio | Negotiates channels; NDL webOS5 advertises at most 6. Preserve implemented Opus 5.1 mapping and `surroundParams=642014523`. UI 7.1 availability does not prove backend support. |
| SOPS | VibeShine stores/exposes it as `SUNSHINE_CLIENT_ENABLE_SOPS` to processes. No automatic game-graphics adjustment conditional on it was found. Does not control VRR/Game Optimizer. Custom scripts can consume it. |
| Mute PC | Inverts `localAudioPlayMode` for normal sessions. Monitor forces host playback; client audio is separately controlled by `remote_monitor_mute_audio`. Input transmits no audio. |
| Auto resume | Tries to resume an advertised running app on Aurora startup/return, with reentry protection. May miss a role advertised as idle. |
| Stats / Compact stats | Controls display. Output FPS derives from backend submissions, not scanout. |
| NTSC refresh | Signals fractional 59.94/119.88 Hz rounded to centihertz. Neither VRR nor a direct physical C2 refresh measurement/change. |
| View-only | Blocks session input. Distinct from Remote Monitor; creates no virtual monitor. |
| Capture system keys | Controls forwarding of modifiers/keys. webOS forces grab while streaming for USB keys; not all system-reserved shortcuts are guaranteed capturable. |
| Hardware mouse / Absolute mouse | Selects device capture and absolute/relative positioning. Absolute suits desktop, relative suits camera games; depends on webOS device access. |
| Deadzone / Swap ABXY / Virtual mouse | Existing local input transformations, independent of Qt renderer. |
| Touchpad mode/speed/gestures/scroll | Converts controller touchpad events to native protocol or mouse with local adjustments; depends on SDL-exposed TV events. |
| Battery / Rumble test | Advertises/sends capabilities through SDL. Rumble, sensors, LED, triggers depend on controller, connection, permissions, driver; C2 hardware alone is insufficient. |
| Game mode | Shown only with Homebrew Channel service. Attempts Luna/root picture/sound changes/restoration; distinct from HDMI ALLM and internal VRR. Historical tests are C5; verify C2 aliases. |
| HEVC sliced frames | Host slicing hint for pipelining; encoder-dependent, experimental, no guaranteed latency reduction. |
| Periodic decoder refresh | Periodic HEVC recovery IDRs. May increase bitrate spikes; does not correct panel cadence. |
| Adaptive bitrate / profiles | Adjusts bitrate for loss/pressure and local profile. Low latency is a bitrate policy independent of the fork's similarly named VRR preset; no presentation synchronization. |
| Language / Show logs / Reset | Local preferences. Reset neither undoes host settings nor replaces streaming tests. |

## VRR: what the fork actually offers

The fork has a pacing worker with an `IVrrFramePresenter` contract, DXGI/Composition,
Vulkan, and Metal backends, prepare/present feedback, and fallback when the
renderer cannot fulfill the contract. UI requires V-Sync; each platform selects
its native presentation mechanism.

On a 120 Hz screen, policy offers 120 FPS, a 116 FPS VRR recommendation, and
100 FPS. These derive from adaptive-presentation headroom. Do not import 116/100
as smoothness presets for the TV's fixed compositor.

Fork VRR presets set buffer limits of 0.5/1/4 frames for Low Latency/Balanced/Smooth,
targets of 99%/99.5%/99.95%, histories of 60/120/300 seconds, and default tolerance
0.5 ms. These are limits/targets, not guaranteed fixed latency. Reduce judder
and Absorb microstutter change timing and may add delay; they do not interpolate
frames.

`clientVrrRequested=1` tells VibeShine the client uses RTP timing for pacing.
In host auto mode it promotes requested virtual-monitor refresh from 4× FPS to
1000 Hz, subject to system limits/mode overrides. It also limits WGC pool to one
frame and native AMF encoder input queue to one frame without explicit override.
Sending it does not enable LG VRR; only send after implementing/validating the
corresponding client capability.

Aurora's NDL exposes `Play(buffer, size, pts)`, queue length/flush, drop threshold,
HDR metadata. It offers neither equivalent adaptive present nor per-frame
physical presentation confirmation. Do not repeat rejected PTS/pacing experiments
under another name. Prior C5 conclusions do not establish C2 behavior, but
constrain justified promises; see [streaming history](WEBOS_STREAMING_HISTORY.md).

## Adaptable features and priority

| Fork feature | Applicability | Priority |
| --- | --- | --- |
| Per-stage diagnostics/percentiles | Measure arrival/reassembly/feed/queue without calling submit physical presentation; export bounded reproducible captures. Aurora already has a diagnostic foundation. | High |
| Configuration/capability notices | Explain bitrate clamp, effective codec, mode ceiling, reconnection; fix profiles/ineffective options. | High |
| Stable source cadence | Align host/stream to measured 60/120 or fractional family without misusing VRR signaling. | High |
| Connection/decoder calibration | Reuse loss, jitter, queue-growth, processing-cost test ideas. Fork probe APIs are PyroWave-specific and require negotiation; do not blindly copy to HEVC. | Medium |
| Controller diagnostics | Actual capabilities, battery, rumble, sensors, permission tests; partly present in Aurora. | Medium |
| Adaptive pacing / microstutter absorption | Experiment only after obtaining real backend observability/control; desktop algorithms alone cannot solve NDL presentation. | Research |
| Internal-app panel VRR | No demonstrated support in current path; LG documents HDMI VRR/G-Sync. No functional checkbox without another backend/API and hardware proof. | No demonstrated viable implementation |
| PyroWave | Custom GPU codec, Vulkan/Metal path, high network demand. NDL cannot decode it; SS4S has no driver. Needs new graphics/decoder path and SoC research. | Outside current practical scope |
| YUV 4:4:4 | Desktop client support does not prove NDL 4:4:4 decoding. Do not advertise without specific profiles/capabilities/tests. | Unvalidated |
| High-performance GPU power | Fork implementation targets amdgpu/Linux, not LG SoC. Aurora's own priority/root adjustments need separate evaluation. | Do not port directly |

## Addendum: resolved VibeShine behavior

### 10-bit SDR, HDR, precedence

Full flow: `nvhttp.cpp:make_launch_session` → `hdr_request_policy.h`/`rtsp.h`
→ `rtsp.cpp` → `video_colorspace.cpp`. Under automatic policy:

| Client request | Effective 10-bit SDR preference | Host result |
| --- | --- | --- |
| SDR / 8-bit decoder | On or off | Preference does not promote an 8-bit client to Main10. |
| Main10 / `hdrMode=1` | On | Main10 SDR, effective HDR off; HEVC/AV1 Main10 must be available. |
| Main10 / `hdrMode=1` | Off | Normal HDR request; result also depends on display/other policies. |

`force_on` enables HDR and clears SDR preference; `force_off` prevents HDR but
is not the Main10-requesting option. RTX HDR conversion and Vulkan HDR layer
also check SDR preference to avoid HDR conversion for these sessions. 10-bit SDR
may use Rec.709 or BT.2020 SDR depending on negotiation; BT.2020 SDR is distinct
from BT.2020/PQ HDR10.

### Catalog, resume, removal

The server advertises secondary clients as idle and includes a synthetic resume
shortcut beside the configured application. This allows attaching Monitor/Input
without Moonlight's generic game-replacement warning. `currentgame=0` therefore
does not prove no application is running. It also explains a local Auto resume
limitation: Aurora depends on the host advertising a current application.

Replacing a normal application has its own confirmation, distinct from Terminate:
`remote_monitor_confirm_app_replacement=true` by default. First request returns
XML 410, second confirms within 60 seconds, bound to client/generation/target
application. With active remote roles, replacement returns 409 and asks for
Terminate before another application.

Defaults `remote_monitor_disconnect_on_stream_end=false` and
`remote_monitor_disconnect_on_client_disconnect=false` retain the monitor for
Resume. The first flag removes it when any role stream ends; the second on client
disconnect. Input instead forgets its owner at transport teardown. The roles do
not have identical retention lifecycles.

Monitor keeps host audio regardless of Mute PC. Default
`remote_monitor_mute_audio=false` allows remote-monitor audio; true suppresses
that role's audio capture/transmission.

### WGC capture and host VRR naming

`frame_limiter_auto_virtual_framegen` distinguishes disabled (base refresh),
legacy (2×), enabled/automatic (4×), vrr (fixed requested 1000 Hz virtual monitor).
In automatic mode, `clientVrrRequested=1` selects 1000 Hz policy. Stream FPS is
independent of this refresh. Selecting it alone generates no intermediate images.

WGC varies frame admission with activity: up to 2× stream FPS on desktop and
4× in game under this policy. `wgc` is variable capture; `wgcc` is constant,
resubmitting the last capture on timeout. Low FPS on a static desktop can be
normal in variable mode. Constant mode can maintain transmission without new
motion but alone does not fix in-game judder.

`/bitrate` returns the applied value, clamps to host-configured ceiling and an
absolute 500 Mbps, and acts on authenticated-client sessions. Server ABR capability
is advertised `supported:false`; adaptive decisions remain in Aurora, which
uses this endpoint to apply changes.

Main local sources: `../vibeshine/src/remote_session.cpp`,
`../vibeshine/src/nvhttp.cpp`, `../vibeshine/src/rtsp.h`,
`../vibeshine/src/rtsp.cpp`, `../vibeshine/src/video_colorspace.cpp`,
`../vibeshine/src/config.h`, `../vibeshine/src/config.cpp`,
`../vibeshine/src/platform/windows/display_wgc.cpp`,
`../vibeshine/src/platform/windows/wgc_capture_policy.h`,
`../vibeshine/src/platform/windows/display_vram.cpp`.

Second inspection compiled/ran existing `test_remote_session.cpp`,
`test_hdr_request_policy.cpp`, `test_framegen_policy.cpp` with corresponding
production code and bundled GoogleTest: Clang/macOS, C++20, ASan/UBSan,
**48 tests passed**. Existing
`tests/unit/platform/windows/test_wgc_source_admission.py` replay with Clang and
modeled clocks/WinRT: **794 checks, zero failures**. This validates policies/frame
admission, not actual WGC/DXGI, PC capture latency, or C2 presentation.

## Concrete log clue and recommended validation

The supplied log records several `2560x1440x90`, `93000 kbps`, NDL webOS5,
Low latency ABR sessions, without reliably identifying model/firmware. With
fixed 120 Hz presentation, 90 FPS does not divide panel intervals evenly; one
possible distribution alternates 8.33 and 16.67 ms holds. This is a plausible
judder cause, not proof for every logged stall. Effective video-path refresh
still needs measurement.

To isolate behavior, compare the same pan in HEVC at 60/120 FPS, ABR off,
conservative fixed initial bitrate such as 35–45 Mbps, same resolution. Record
host mode, effective codec, loss, queue, feed stalls, screen video; then vary HDR
and resolution separately. This bitrate is a comparison starting point, not a
universal limit/quality promise. Only then increase bitrate or enable ABR.

LG's published webOS TV 22 UHD player limits list HEVC Main/Main10 4K60 up to
60 Mbps and AV1 4K60 up to 50 Mbps. These do not establish DirectVideo ceilings
for homebrew or justify internal 4K120 promises from HDMI 4K120 capability.

Executed in this review: `tests/core/run_vibeshine_protocol.sh` in
`aurora-ui-sim:local`, Linux, read-only source mount, no network.
**VibeShine protocol tests passed**, ASan/UBSan. Fixtures cover parsing, UUIDs,
HDR resume, completed controls, confirmation without automatic repeat, applied
bitrate. They do not validate motion, VRR, physical audio, or C2 decoder.

## External sources

- [Reviewed fork at fixed commit](https://github.com/Nonary/moonlight-qt/tree/3ae5f3b0e8dfb7538422f2920d38126385d9efb4).
- [VRR presenter contract](https://github.com/Nonary/moonlight-qt/blob/3ae5f3b0e8dfb7538422f2920d38126385d9efb4/app/streaming/video/ffmpeg-renderers/ivrrframepresenter.h).
- [VRR FPS policy](https://github.com/Nonary/moonlight-qt/blob/3ae5f3b0e8dfb7538422f2920d38126385d9efb4/app/streaming/vrrratepolicy.h).
- [Timing presets](https://github.com/Nonary/moonlight-qt/blob/3ae5f3b0e8dfb7538422f2920d38126385d9efb4/app/settings/vrrtimingoptions.h).
- [VibeShine launch signaling](https://github.com/Nonary/moonlight-qt/blob/3ae5f3b0e8dfb7538422f2920d38126385d9efb4/app/backend/nvhttp.cpp).
- [PyroWave and negotiation](https://github.com/Nonary/moonlight-qt/blob/3ae5f3b0e8dfb7538422f2920d38126385d9efb4/docs/pyrowave-protocol.md).
- [GPU priorities/power](https://github.com/Nonary/moonlight-qt/blob/3ae5f3b0e8dfb7538422f2920d38126385d9efb4/docs/stream-performance.md).
- [LG: VRR/G-Sync requires HDMI](https://www.lg.com/us/support/help-library/lg-oled-tv-settings-do-you-want-to-know-how-to-enable-vrr-gsync--20154713308922).
- [LG: published webOS TV 22 formats/limits](https://webostv.developer.lge.com/develop/specifications/video-audio-220).
