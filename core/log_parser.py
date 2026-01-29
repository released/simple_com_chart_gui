import re

"""
MCU log protocol (text-based)

Expected input per line:
    key:value,key:value,...

Example:
    state:5,CHG:4179mv,T1:2296mv,...

Notes:
- One line = one sample
- Order of fields does not matter
- Unknown keys are ignored
- Timestamp is generated on PC side

Any format change on MCU side must be reflected here.
"""


# key must start with letter, allow A-Z a-z 0-9 _ /
# length limit avoids garbage fragments
VALID_KEY_PATTERN = re.compile(r'^[A-Za-z][A-Za-z0-9_/]{1,15}$')


def parse_kv_log(line):
    """
    Parse key-value log line into dict[key]=int_value.

    Example:
      "state:5,CHG:4181mv,T1:2299mv,Q2/Q3:  21mv,"
      -> {"state":5, "CHG":4181, "T1":2299, "Q2/Q3":21}
    """
    result = {}

    if line is None:
        return result

    tokens = line.strip().split(",")
    for tok in tokens:
        tok = tok.strip()
        if not tok:
            continue
        if ":" not in tok:
            continue

        key, val = tok.split(":", 1)
        key = key.strip()
        val = val.strip()

        # ---- KEY SANITY CHECK (IMPORTANT) ----
        if not VALID_KEY_PATTERN.match(key):
            continue
        if key.isdigit():
            continue
        # -------------------------------------

        m = re.search(r"(-?\d+)", val)
        if not m:
            continue

        try:
            result[key] = int(m.group(1))
        except ValueError:
            pass

    return result
