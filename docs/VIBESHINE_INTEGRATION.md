# VibeShine session and catalogue integration

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
