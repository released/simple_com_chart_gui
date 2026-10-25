# UART debug implementation plan

Approved 2026-10-24. Target: portable native MFC application. Python UI is unchanged.
No Telnet/SSH/network login, firmware-specific event triggers, or automatic target reset.

## Acceptance and delivery sequence

Each batch must build, pass focused software checks, and update this record before delivery.
Keep all existing capture/filter/export/reconnect contracts unless explicitly superseded below.

### A - Receive and recording

- [x] Continuous recording independent of display retention/filter/tab/clear; raw bytes and timestamped text, size/date rotation, observable disk/queue errors, safe stop/drain.
- [x] PC receipt timestamps, elapsed time and inter-line delta; never imply device or wire timing.
- [x] Message search (literal/limited safe regex), next/previous, keyword highlighting and session bookmarks.
- [x] UTF-8/ANSI text decoding, newline/wrap options, optional foreground ANSI SGR display; ASCII/HEX byte inspection and existing Save As remain.

### B - UART operations and offline analysis

Implementation sequence: B1 = TX / flow-lines / statistics (1.0.1.020); B2 = offline replay / named profiles (delivered, 1.0.1.021). RX and operation workers are independent; stopping is a cancellable UI state, not a UI-thread join of an unfinished driver request. Late/partial operation results must never be replayed into a new connection.

- [x] Explicit Text UTF-8/HEX TX, line endings, history/presets and direction; workers only, no automatic replay after reconnect.
- [x] Flow control, DTR/RTS policy, input-pin status and explicit Send Break; retain compatible defaults until changed by user.
- [x] RX/TX traffic and driver error statistics, separated from UI-history eviction/software input loss.
- [x] Bounded background offline import/replay with pause/speed/seek and honest time provenance, feeding the existing parser/charts.
- [x] Named portable profiles with validated import/export and no implicit hardware open or transmission.

### C - Analysis and explicit automation

- [x] Chart cursors/delta, interval statistics/threshold reference lines, time-position navigation.
- [x] Opt-in periodic/batch TX, reply/timeout sequencing, generic automatic response and checksum/CRC; stop on disconnect, bounded retries, no implicit resume.

## Safety and verification

- Record raw RX before UI queuing. Never describe a bounded buffer as lossless; report recorder overflow and disk failures.
- Raw files do not contain synthetic timestamps, TX or connection markers. Separate metadata/text may contain these.
- Display pause, scroll-off, Clear and chart Apply do not stop recording.
- Tests use fixture files and fake transports only. Physical UART timing, electrical pin effects and throughput are separate NOT RUN hardware checks unless actually exercised.
- Do not publish private INI/log/profile/recordings, or push GitHub without a current publication request.

## Progress

- Baseline inspected: 1.0.1.018, clean worktree at entry.
- Batch A implemented as 1.0.1.019; Debug/Release native builds and focused software verification pass. See SELF_CHECK.md / HANDOFF.md for evidence and hardware limitations. Existing personal INI preserved; no GitHub push.
- Batch B1 implemented as 1.0.1.020. Added UART Console, bounded generation-bound operation queue, independent RX/operation workers, overlapped native I/O, nonblocking stop/close, text/HEX/history/presets, flow/lines/Break and statistics. Debug/Release builds, hardware-free core/pipe tests, focused fake-transport UI and affected receive/recovery regressions pass; physical UART acceptance remains NOT RUN.
- B2/C delivered as 1.0.1.021: independent advanced-tools window for replay/profiles/capture analysis/finite automation. Debug/Release builds, 4/4 bundled native tests, focused Advanced UI checks (Debug / Release enlarged fonts) and affected Serial/Uart/UartOperations/Charts checks pass. The accepted software feature checklist is complete; physical UART/pins/precision timing/DPI/file-dialog mouse acceptance is NOT RUN, not implied by fake/fixture tests. See ADVANCED_UART_TOOLS.md and latest SELF_CHECK/HANDOFF for exact limits. No private INI change or GitHub push.
- Batch A limits: readable continuous log escapes non-ASCII bytes (raw `.bin` preserves original encoding); rotation happens between read chunks; receipt time is not hardware timing; bookmarks expire with retained display; search Regex is a limited safe subset; ANSI rendering supports foreground SGR, not a complete terminal emulator. Native file-dialog mouse interaction and physical UART acceptance are NOT RUN.
