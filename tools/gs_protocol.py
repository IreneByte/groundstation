"""Parse $GS telemetry lines.

The format is written down in lib/gs_core/telemetry_format.h. Short version:
    $GS,seq,t_ms,...,period_max_us*CS
CS = XOR of every character between '$' and '*', as 2 hex digits.
"""

FIELDS = [
    "seq", "t_ms", "pitch", "roll", "spd_l", "spd_r", "pwm_l", "pwm_r",
    "dist_cm", "temp_c", "state", "fault", "warn", "faults",
    "loop_avg_us", "loop_max_us", "period_max_us",
]

# how to convert each field from text
INT_FIELDS = {"seq", "t_ms", "pwm_l", "pwm_r", "dist_cm", "faults",
              "loop_avg_us", "loop_max_us", "period_max_us"}
TEXT_FIELDS = {"state", "fault", "warn"}
STATES = {"IDLE", "ONLINE", "MANUAL", "FAULT", "RESET_REQUIRED"}


def checksum(body):
    """XOR of all characters in body (the part between '$' and '*')."""
    cs = 0
    for ch in body:
        cs ^= ord(ch)
    return cs


def make_line(values):
    """Build a full line from a dict. Used by the fake telemetry mode."""
    parts = ["GS"]
    for name in FIELDS:
        v = values[name]
        if isinstance(v, float):
            v = f"{v:.1f}" if name in ("spd_l", "spd_r", "temp_c") else f"{v:.2f}"
        parts.append(str(v))
    body = ",".join(parts)
    return f"${body}*{checksum(body):02X}"


def parse_line(line):
    """Return ("ok", record), ("bad", reason) or ("skip", None).

    "skip" is for '#' log lines and empty lines, which are not telemetry.
    """
    line = line.strip()
    if not line or line.startswith("#"):
        return "skip", None
    if not line.startswith("$GS,") or "*" not in line:
        return "bad", "not a $GS line"

    body, _, cs_text = line[1:].rpartition("*")
    try:
        cs = int(cs_text, 16)
    except ValueError:
        return "bad", "checksum is not hex"
    if len(cs_text) != 2 or cs != checksum(body):
        return "bad", "checksum mismatch"

    parts = body.split(",")[1:]
    if len(parts) != len(FIELDS):
        return "bad", f"expected {len(FIELDS)} fields, got {len(parts)}"
    return _convert(parts)


def _convert(parts):
    record = {}
    try:
        for name, text in zip(FIELDS, parts):
            if name in INT_FIELDS:
                record[name] = int(text)
            elif name in TEXT_FIELDS:
                record[name] = text
            else:
                record[name] = float(text)  # "nan" is fine here
    except ValueError:
        return "bad", f"bad number in field {name}"
    if record["state"] not in STATES:
        return "bad", "unknown state"
    return "ok", record
