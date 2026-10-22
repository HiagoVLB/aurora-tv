# webOS diagnostic log review and upstream comparison

## Log reset on every installation (2026-10-22)

At the user's request, each webOS IPK now ships `aurora-logs-reset.pending` in
the application root. Before opening diagnostic writers, the first launch
checks this marker, removes only `aurora.log`, `aurora-crash.log` and
`aurora-run.state` from `$HOME/conf` and `/tmp/moonlight-tv/conf`, then consumes
the marker. Installing the same IPK again restores the same marker and causes
another reset, without relying on version changes or build timestamps.
Ordinary launches, including reopening after a crash, retain the current
installation's history. Cleanup errors are logged and leave the marker pending
for retry. Settings, profiles, pairing keys and unrelated files are excluded.
Diagnostics are unlinked by exact name through directory descriptors, so log
symlinks/hard links do not truncate their targets. Symlinked directories and
invalid/nonregular marker files are refused. The IPK verifier requires the
marker before publishing an installer. Actual package-manager/permission
behavior on the C2 still requires validation after installation.

## C2 incident and persistent fatal diagnostics (2026-10-22)

The user supplied an Aurora log spanning 2026-10-22 00:07–01:46 UTC
(2026-10-21 21:07–22:46 in São Paulo). Two distinct events must not be conflated:

- At 00:27:29.185 UTC logging stops without interruption, teardown or shutdown
  records. At 00:27:39.470 UTC a new Aurora 1.3.1 process starts, followed by
  `/resume` with the same current game. The preceding video sample reports no
  feed errors or frame gaps. This supports an abrupt process interruption but
  supplies neither a native crash stack nor its cause.
- At 01:46:52 UTC the receiver reports frame losses/IDR recovery. At
  01:46:55.575 UTC NDL rejects AV1 feed with `-1`; Aurora interrupts with
  `source=decoder reason=4099 quit_app=0`, completes connection teardown and
  records `Shutdown complete`. Temporal proximity does not establish why the
  decoder failed, nor explain the earlier abrupt interruption.

The restart identifies webOS **10.3.1 (HE_DTV_W22O_AFABATPU)**, NDL video/audio,
2560×1440 at 90 FPS, AV1 8-bit, HDR disabled, requested bitrate 95000 kbps and
ABR mode 2. These are incident runtime settings, not new hardware validation.
The user had both Aurora 1.3.1 and upstream Moonlight 1.6.36 installed; the
supplied log and `/proc` examination identify Aurora's `bin/aurora` process.

Terminal checks at 13:44 São Paulo on 22/10 show uptime 8h56: the current boot
followed the incident. `/tmp/faultmanager/crash/` is readable and empty;
`/tmp/var/log/reports/librdx/` does not exist. `dmesg` is denied to prisoner,
and traditional messages/syslog files are absent. `/proc/sys/kernel/core_pattern`
contains `core`. The live Aurora process's `/proc/<pid>/limits` confirms both
core-size limits are zero. Current empty temporary directories cannot establish
whether a report existed before reboot. The desktop manager also stops at the
first existing report directory, even if it is empty; that limitation was not
the explanation in this inspection, because the second directory was absent.

The new webOS diagnostics keep a bounded **64 KiB `aurora-crash.log`** and a
**511-byte `aurora-run.state`** beside the ordinary log. File descriptors are
opened before a failure, close-on-exec, readable through Dev Manager (`0644`)
and checked for symlinks, shared inodes and non-regular files. The last fatal
report survives clean restarts. A running/clean/crashed marker is synced at
lifecycle boundaries; the next launch reports an incomplete preceding shutdown
in `aurora.log`, without assigning a cause. Boot ID/PID identify matching runs.

Fatal SIGSEGV/BUS/ILL/FPE/ABRT/TRAP capture uses fixed buffers and signal-safe
operations, records timestamps, signal code, hardware fault address when
available, instruction/stack/link registers and Linux executable mappings, then
re-raises the original signal. No stdio, normal logger, allocator, unwinder,
mutex, raw stack or heap contents are used in the handler. Map scanning is
bounded to 1 MiB input and the report's 64 KiB output cap. The main thread has
a 64 KiB alternate signal stack; other threads can capture faults using their
normal stack, but their stack exhaustion is not covered by that alternate stack.
Full core dumps stay disabled outside Debug. This is intentional text capture,
not a replacement native stack trace or proof of the original crash cause.

SIGKILL, loss of power, failure before initialization, severe corruption and
storage errors remain best-effort gaps. An unclean marker may also follow a
forced stop or reboot; a retained fatal report must match the run identity.
Fallback `/tmp` storage is explicitly nonpersistent. The app additionally logs
SDL background/foreground/close/quit events, exit requests and shutdown begin.
Existing stream/disconnect/cancel behavior is unchanged.

