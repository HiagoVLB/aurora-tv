# VibeShine session and catalogue integration

## 1.3.3 black game image and audio review (2026-10-22)

Aurora `454e6b25888201816fa52ffb4720c67bd92cbdfb` plus local version/docs
changes was compared with the clean local VibeShine checkout
`97835dbfb14faafd8b4f13d98bc00cc921d45296`; remote freshness was not checked.
The snapshot is `.cache/comparisons/vibeshine-state.json`. No installed host
version can be established from the TV log. See the dated incident in
[DIAGNOSTIC_LOG_REVIEW.md](DIAGNOSTIC_LOG_REVIEW.md) for observations/timestamps.

The trace contains one successful launch/RTSP setup, audio queue overflows,
subsequent loss of new video submissions and finally an unexpected control
disconnect. Windows still responded to its Windows key; the user's black-game
report does not establish a system hang. No automatic repeated launch, `/cancel`,
decoder-feed rejection or Aurora fatal signal appears in this trace.

VibeShine's `nvhttp.cpp:setBitrate` validates identity and bitrate, then
`stream.cpp:set_bitrate_for_sessions` queues a change for that client's encoder.
`video.cpp` coalesces requests and either applies the bitrate live or rebuilds
an encoder that cannot apply it. The capability endpoint reports server-side
ABR unsupported, matching Aurora's local control in this trace. These paths
do not directly request a display-mode or audio-device change; whether encoder
reconfiguration failed in the installed host requires its log. Aurora does
have a demonstrated stale-summary ABR issue: it probes upward after new video
stops. That correction remains pending and is not a proven game-black-screen
cause.

Stereo audio open and first-packet receipt succeed on the TV. Queue overflow
does not identify whether the host captured silence, the audio consumer stalled
or packets arrived in bursts. The host has explicit sink selection, capture-only,
virtual sink and retained-ownership behavior; none can be diagnosed as the cause
without host configuration/logs. Request the VibeShine log export for
**20:17:45–20:20:05 São Paulo (23:17:45–23:20:05 UTC)**, including capture,
encoder and audio messages. The Logs/Troubleshooting export uses
`/api/logs/export` on Windows. No host/client behavior changed in this review.

## Evidence from the supplied TV log

At 2026-10-18 21:49:03.361 UTC (18:49:03 in São Paulo), Aurora records
`Interrupt source=user reason=0 quit_app=1 state=2`. At 21:49:03.853 the
host acknowledges the quit request with `ret=0`. This is the explicit
“Quit game” action, which also ends a configured Desktop application/session.
The TV log contains no decoder feed error or stack trace proving a crash here.
The later host-offline warnings do not establish why VibeShine became unreachable.

There are separate, reproducible client defects in this trace: ABR sends a
97,650 kbps update after teardown starts, above the decoder's advertised
95,000 kbps maximum, and attempts another request after `/cancel`.
Video statistics also retain samples from earlier sessions during startup.
Low frame rate while transmitting a static desktop is not by itself evidence
of a failed decoder or host capture pipeline.

## Implemented behavior

- “End session” asks for confirmation and explains that the host may close its
  configured application or Desktop session and restore its display.
  “Disconnect” leaves that application/session running.
- ABR stops and joins before `LiStopConnection`, without restoring a bitrate
  belonging to the departing session. It respects the selected decoder cap.
- `/bitrate` checks the XML status and uses the returned applied bitrate.
  A host clamp lowers the subsequent probe ceiling. HTTP 200 alone is not success.
- The application catalogue preserves VibeShine's UUID and ArtVersion metadata.
  Launch requests carry the stable UUID when present. Older hosts without this
  metadata still use numeric IDs. Artwork cache invalidation is unchanged.
- The launcher permits the host's advertised Remote Monitor, Remote Input,
  Resume, Disconnect and Terminate controls while another app is active, using
  the reserved UUIDs from VibeShine `src/remote_session.cpp`; it does not guess
  roles from names or arbitrary IDs. Normal application switching keeps its
  existing restriction.
- Remote Monitor/Input streams disable global “End session”: `/cancel` can end
  the configured game rather than the remote role. Disconnect leaves the remote
  display available. Use the advertised Disconnect Monitor/Input catalogue item
  to release that retained role.
- The same running application resumes through `/resume`, including HDR, rather
  than forcing a fresh `/launch` just because HDR is enabled.
- VibeShine's documented successful control completions (410 with their exact
  completion message and matching UUID) return to the launcher without attempting
  RTSP. A 410 confirmation prompt remains an error/message requiring a new user
  action; Aurora never automatically resubmits a destructive request.
- New video sessions clear previous summary samples before receiving frames.

The local VibeShine implementation used as reference is `src/nvhttp.cpp`
(`/applist`, `/launch`, `/resume`, `/cancel`, `/bitrate`, `/api/abr/capabilities`)
and `src/remote_session.cpp`. Server-side ABR is explicitly unsupported there;
Aurora retains its local controller. Global monitor selection, placement and
per-client virtual-display policies remain configured in VibeShine. Its admin
web API uses separate authentication and is not called with pairing credentials.

## Validation

`tests/core/run_vibeshine_protocol.sh` runs real client/XML code against fixture
HTTP responses, with ASan and UBSan on Linux. It covers applied bitrate/clamping,
HTTP-success/XML-failure, malformed responses, failed/successful cancel, optional
catalogue metadata, role recognition, HDR resume with UUID, completed controls,
and confirmation prompts without automatic retry. Crypto and network I/O are
mocked only in this test executable. The same fixture target is registered in
CTest for Linux builds.

The webOS Release build produces the ARM installer. A successful build and these
protocol fixtures do not validate LG C2 decoder behavior, virtual-display driver
behavior, remote-control navigation or host cleanup on an actual TV/PC pair.
