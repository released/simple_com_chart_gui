# Recording sessions and UART workflow tools

Open **UART > Laboratory / Diagnostics**. These tools use native C++/MFC and
Windows APIs; no interpreter, database, third-party driver or added runtime is
required. USB-UART hardware still needs a suitable device driver.

## Recording limits and capture-start settings

**UART > Recording options** configures rotation plus three independent limits:

- **Total output MiB** counts actual `.bin`, `.idx`, `.log` and capture metadata
  bytes across all rotation segments for this recording. Zero is unlimited.
- **Maximum seconds** bounds one recording, including periods with no RX.
- **Minimum free MiB** keeps a selected amount of disk space available. Zero
  disables this guard. The writer periodically queries available user space.

Options apply to the next A or B recording. A limit stops recording and displays
its reason; it does not disconnect the port or delete older logs. A data chunk
that would exceed the byte/free-space limit is rejected before its raw/index/text
writes. Space may change after a check, so normal write/flush error handling is
still required. Limits are not a power-loss durability guarantee.

New GUI recordings create a private `<prefix>.sccmeta` sidecar. It contains the
tool version, logical A/B source and a sanitized settings snapshot taken at
recording start. Device identities, port names, private commands/history and
recording paths are excluded from that snapshot. It is plain encoded text, not
encryption. Later numeric setting changes are marked; the start snapshot does not
reconstruct a complete history of configuration changes.

## Recording sessions

The **Recording sessions** page creates a new `.sccsession` manifest beside
completed `.bin` recordings. It references files by relative filename, records
their exact sizes/SHA256 and embeds the safe settings snapshot. Raw data is not
copied into another archive. Keep the manifest and referenced files together.

1. Select a completed `.bin` or numbered rotation segment.
2. Select **Capture-start metadata**, or explicitly choose **Current reviewed
   settings** for older files. The latter is labeled current, not capture-time.
3. Choose a new manifest filename in the same folder and click **Create manifest**.
4. **Verify files** checks hashes, sizes, recording structure and segment set.
5. **Verify and stage settings** stages the safe settings in Profiles and fills
   the existing replay path/interval. Review and explicitly Apply while idle,
   then Load/Play separately. No connection or command replay occurs.

Readable logs may include TX content; **Include readable TX/RX .log** defaults to
No. Raw device data may itself be sensitive. Review the selected files before
sharing; a manifest is not a privacy filter for recorded payloads.

Limits: 4096 file references, 4 MiB manifest and 256 GiB of referenced files.
References cannot escape the manifest folder or traverse links/junctions.
Files are held read-only during checks; cancellation leaves originals unchanged.
SHA256 detects changed content, not authorship or correct device behavior.
Verification reflects the files at check time; later edits require verification
again. The underlying replay limits and visible-history retention still apply.

## COM device information

**Refresh device list** queries present COM devices without opening them.
Search by COM/name/VID/PID/manufacturer and use **Copy displayed information**
when needed. Driver-published serial, location/provider/version may be n/a.
An instance-ID suffix is not guessed to be a USB serial number. This viewer does
not change existing connection, uniqueness or same-device recovery decisions.

## Numeric input framing

Choose A, B or independent offline replay in **Numeric input framing**.
LF is the compatible default. Additional choices are CR, CRLF, an explicit
1..16-byte HEX delimiter, or a PC receipt idle gap of 1..60000 ms. CR mode treats
an immediately following LF as padding. Custom delimiters are removed before
numeric parsing; raw Messages/recordings are unchanged.

For `{"fan":{"rpm":201}};`, select delimiter HEX `3B`, then configure JSON
extraction with the pointer selection below. Partial data and byte ranges survive
split reads. Reconnect/seek/input discontinuities clear partial frames. The 64 KiB
numeric-input limit remains; overlong input is discarded until resynchronization.
Idle gaps are host read-chunk timing, not guaranteed UART inter-byte timing.
Apply requires disconnected/idle ports, unloaded replay and finished file work;
it starts a new numeric capture and persists the selected source configuration.

