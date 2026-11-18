"""Fake telemetry so the tool and its tests run without the ESP32.

It makes the same $GS lines the firmware sends, then breaks some on
purpose: drops, corrupted characters (bad checksum), and swapped pairs
(out of order). It also returns how many of each it injected, so tests
can check that the tool counts them right.
"""
import math
import random

from gs_protocol import make_line

PERIOD_MS = 100  # 10 Hz like the firmware


def fake_record(seq):
    """One plausible record. Drives in MANUAL, faults once at seq 60."""
    t = seq * PERIOD_MS
    if seq < 10:
        state, fault = "ONLINE", "-"
    elif seq < 60:
        state, fault = "MANUAL", "-"
    elif seq < 80:
        state, fault = "FAULT", "F02"
    else:
        state, fault = "MANUAL", "-"
    driving = state == "MANUAL"
    pitch = 5.0 * math.sin(t / 1000.0) + (35.0 if fault == "F02" else 0.0)
    return {
        "seq": seq, "t_ms": t, "pitch": pitch, "roll": 2.0 * math.cos(t / 700.0),
        "spd_l": 60.0 if driving else 0.0, "spd_r": 59.0 if driving else 0.0,
        "pwm_l": 140 if driving else 0, "pwm_r": 143 if driving else 0,
        "dist_cm": 80, "temp_c": 24.5, "state": state, "fault": fault,
        "warn": "-", "faults": 1 if seq >= 60 else 0,
        "loop_avg_us": 800 + seq % 7, "loop_max_us": 1400 + seq % 50,
        "period_max_us": 20000 + seq % 90,
    }


def corrupt(line, rng):
    """Change one character after the seq field so the checksum no longer
    matches. (The seq field is left alone so the tool can tell "bad" from
    "dropped". A real broken seq field counts as one bad + one dropped.)"""
    i = rng.randrange(line.index(",", 4) + 1, line.index("*"))
    new = "#" if line[i] != "#" else "%"
    return line[:i] + new + line[i + 1:]


def fake_stream(count, drop=0.03, bad=0.03, swap=0.02, seed=1):
    """Return (lines, truth). truth counts what was injected."""
    rng = random.Random(seed)
    truth = {"dropped": 0, "bad": 0, "out_of_order": 0, "sent": count}
    lines = []
    for seq in range(count):
        line = make_line(fake_record(seq))
        r = rng.random()
        if r < drop:
            truth["dropped"] += 1
            continue
        if r < drop + bad:
            truth["bad"] += 1
            line = corrupt(line, rng)
        lines.append(line)

    # swap some neighbour pairs of good lines -> out-of-order messages
    i = 1
    while i < len(lines):
        both_good = "#" not in lines[i] + lines[i - 1] and "%" not in lines[i] + lines[i - 1]
        if both_good and rng.random() < swap:
            lines[i - 1], lines[i] = lines[i], lines[i - 1]
            truth["out_of_order"] += 1
            i += 2
        else:
            i += 1
    return lines, truth
