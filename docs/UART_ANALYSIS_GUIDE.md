# UART signal and recording analysis

These functions use native Windows controls and run without Python. Open
**UART > Workbench / Dual COM** for the independent A/B monitor and its Tools menu.
Physical serial timing and target behavior still depend on the USB bridge/driver
and connected equipment. File analysis never sends the recording to a COM port.

## Dense waveforms

The Chart page keeps first/last values and both extrema for each horizontal pixel
when rendering dense data. Narrow positive and negative peaks remain visible;
capture, exports, cursor values and FFT still use the retained original samples.
This does not recover samples already lost by the device or retention limit.
Chart and table fields start unchecked; select the signals you need. Saved four
small-chart selections continue to restore, otherwise all four start at `(none)`.

## Port B text charts

In Workbench **Tools > Text charts B**, enable text charts and choose the independent
Include/Exclude and Key/value, CSV, Tab, JSON or Regex extractor. Applying these options
does not send anything. CSV/Tab needs explicit column names; Regex uses name/value
capture groups. Filters match complete raw lines; Exclude has priority.

Port B numeric keys appear as `B.<original key>` and start unchecked. The two raw
message panes and recordings remain separate. Numeric samples share the PC
receipt clock; disable a separate device-time field/group on the main extractor
before enabling mixed-source charting. A prefix collision with a literal Port A
key is reported and the conflicting numeric sample is omitted; raw data remains.
Binary/RTU/transfer-owned RX bypasses text extraction, and partial lines are reset
at generation changes/gaps. Applying the parser starts a fresh numeric capture.

## Offline binary replay

Disconnect both ports and finish file/recording/export work. Open **UART > Binary
replay schema**, select Binary and supply the header, fixed/length-based frame
size, byte order, checksum and numeric fields. For example `rpm,1,u16le` reads two
little-endian bytes at offset 1. Use Preview HEX before applying.

This schema is independent of the live A/B binary parsers. Apply pauses playback,
seeks to the beginning and clears the offline view. Use Offline replay to load
and Play the file. Recorded raw/index pairs use PC receipt intervals; unindexed
input uses the explicitly synthetic interval. Seek/reconnect boundaries discard
partial frames. CRC/discard counters remain observable; source files are read-only.

## Derived channels

Open **UART > Derived channels** and enter one definition per line, for example:

```text
power = voltage * current
error = target - actual
smooth = ema(voltage, 0.2)
adjusted = calc.power + abs(-2)
```

Outputs are named `calc.power`, `calc.error`, etc. Supported operations are
`+ - * /`, parentheses, `abs`, `sqrt`, `min`, `max` and `ema(value, alpha)` with
`0 < alpha <= 1`. Backticks quote a special field name. Refer only to an earlier
`calc.name`; cycles/forward references, executable functions and nonfinite results
are rejected or omitted. Inputs must exist in the same numeric frame: an absent
value is not replaced by zero or an older sample. A and B keep independent EMA
state; reconnect/new capture resets it. Use source-qualified B inputs, for example
`speed = B.rpm * 2`; its Port B output is `B.calc.speed`. Apply starts a fresh numeric capture.
Raw fields/recordings are preserved. Limits: 32 definitions, 8 KiB total, 128
expression nodes per definition and nesting depth 32.

## Binary field inspector

Workbench Tools opens **Field inspector A/B** on a retained packet snapshot.
Select a packet and a field to highlight its exact bytes. The table shows offsets,
types, values and truncated-field errors; checksum diagnostics show calculated
and received values. The inspector is read-only and never changes the live schema.
Optional bit projections use one CSV line per field:

```text
state,1,u16le,0,2,0=OFF|1=ON
```

That reads a two-bit field starting at bit 0 of a little-endian word at byte 1.
Supported containers are u8/u16le/u16be/u32le/u32be; at most 32 projections.

## Numeric script assertions and results

In the Workbench command/script window, edit a step's optional **Numeric key**,
minimum and maximum. After that step starts, its first complete matching
`key:value` / `key=value` reply must fall within the inclusive range. If raw
Expect bytes are also configured, both conditions must pass. Missing data times
out; an out-of-range value fails explicitly. Assertions do not infer a hardware
ACK or physical measurement. Explicit Start, exclusive TX ownership, cancellation
and connection-generation guards remain in effect; no automatic reconnect resume.

**Save results** offers structured CSV with each step's outcome and observed
value, or a plain log summary. Older scripts remain readable; numeric-assertion
scripts use the versioned newer format.

## XY and recorded-run comparison

Open Workbench **Tools > XY / Recorded-run comparison**. Live XY starts with no selected X/Y.
Choose two fields to plot the latest 2,000 same-row pairs using independent axes;
display scale/offset applies. Missing fields never pair with a stale value. Connect
XY points is optional; selected field names are saved in INI.

For **Compare runs**, load two recordings in the background, select a shared key
and open **Align / tolerance**. Time zero is each run's first numeric row. Shift
is added to run 2; pairing uses the nearest available sample within the maximum
time difference, each sample at most once, with no interpolation. Absolute value
tolerance checks `abs(run2 - run1)`. The plot overlays raw numeric values; the
summary and Save comparison CSV show matches, failures and unmatched samples.

Supported inputs are this tool's CSV/CVS export and text/raw recordings. Load
snapshots the current main extraction/filter settings, independent offline binary
schema and Offline replay synthetic interval (100 ms if unopened). A raw/index
pair uses recorded receipt timing; synthetic timing is labeled. Imports are
read-only and bounded to 64 MiB raw / 32 MiB CSV, 10,000 numeric rows and 32 MiB
retained row data. Exceeding a limit rejects the new load and keeps the old run;
use Offline replay to locate a smaller region in larger recordings.

## Complete workspace profiles

Profiles also include allowlisted B port/settings, split ratio, A/B binary/text
parsers, framing, pacing/view preferences, offline binary schema, derived formulas,
instrument choices/ranges and XY/comparison preferences. Save captures applied
settings. Import/Load stages a preview; explicit idle Apply writes atomically and
preserves unrelated private INI keys. Legacy v1 files remain readable (64 KiB);
workspace v2 permits up to 512 KiB. Unknown fields/ranges are rejected.

Commands, TX history, file/recording paths, loaded-run contents and armed forwarding
are excluded. Import never connects, sends or resumes an operation. Profiles can
contain private device IDs/filter names and are not encrypted; review before sharing.

## Explicit finite A/B forwarding

Connect both ports, then use Workbench **Tools > Start A/B forwarding**. Select A to B,
B to A, or Both, a duration of 1..3600 seconds and a byte budget of 1..1024 MiB.
Review the confirmation: forwarding sends received bytes to physical equipment
and can execute commands. Both TX queues are reserved; conflicting tools must stop.

The forwarder preserves exact bytes in bounded chunks/queues, with one in-flight
write per destination. Stop, partial/failed TX, disconnect/generation change,
input gap, queue limit or duration ends it without retry or automatic resume.
The byte budget drains accepted queued work before stopping. Both-direction mode
stops on a possible exact-payload echo; that heuristic cannot prove absence of all
physical loops. Prefer one direction unless the equipment requires two. Logs stay
in their separate A/B groups; already accepted bytes cannot be undone by Stop.
