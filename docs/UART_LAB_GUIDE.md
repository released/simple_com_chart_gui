# UART Laboratory

Open **UART > Laboratory / Diagnostics**. The laboratory uses the existing native
UART/parser/recording core and adds no interpreter or runtime installation.
Messages, Chart and 4 Charts keep their independent selections and raw data.

## Diagnostics

The Diagnostics page shows the active text pipeline's accepted lines, Include
misses, exclusions, filter work limits, extraction failures, missing numeric
fields and invalid/backwards extracted time. Small printable examples help locate
the relevant setting; full bytes remain in Messages/recordings. Binary A/B has
separate frame/checksum/layout/codec/unknown-ID/discard/history-gap counters.
Text counters are not evidence about binary/RTU-owned traffic, which bypasses
text extraction. History expiry, software input loss and waveform selection are
separate observations, not measurements of electrical UART errors.

If extraction succeeds but a chart stays empty, check its selected channels,
Snapshot, time window and fixed Y range. All/None waveform selection never turns
raw recording on or off. Reset diagnostic counters does not clear raw data.

## Recording range export and health

Select a completed recording/rotation segment, then choose record bounds or file
seconds. Bounds are **[first, past-last)**; an empty end means EOF. Record indexes
are zero-based. Seconds select existing index boundaries rather than inventing
per-byte times. A large RX chunk may occupy several 4 KiB replay records sharing
the same PC receipt timestamp. Export reads ordered rotation segments using the existing paged
index and writes a **new** `.bin` plus adjacent `.idx` for receipt-indexed input.
Unindexed/synthetic input exports `.bin` only. Original files remain read-only.

Check selected segment and Salvage valid prefix inspect only the selected `.bin`
and adjacent `.idx`, not every rotation file. The report distinguishes matched
structure, declared recorder gaps, unindexed bytes and malformed/truncated rows.
Salvage copies only the contiguous valid indexed RX prefix to a new filename;
it cannot recover unknown bytes or receipt times. No stored payload hash exists,
so a structurally valid file is not proof of byte integrity or power-loss durability.
Archive gap warnings remain identified as source warnings when the selected
range's precise gap location is unavailable.

Operations run in background. Cancel file work or close the laboratory to request
cancellation; application close drains owned file work. A completed save is not
undone by a later Cancel. Existing destination files are never overwritten;
failed/cancelled newly created outputs are removed or a cleanup error is reported.

## Multi-parameter packets

Choose A/B, Text or HEX, a template, and CSV parameter rows:

```text
name,value,minimum,maximum
```

Do not include a header row. A template can contain `{address:u8}` and
`{target:u16le}`; each name has its own value/range. HEX integer types are
u8/i8, u16/i16/u32/i32 with le/be suffixes. Text supports untyped `{name}`.
Reserved `{length:type}` contains total unencoded payload bytes including final
checksum/ending; `{sequence:type}` uses the explicitly edited sequence value.
Length substitution must converge. Sequence does not automatically increment.

Parameters are substituted first, then checksum and ending are appended, then
the optional COBS/SLIP encoding is applied. The final wire payload must fit the
existing 1..4096-byte TX limit. Up to 32 parameters are supported. Preview shows
exact final HEX; preview/load/save never opens a port or transmits.

Send once requires a connected idle port and confirmation. If connection identity
changes while confirming, nothing is submitted. Existing per-port TX ownership,
pacing and partial/error/cancel behavior apply; no automatic retry/reconnect
replay is added. Driver acceptance does not establish target acknowledgement.
Definitions use private `.sccpacket` files; old `.scccommands`/`.sccsteps` formats
remain unchanged. New-file saves do not overwrite existing definitions.

## Sequence and response statistics

Select the source, exact numeric key and expected sequence modulus/step. Include
the `A.`/`B.` prefix when that is part of the discovered key. Empty key disables
observation. Each source has its own counters. Accepted numeric samples after
filtering are classified as duplicates, expected wraps, forward discontinuities,
backwards/reset ambiguity or invalid values. Connection-generation changes create
a new baseline. These are observed discontinuities: filtering, firmware behavior
and device reset can all change them; they do not prove where data was lost.

