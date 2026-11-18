#!/usr/bin/env python3
"""Groundstation recorder: read $GS telemetry, save CSV, summary and plot.

Sources (pick one):
  --fake N            N fake messages with drops / bad / out-of-order lines
  --file LOG          a saved serial log or a recording
  --ws HOST[:PORT]    live from the ESP32 WebSocket (default port 81)
  --serial PORT       live from USB serial (needs: pip install pyserial)

Output goes to --out (default tools/out): telemetry.csv, bad_lines.txt,
summary.txt, plot.png
"""
import argparse
import os
import socket
import sys
import time

from gs_fake import fake_stream
from gs_protocol import parse_line
from gs_report import clean_for_csv, save_plot, write_csv
from gs_stats import Tracker, format_summary

HERE = os.path.dirname(os.path.abspath(__file__))


def ws_lines(target, seconds, keepalive):
    from gs_ws import WsClient
    host, _, port = target.partition(":")
    ws = WsClient(host, int(port or 81), timeout=0.2)
    end, next_ping = time.time() + seconds, 0.0
    try:
        while time.time() < end:
            # the firmware faults (F04) without a ping every < 500 ms
            if keepalive and time.time() >= next_ping:
                ws.send_text("ping")
                next_ping = time.time() + 0.2
            try:
                yield ws.recv_text()
            except socket.timeout:
                pass
    finally:
        ws.close()


def serial_lines(port, seconds):
    try:
        import serial  # pyserial, only needed for this source
    except ImportError:
        sys.exit("pyserial is not installed: pip install pyserial")
    end = time.time() + seconds
    with serial.Serial(port, 115200, timeout=0.5) as s:
        while time.time() < end:
            yield s.readline().decode(errors="replace")


def pick_source(args):
    if args.fake:
        lines, truth = fake_stream(args.fake, seed=args.seed)
        print(f"fake mode: sent {truth['sent']}, injected dropped={truth['dropped']} "
              f"bad={truth['bad']} out_of_order={truth['out_of_order']}")
        return iter(lines)
    if args.file:
        return open(args.file, errors="replace")
    if args.ws:
        return ws_lines(args.ws, args.seconds, not args.no_ping)
    return serial_lines(args.serial, args.seconds)


def record(lines):
    """Parse every line. Returns (tracker, good records, bad lines)."""
    tracker, records, bad = Tracker(), [], []
    for line in lines:
        status, result = parse_line(line)
        if status == "ok":
            tracker.add(result)
            records.append(result)
        elif status == "bad":
            tracker.add_bad(line.strip())
            bad.append(f"{result}: {line.strip()}")
    return tracker, records, bad


def save_outputs(out, tracker, records, bad):
    os.makedirs(out, exist_ok=True)
    write_csv(os.path.join(out, "telemetry.csv"), [clean_for_csv(r) for r in records])
    with open(os.path.join(out, "bad_lines.txt"), "w") as f:
        f.write("\n".join(bad) + ("\n" if bad else ""))
    text = format_summary(tracker.summary(), tracker.fault_events)
    with open(os.path.join(out, "summary.txt"), "w") as f:
        f.write(text + "\n")
    if records:
        save_plot(os.path.join(out, "plot.png"), records, tracker.fault_events)
    return text


def main(argv=None):
    p = argparse.ArgumentParser(description="Record and check groundstation telemetry")
    src = p.add_mutually_exclusive_group(required=True)
    src.add_argument("--fake", type=int, metavar="N")
    src.add_argument("--file")
    src.add_argument("--ws", metavar="HOST[:PORT]")
    src.add_argument("--serial", metavar="PORT")
    p.add_argument("--seconds", type=float, default=30, help="how long to record live")
    p.add_argument("--no-ping", action="store_true", help="don't send watchdog pings")
    p.add_argument("--seed", type=int, default=1, help="random seed for --fake")
    p.add_argument("--out", default=os.path.join(HERE, "out"))
    args = p.parse_args(argv)

    tracker, records, bad = record(pick_source(args))
    print(save_outputs(args.out, tracker, records, bad))
    print(f"saved to {args.out}")
    return 0 if records else 1


if __name__ == "__main__":
    sys.exit(main())