Validation and package receipt for this change are tracked in
[CURRENT_STATE.md](CURRENT_STATE.md). The signal-safety design follows
[signal-safety(7)](https://man7.org/linux/man-pages/man7/signal-safety.7.html),
[sigaction(2)](https://man7.org/linux/man-pages/man2/sigaction.2.html) and
[sigaltstack(2)](https://man7.org/linux/man-pages/man2/sigaltstack.2.html).

## Historical review (2026-10-18)

Reviewed on 2026-10-18. Reference: mariotaku/moonlight-tv commit
`cf96ecc72cdab3aae7ba0698f463b89065ae7e3b`, including its pinned SS4S
`dfba721b85420ccabf91dac65be73984bf1865f9` and commons-c
`26a8eb70f9ef374887f43618a9f9d2fd273c4c84`.

## Storage and execution environment

Aurora is a native app (`type: native`), using SDL and SS4S/NDL. LG's public
JavaScript storage/lifecycle examples cannot be used as a specification for its
native filesystem access or decoder behavior.

The [upstream webOS path implementation](https://github.com/mariotaku/moonlight-tv/blob/cf96ecc72cdab3aae7ba0698f463b89065ae7e3b/src/app/platform/webos/path_webos.c)
uses `$HOME/conf`, falling back to `/tmp/moonlight-tv/conf` if the configuration
directory is not writable. Aurora retains that implementation. The diagnostic
file uses the same directory, without attempting to write system logs or modify
jailer permissions. The startup record identifies the actual file path and
whether it is persistent.

The [webOS Homebrew filesystem guide](https://www.webosbrew.org/develop/guides/filesystem/)
documents app installation under `/media/developer/apps/usr/palm/applications`,
older webOS app-directory permission problems, and `/tmp` being cleared on boot.
The log now also attempts the temporary fallback if opening the file itself fails,
not only if directory creation fails. Persistent history cannot be guaranteed
when the writable fallback is `/tmp`, after uninstall, or after removal of
Developer Mode applications.

The writer preserves one filename and inode, compacts complete recent records
in place, and stays within 2 MiB. Its queue is bounded (256 records, 1 KiB each),
and the retention buffer is bounded to 1 MiB during compaction. The background
writer has an explicit 128 KiB stack, excludes DEBUG/VERBOSE messages, closes its
file descriptor on exec and flushes each completed record to the OS. This is not
an atomic journal: a kill during compaction or loss of power can lose history;
normal shutdown drains the queue. It does not claim to capture native stack traces.

New video diagnostics perform a bounded snapshot attempt rather than spinning on
the UI thread. Atomic live markers identify whether the submit callback has been
entered recently and whether a player feed is still running, even if no fresh
statistics window has completed. These markers describe client callbacks, not
raw network arrival or actual frames presented by the TV.

## Differences relevant to the failing session

The host log matches the Aurora excerpt at 14:31:54 and 14:32:00 through exact
runtime bitrate values `34894 -> 24425 -> 25646`. It identifies a 1440p90 AV1 HDR
stream on an RX 9070 XT. WGC publishes about 11 FPS in that session, but the logs
do not establish why capture slowed or prove that the client could not trigger
the failure. There is no supplied Aurora native stack trace.

1. **Bitrate ceiling and ABR: strongest concrete divergence.** The
   [upstream session configuration](https://github.com/mariotaku/moonlight-tv/blob/cf96ecc72cdab3aae7ba0698f463b89065ae7e3b/src/app/stream/session.c)
   clamps the requested bitrate to the backend's `maxBitrate`. Aurora disables
   that clamp on webOS and adds a separate adaptive-bitrate controller. The pinned
   [NDL backend capabilities](https://github.com/mariotaku/ss4s/blob/dfba721b85420ccabf91dac65be73984bf1865f9/modules/webos/ndl/webos5/ndl_video.c)
   report 65 or 95 Mbps depending on the SoC classification; Aurora retains these
   values but its ABR does not honor them. The failing session receives a client
   request for 107658 kbps, above both values. This is a testable overload /
   interoperability hypothesis, not proof of the crash cause. New startup logs
   record the actual selected backend's limits alongside requested settings.

2. **Codec negotiation differs.** Upstream can advertise both HEVC and AV1 when
   both are enabled. Aurora deliberately excludes HEVC when AV1 is selected.
   A comparison using the same UI checkbox states may therefore negotiate a
   different codec. Compare the actual negotiated codec, resolution, FPS, HDR,
   bitrate and decoder backend in both applications.

3. **Teardown has an additional concurrent actor.** Upstream stops the connection
   before sending `/cancel`. That ordering itself is inherited, not an Aurora-only
   defect. Aurora adds an ABR worker which remains alive until after connection
   stop, app quit and status refresh. It can still query transport metrics or
   request bitrate changes during teardown. HTTP calls sharing its GS_CLIENT are
   protected by a mutex, so this is not evidence of unsynchronized cURL use;
   serialized requests can still delay one another. Stage start/end and elapsed
   time records now reveal where teardown stops progressing. No change to teardown
   behavior was made as part of this diagnostic review.

4. **AV1 timestamps and NDL lifecycle differ.** Aurora adds an AV1 unique-ms PTS
   path and substantial NDL startup/lifecycle changes. Upstream simply supplies
   its player clock to `NDL_DirectVideoPlay`. These are candidates for controlled
   comparison, not grounds to repeat pacing experiments already ruled out in
   AGENTS.md. Both applications use direct submit; disabling it is not an
   upstream-equivalent fix.

5. **The latency estimate is inherited.** The pinned upstream NDL implementation
   also substitutes half a frame interval when render-buffer length is zero.
   `D=31 ms` at about 16 FPS is therefore not independent evidence of slow decoder
   execution, nor an Aurora-only regression. The file labels it
   `player_latency_est_ms` and separately records actual feed-call duration.

[LG's webOS 25 video specification](https://webostv.developer.lge.com/develop/specifications/video-audio-250)
lists AV1 4K60 at 50 Mbps for UHD models. That is a public playback specification,
not a direct NDL API benchmark or proof that a specific model cannot do 1440p90.
Likewise, older upstream FAQ bitrate guidance must not be treated as a universal
hard limit for every recent TV. Model, firmware, selected backend and negotiated
codec must accompany the next reproduction.

The [upstream troubleshooting guide](https://github.com/mariotaku/moonlight-tv/wiki/Troubleshooting)
separates native crash reports from application output. An empty Dev Manager crash
list alone does not establish that a disconnect was a native crash.

## Validation and remaining work

The standalone writer test passes with AddressSanitizer + UndefinedBehaviorSanitizer
and separately with ThreadSanitizer on the local Mac. It exercises size limits,
multiple in-place compactions, retained sessions, oversized startup recovery,
concurrent producers, overload recovery, record boundaries and DEBUG exclusion.
These runs validate the writer, not all of Aurora's pre-existing concurrency.

The complete webOS build and runtime validation have not run in this environment:
CMake/SDK are unavailable and the Docker daemon is not running. In particular,
actual file accessibility through the TV's jail and Dev Manager must be checked
on hardware after installation. The diagnostic changes do not fix or reproduce
the streaming failure by themselves.

For the next comparison, keep host settings and scene unchanged. First compare
Aurora ABR on/off while preserving the other negotiated settings. If only ABR-on
fails, investigate its requests/ceilings first. Then compare Aurora and upstream
with matched fixed bitrate, codec, FPS, HDR and backend. If changing AV1 to HEVC,
change only the codec for that comparison. Do not infer a network fault solely
from frame-number gaps or a decoder fault solely from `D`.


## Confirmed device model

The user identified the TV as **LG OLED42C2**. The installed webOS and Aurora
versions remain unknown. [LG's model page](https://www.lg.com/br/televisores-e-colunas-de-som/oled/oled42c2psa/)
confirms the 120 Hz panel and alpha 9 Gen 5 processor. Those display/input features
do not establish the maximum AV1 rate of a native app. The
[webOS 22 playback specification](https://webostv.developer.lge.com/develop/specifications/video-audio-220)
is the generation-specific reference, not the webOS 25 table used initially;
it likewise lists UHD AV1 at 4K60 and 50 Mbps. Neither page establishes the TV's
currently installed firmware or a tested 1440p90 NDL limit.

SS4S classifies machine names `o22` and later as high-end and then advertises
95 Mbps for NDL. This remains conditional until the runtime machine/backend
identification is captured; do not infer it solely from the commercial model name.
The failing 107658 kbps ABR request exceeds both possible NDL ceilings. Existing
C5-specific findings in AGENTS.md must not be presented as hardware tests on C2.


## Download permission repair

The initial writer created `aurora.log` with mode `0600`. After installation, the
user reported permission denied specifically when downloading the file. Different
native-app and SSH identities are a plausible cause; the actual owner and mode
on the TV have not been inspected because remote access was unavailable.

The writer now explicitly applies `0644` to the opened diagnostic file, including
existing logs, so a restrictive umask does not defeat export. Group/other write
permissions remain disabled. Symlinks and multiply-linked files are rejected
before changing permissions, preventing a log path from altering another file's
mode. Tests exercise creation with umask `0077`, repair of an existing `0600` log,
and unchanged target permissions when links are refused. DEBUG/VERBOSE exclusion,
single-file retention and the 2 MiB cap remain in place. Other local TV users can
read this diagnostic output; configuration and authentication files are not changed.

A complete ARM webOS build of the initial diagnostics subsequently succeeded,
and the resulting IPK's metadata, ELF architecture and embedded diagnostic markers
were checked. Validation on the actual C2 remains pending.