Response statistics use matched successful steps of the **most recent finite
script** only. Driver-only steps are excluded. Min/mean/max and nearest-rank
p50/p95/p99 describe `PC_step_ms`, including host scheduling/queuing/TX work;
they are not UART bit timing, protocol-validated ACK latency or long-term metrics.
Timeouts are reported separately. Small sample sets have limited statistical value.
Statistics can be saved as a new CSV. Sequence preferences persist in the INI;
counters do not auto-start a script or send probes.

## Offline parser cases

Use current parser copies its configuration into this editor without applying
anything to live capture. Cases are CSV with this exact first row:

```csv
Input,Expected,Seconds
```

Expected contains numeric `key:value` pairs, an empty cell for no fields, or
`ERROR` to require an extraction error/work-limit result. Quote comma-containing
cells using normal CSV quotes. Nonempty Seconds asserts an extracted device-time
value; empty Seconds does not assert time. All numeric output keys must match,
with the selected absolute tolerance. Empty/unparseable numeric fields are not
fabricated as zero. Limits: 64 cases, 64 KiB CSV and 4096 input bytes per case.

Run/import/export never touches COM, sends bytes or changes the active parser.
Save curated cases as private `.scccases` files. Temporary generated validation
datasets are not permanent product assets and must not accumulate in the project.

## Binary message schemas and codecs

Choose a live A/B schema or the independent offline replay schema. Normal rows
remain `name,offset,type[,scale,add]`; existing header/length/checksum settings
still apply. Additional optional rows are:

```text
@codec,COBS
@id,1,1,Little
@case,1,temperature,2,u16le
@case,2,voltage,2,u16le,0.001
```

Codec is None, COBS or SLIP. ID rows specify offset, width (1/2/4 bytes) and
Little/Big order; case IDs are decimal. Common fields apply to all messages;
matching case fields are added. Up to 32 message types and 64 common+case fields
per message are supported. Unknown IDs remain raw/inspectable and are counted.
Fields with identical names share a numeric channel; choose distinct names when
their meanings differ. Variable arrays/nested schemas are not part of this format.

Wire decoding precedes normal header/length/checksum processing. Partial codec
packets never span a reconnect/seek. Invalid/overlong packets and bounded-dispatch
overflow are counted; later delimiters allow resynchronization. Raw Messages and
recording preserve encoded wire bytes. Decoded byte offsets refer to decoded
payloads, not locations in original wire data. Packet completion uses the PC
receipt time of the final input chunk, not per-byte wire timestamps.

## Digital state history

Define up to eight rows `name,exact-key,first-bit,width`. Width 0 with first-bit 0
shows the complete unsigned enum; otherwise the selected 1..32 bits must fit
within an unsigned 32-bit value. Empty definitions and Reset to none disable lanes.
Lane/window/maximum-hold preferences persist in the portable INI/workspace.

The view uses retained raw numeric values, independently of waveform checkboxes
and display-only scaling. It shows UART-reported states, not electrical logic
levels. Missing/invalid source samples, connection generations and excessive
hold gaps interrupt the history. Other-source rows do not invent transitions.
No line is extrapolated beyond the latest report. Zoom the time window to inspect
changes narrower than a screen pixel. Retention expiry limits available history.

## Optional finite CLI

`build/SimpleComChartCli.exe` uses the native script/serial core and static CRT.
`--help` and `validate` do not open hardware or change the portable INI.

```text
SimpleComChartCli.exe validate --script private.sccsteps
SimpleComChartCli.exe run --script private.sccsteps --port COM3 --baud 115200 --report new-result.csv --confirm-tx
```

Optional data/parity/stop settings and maximum duration are listed by `--help`.
Default is 8/N/1, no flow control, DTR/RTS enabled, 60 seconds maximum. Opening a
port can change control lines/reset a connected target. Run is explicit and
finite; only one outstanding step is sent. Ctrl+C, errors, partial writes and
duration limits stop without retry/reconnect/resume. GUI and CLI share session
exclusion; a busy owner fails closed rather than opening another tool session.

Exit 0 means validation succeeded or all script steps passed; 2 means invalid
arguments/setup/report failure; 3 means port open or script execution failed.
The report path must be new and is reserved before opening the port. Successful
driver-only steps still do not establish target ACK; CSV makes that distinction.
Physical driver/hardware acceptance requires separate testing.