## Nested JSON numeric paths

In JSON extraction, use the **Columns / JSON ptr:name=/path** field:

```text
ptr:rpm=/fan/rpm,temp=/temps/1,state=/a~1b/~0
```

Names become numeric channel keys. Paths use JSON Pointer: `/` separates object
keys/array indexes, `~1` means a literal slash and `~0` a literal tilde. A CSV-quoted
whole selection can contain commas, for example `ptr:"value=/a,b"`.
Missing/null/string/bool values remain blank, never zero. Explicit path mode
rejects ambiguous duplicate keys, excessive depth/work and invalid numbers.
Limits: 64 selections, 8 KiB definitions, depth 16 and 4096 parsed values.
No `ptr:` prefix keeps compatible flat JSON behavior, including old unused CSV
column names. Parser preview and offline cases use the same production core.
Pointer-based parser and Port B settings use a v2 marker so older tools reject
them visibly; ordinary legacy settings retain their v1 format. Use this version
or newer when sharing profiles that contain paths.

## Custom CRC

The **Custom CRC** page accepts:

```text
width:polynomialHEX:initialHEX:xorHEX:reflectInput:reflectOutput
8:07:00:00:0:0
16:1021:FFFF:0000:0:0
32:04C11DB7:FFFFFFFF:FFFFFFFF:1:1
```

Supported widths are 8/16/32. Polynomial excludes its leading term; reflection
flags are 0/1. Specify the first input byte and covered count; 0 count means the
remaining bytes. Calculate is offline. **Use for packet** fills the packet CRC
settings only; preview and confirmed Send once retain existing TX ownership.
CRC is appended before the selected ending and optional COBS/SLIP encoding.
Packet length includes CRC/ending and excludes wire encoding.

For live A/B or offline binary RX, choose standard Checksum **None**, set the
existing checksum-from/byte-order fields and add a layout directive:

```text
@crc,8,07,00,00,0,0
value,0,u8
```

An optional final directive column limits the covered count; 0/omitted means
all payload bytes after checksum-from. The checksum occupies the frame's final
1/2/4 bytes. **Show binary directive** provides the current values. Existing
standard checksums and older packet definitions retain their meanings.

## Time derivative and integral

Select a retained key, A/B source, operation and maximum allowed sample gap,
then **Calculate retained snapshot**. The view and new-file CSV use raw numeric
values independently of waveform choices or display-only scaling.

Derivative is `(current - previous) / dt`; a segment's first value is n/a.
Integral starts at zero for every valid segment and uses the trapezoidal rule,
assuming a piecewise-linear signal only between adjacent samples in that segment.
Missing/nonfinite values, reconnects and nonpositive/excessive time steps break
segments. Other-source rows are ignored. No line joins across a gap.
Time uses retained source seconds: PC receipt, configured device time or explicitly
synthetic replay. Output units are raw value/second or raw value*second; these are
not electrical UART measurements. The capture's 10000-row retention bounds apply.

## Passive transactions

Select one port, framing, literal TX request/RX reply prefixes and ID byte spans.
**Analyze retained traffic** observes a bounded snapshot; it sends nothing.
Each source/generation is isolated. Multiple outstanding requests with the same
ID remain ambiguous until timeout; missing replies and expired history remain
visible. A rule match is not protocol-validated ACK or evidence of where loss
occurred. PC intervals include worker/driver scheduling.

Choose a zero-based result row and **Inspect request/reply** to view its original
retained bytes and traffic sequence:byte spans. Span ends are exclusive; HEX
preview is limited to 4096 bytes. Save the bounded result table as a new CSV.
Analysis limits: 256 KiB / 2048 input chunks, 8192 frames and 512 result rows.
Opening Laboratory begins future bounded A traffic collection; B uses Workbench.
