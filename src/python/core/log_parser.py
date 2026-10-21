"""Numeric telemetry: comma-delimited key:value or key=value fields."""

import math
import re

NUMBER = re.compile(
    r"^([+-]?0[xX][0-9a-fA-F]+|[+-]?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?)"
    r"\s*([A-Za-z%]*)$"
)


def _number(text):
    match = NUMBER.fullmatch(text.strip())
    if not match:
        return None
    token, unit = match.groups()
    if text.strip().lstrip('+-').lower().startswith('0x') and '0x' not in token.lower():
        return None
    try:
        if '0x' in token.lower():
            if unit:
                return None
            integer = int(token, 16)
            if abs(integer) > 0xffffffffffffffff:
                return None
            value = float(integer)
        else:
            value = float(token)
        return value if math.isfinite(value) else None
    except (ValueError, OverflowError):
        return None


def parse_kv_log(line):
    """Discover channels; slash lists become key[0], key[1], ... .

    Percentages keep their displayed magnitude (11% -> 11, not 0.11).
    Text-only tokens, malformed fields and non-finite values are skipped.
    """
    result = {}
    for token in (line or '').strip().split(','):
        separators = [pos for pos in (token.find(':'), token.find('=')) if pos >= 0]
        if not separators:
            continue
        pos = min(separators)
        key, raw = token[:pos].strip(), token[pos + 1:].strip()
        if not key or len(key.encode('utf-8')) > 128 or any(ord(ch) < 32 for ch in key):
            continue
        values = [_number(part) for part in raw.split('/')]
        if not values or any(value is None for value in values):
            continue
        for index, value in enumerate(values):
            name = key if len(values) == 1 else f'{key}[{index}]'
            result[name] = value
    return result
