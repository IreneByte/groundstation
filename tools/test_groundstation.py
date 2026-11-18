"""Tests for the groundstation tool. Run: python3 -m unittest -v (in tools/)"""
import os
import tempfile
import unittest

import groundstation
from gs_fake import fake_record, fake_stream
from gs_protocol import checksum, make_line, parse_line
from gs_stats import Tracker

# exact line printed by host_tests/test_telemetry.cpp (C++ side)
CPP_LINE = "$GS,42,8123,1.25,-0.40,60.0,58.5,141,147,57,23.4,MANUAL,-,-,0,812,1430,20110*2D"


class ProtocolTest(unittest.TestCase):
    def test_same_checksum_as_cpp(self):
        status, rec = parse_line(CPP_LINE)
        self.assertEqual(status, "ok")
        self.assertEqual(rec["seq"], 42)
        self.assertEqual(rec["state"], "MANUAL")
        self.assertAlmostEqual(rec["roll"], -0.40)

    def test_make_line_round_trip(self):
        line = make_line(fake_record(15))
        status, rec = parse_line(line)
        self.assertEqual(status, "ok")
        self.assertEqual(rec["seq"], 15)

    def test_bad_lines(self):
        self.assertEqual(parse_line(CPP_LINE.replace("141", "142"))[0], "bad")  # bit flip
        self.assertEqual(parse_line(CPP_LINE[:-2] + "ZZ")[0], "bad")          # not hex
        self.assertEqual(parse_line("$GS,1,2*" + f"{checksum('GS,1,2'):02X}")[0], "bad")
        self.assertEqual(parse_line("garbage")[0], "bad")

    def test_log_lines_skipped(self):
        self.assertEqual(parse_line("#FSM t=1 IDLE -> ONLINE")[0], "skip")
        self.assertEqual(parse_line("   ")[0], "skip")

    def test_nan_temperature(self):
        rec = fake_record(3)
        rec["temp_c"] = float("nan")
        status, out = parse_line(make_line(rec))
        self.assertEqual(status, "ok")
        self.assertNotEqual(out["temp_c"], out["temp_c"])  # nan != nan


class TrackerTest(unittest.TestCase):
    def feed(self, seqs):
        t = Tracker()
        for s in seqs:
            t.add(fake_record(s))
        return t.summary()

    def test_clean_stream(self):
        s = self.feed(range(50))
        self.assertEqual((s["dropped"], s["out_of_order"]), (0, 0))
        self.assertEqual(s["avg_rate_hz"], 10.0)
        self.assertEqual(s["worst_gap_ms"], 100)

    def test_drop_and_gap(self):
        s = self.feed([0, 1, 2, 5, 6])
        self.assertEqual(s["dropped"], 2)
        self.assertEqual(s["worst_gap_ms"], 300)

    def test_out_of_order_is_not_dropped(self):
        s = self.feed([0, 1, 3, 2, 4])
        self.assertEqual((s["dropped"], s["out_of_order"]), (0, 1))

    def test_duplicate(self):
        self.assertEqual(self.feed([0, 1, 1, 2])["duplicates"], 1)

    def test_bad_line_is_not_also_dropped(self):
        t = Tracker()
        t.add(fake_record(0))
        t.add_bad(make_line(fake_record(1)).replace("ONLINE", "0NLINE"))
        t.add(fake_record(2))
        self.assertEqual((t.summary()["bad"], t.summary()["dropped"]), (1, 0))


class FakeModeTest(unittest.TestCase):
    def test_counts_match_injected(self):
        for seed in (1, 2, 3):
            lines, truth = fake_stream(500, seed=seed)
            tracker, _, _ = groundstation.record(lines)
            s = tracker.summary()
            self.assertGreater(truth["dropped"] * truth["bad"] * truth["out_of_order"], 0)
            self.assertEqual(s["dropped"], truth["dropped"], f"seed {seed}")
            self.assertEqual(s["bad"], truth["bad"], f"seed {seed}")
            self.assertEqual(s["out_of_order"], truth["out_of_order"], f"seed {seed}")
            self.assertEqual(s["faults_seen"], 1, f"seed {seed}")  # only one real fault

    def test_fault_shows_up(self):
        lines, _ = fake_stream(200, drop=0, bad=0, swap=0)
        tracker, _, _ = groundstation.record(lines)
        self.assertEqual(tracker.fault_events, [(6000, "F02")])

    def test_cli_writes_files(self):
        with tempfile.TemporaryDirectory() as out:
            code = groundstation.main(["--fake", "300", "--out", out])
            self.assertEqual(code, 0)
            for name in ("telemetry.csv", "summary.txt", "plot.png", "bad_lines.txt"):
                self.assertTrue(os.path.getsize(os.path.join(out, name)) > 0, name)
            with open(os.path.join(out, "telemetry.csv")) as f:
                self.assertTrue(f.readline().startswith("seq,t_ms,pitch"))


if __name__ == "__main__":
    unittest.main()
