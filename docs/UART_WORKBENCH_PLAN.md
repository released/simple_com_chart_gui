# Native UART workbench implementation plan

User approved the COM/UART feature recommendations on 2026-10-25 and requires
two simultaneously visible, vertically stacked independent port groups. This
extends the existing native application; completed UART/optimization work is
preserved. Python, network login, MCU firmware and Git publication are out of scope.
User explicitly confirmed that items6..9 are included too; finish every batch,
not only1..5/dual COM. Later priority does not mean deferred outside this task.

## Scope and sequence

1. Dual COM and packets: upper A follows the existing primary connection/chart;
   lower B has an independent connection. Each group has its own visible receive
   area, controls, display state, clear/save and recording. Never merge A/B logs.
   Include direction, receipt time, gap, length and HEX/ASCII packet inspection.
2. Binary frame extraction: bounded streaming frame assembly, configured header/
   length, typed/endian fields and checksum; expose actual decoded primary values
   to charts without changing raw recording or text-parser compatibility.
3. Commands and automation: named/grouped portable command panel with validated
   import/export; per-step send/expect/timeout/result scripts; character/line pacing
   across explicit sending paths. Preserve bounded generation/partial-write rules.
4. Transfer protocols: explicit XMODEM checksum/CRC/1K and YMODEM file send/receive,
   bounded retries, protocol progress/cancel and safe output replacement. This is
   PC-side serial transfer, not target firmware implementation or acceptance.
5. Displays and protocol tools: numeric/gauge/bar/LED/FFT views with honest time
   provenance; opt-in VT100-style terminal controls; Modbus RTU register polling
   and decoding. Keep these independent of normal raw receive monitoring.

## Acceptance

- [x] A/B groups remain simultaneously visible at supported window sizes/fonts.
- [x] Concurrent distinct A/B fixture streams, display/clear/save/recording and
      connection failure/cancel cannot affect the other group's data or session.
- [x] Refuse duplicate physical-port ownership; startup does not open hardware.
- [x] Packet inspection and binary extraction work across arbitrary read splits,
      malformed frames, bounds, reconnect and independent raw-recording paths.
- [x] Commands/scripts/pacing support explicit cancellation, meaningful per-step
      results and validated private-data-safe import/export; no automatic TX.
- [x] Protocol file send/receive handles ACK/NAK/timeout/cancel/errors without
      misreporting driver acceptance as target success or damaging existing files.
- [x] Added displays/terminal/RTU operations work through actual native controls;
      sparse/invalid timing/data never create fabricated measurements.
- [x] Release/Debug compile and affected core/UI regression checks pass. Use fake
      transports, isolated INIs/files and owned test processes only. Inspect the
      new split view screenshots and record remaining physical acceptance gaps.
- [x] Production contains no demo/test feeder, extra installer/runtime dependency
      or unrelated Python/MCU changes. Record EXE size and preserve personal INI.

## Design and verification constraints

Retain single-instance behavior, static MFC/CRT, standard controls and grey UI.
Use separate core/session/UI modules; no unbounded buffers, UI-thread driver waits
or hidden-page rebuilds at receive rate. Timestamps are PC receipt times unless
explicit source data provides a reliable clock. Windows pacing is best-effort,
not a physical timing guarantee. FFT requires supported sampling assumptions.

New test data exists only in separate verification builds/fixtures. Test modified
and affected paths incrementally; do not replay the entire historical checklist.
Real UART/USB/driver/pin/timing/hardware acceptance remains NOT RUN unless actually
performed. No unvalidated MCU-related implementation/assets are published.

## Status

Delivered as native1.0.1.025. Formal Debug/Release builds, selected core groups,
native Workbench/default/enlarged-font checks, isolated dual recordings and actual
retail startup/import/size/private-INI checks pass. See HANDOFF/SELF_CHECK for exact
artifacts, commands, results and physical/long-duration acceptance gaps. No Git
publication or MCU implementation/assets are part of this task.
