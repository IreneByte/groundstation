"""Write the CSV and the plot."""
import csv
import math

import matplotlib
matplotlib.use("Agg")  # no window needed, just save a PNG
import matplotlib.pyplot as plt  # noqa: E402

from gs_protocol import FIELDS  # noqa: E402


def write_csv(path, records):
    with open(path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=FIELDS)
        w.writeheader()
        for r in records:
            w.writerow(r)


def save_plot(path, records, fault_events):
    """3 panels: tilt (filter output), wheel speed, control loop timing."""
    records = sorted(records, key=lambda r: r["seq"])  # late messages back in place
    t = [r["t_ms"] / 1000.0 for r in records]
    fig, ax = plt.subplots(3, 1, figsize=(10, 8), sharex=True)

    ax[0].plot(t, [r["pitch"] for r in records], label="pitch")
    ax[0].plot(t, [r["roll"] for r in records], label="roll")
    ax[0].set_ylabel("deg")
    ax[0].set_title("Tilt (complementary filter output)")

    ax[1].plot(t, [r["spd_l"] for r in records], label="left")
    ax[1].plot(t, [r["spd_r"] for r in records], label="right")
    ax[1].set_ylabel("ticks/s")
    ax[1].set_title("Wheel speed")

    ax[2].plot(t, [r["loop_avg_us"] for r in records], label="avg run time")
    ax[2].plot(t, [r["loop_max_us"] for r in records], label="worst run time")
    ax[2].set_ylabel("us")
    ax[2].set_xlabel("time on the ESP32 (s)")
    ax[2].set_title("Control loop timing (20 ms period)")

    # red line wherever a new fault showed up
    for t_ms, code in fault_events:
        for a in ax:
            a.axvline(t_ms / 1000.0, color="red", linestyle="--", linewidth=1)
        ax[0].annotate(code, (t_ms / 1000.0, ax[0].get_ylim()[1]), color="red")

    for a in ax:
        a.legend(loc="upper right")
        a.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(path, dpi=100)
    plt.close(fig)


def clean_for_csv(rec):
    """NAN temperature -> empty cell, easier to read in a spreadsheet."""
    out = dict(rec)
    if isinstance(out["temp_c"], float) and math.isnan(out["temp_c"]):
        out["temp_c"] = ""
    return out
