"""Count messages, drops, out-of-order, bad lines, and timing."""


class Tracker:
    def __init__(self):
        self.ok = 0
        self.bad = 0
        self.out_of_order = 0
        self.duplicates = 0
        self.missing = set()   # seq numbers we skipped over and never saw
        self.bad_seqs = set()  # seq numbers that arrived, but broken
        self.last_seq = None   # highest seq seen so far
        self.first_t = None
        self.last_t = None
        self.prev_t = None     # t_ms of the previous good message
        self.worst_gap_ms = 0
        self.loop_avg_sum = 0
        self.loop_max_us = 0
        self.period_max_us = 0
        self.fault_events = []  # (t_ms, fault code) each time a new fault shows up
        self._last_fault = "-"

    def add_bad(self, line):
        """A line with a bad checksum or format. If its seq field can
        still be read, that seq arrived (broken) and is not a drop."""
        self.bad += 1
        try:
            self.bad_seqs.add(int(line.split(",")[1]))
        except (IndexError, ValueError):
            pass  # seq field itself is broken: it will count as dropped

    def add(self, rec):
        """Feed one good record (dict from gs_protocol.parse_line)."""
        self.ok += 1
        in_order = self._check_seq(rec["seq"])
        self._check_time(rec["t_ms"])
        self.loop_avg_sum += rec["loop_avg_us"]
        self.loop_max_us = max(self.loop_max_us, rec["loop_max_us"])
        self.period_max_us = max(self.period_max_us, rec["period_max_us"])
        if not in_order:
            return  # a late message must not look like a new fault
        if rec["fault"] != "-" and rec["fault"] != self._last_fault:
            self.fault_events.append((rec["t_ms"], rec["fault"]))
        self._last_fault = rec["fault"]

    def _check_seq(self, seq):
        """Returns True if this message is newer than everything before it."""
        if self.last_seq is not None and seq > self.last_seq + 1:
            # jumped ahead: everything in between is missing (for now)
            self.missing.update(range(self.last_seq + 1, seq))
        if self.last_seq is None or seq > self.last_seq:
            self.last_seq = seq
            return True
        if seq in self.missing:
            # a late message we thought was dropped
            self.missing.discard(seq)
            self.out_of_order += 1
        else:
            self.duplicates += 1
        return False

    def _check_time(self, t_ms):
        if self.first_t is None:
            self.first_t = t_ms
        if self.prev_t is not None and t_ms > self.prev_t:
            self.worst_gap_ms = max(self.worst_gap_ms, t_ms - self.prev_t)
        self.prev_t = t_ms
        self.last_t = t_ms if self.last_t is None else max(self.last_t, t_ms)

    def summary(self):
        span_ms = (self.last_t - self.first_t) if self.ok > 1 else 0
        rate = (self.ok - 1) * 1000.0 / span_ms if span_ms > 0 else 0.0
        return {
            "messages": self.ok,
            "bad": self.bad,
            "dropped": len(self.missing - self.bad_seqs),
            "out_of_order": self.out_of_order,
            "duplicates": self.duplicates,
            "avg_rate_hz": round(rate, 2),
            "worst_gap_ms": self.worst_gap_ms,
            "loop_avg_us": round(self.loop_avg_sum / self.ok) if self.ok else 0,
            "loop_worst_us": self.loop_max_us,
            "period_worst_us": self.period_max_us,
            "faults_seen": len(self.fault_events),
        }


def format_summary(s, fault_events):
    lines = [
        f"messages      {s['messages']}",
        f"bad           {s['bad']}  (checksum or format wrong)",
        f"dropped       {s['dropped']}  (seq numbers never received)",
        f"out of order  {s['out_of_order']}",
        f"duplicates    {s['duplicates']}",
        f"average rate  {s['avg_rate_hz']} Hz",
        f"worst gap     {s['worst_gap_ms']} ms",
        f"control loop  avg {s['loop_avg_us']} us, worst {s['loop_worst_us']} us, "
        f"worst period {s['period_worst_us']} us",
        f"faults seen   {s['faults_seen']}",
    ]
    for t_ms, code in fault_events:
        lines.append(f"  {code} at {t_ms} ms")
    return "\n".join(lines)
