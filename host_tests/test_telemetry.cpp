// Host test for lib/gs_core/telemetry_format and loop_stats.
// Build: see run_host_tests.sh
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "loop_stats.h"
#include "telemetry_format.h"

static TelemetryFrame sample() {
  TelemetryFrame f = {42, 8123, 1.25f, -0.4f, 60.0f, 58.5f, 141, 147, 57, 23.4f,
                      "MANUAL", "-", "-", 0, 812, 1430, 20110};
  return f;
}

static void testChecksum() {
  assert(xorChecksum("", 0) == 0);
  assert(xorChecksum("A", 1) == 0x41);
  assert(xorChecksum("AB", 2) == (0x41 ^ 0x42));
}

static void testFormat() {
  char buf[160];
  TelemetryFrame f = sample();
  int n = formatTelemetry(buf, sizeof(buf), f);
  printf("  %s\n", buf);
  assert(n == (int)strlen(buf));
  const char* body = "$GS,42,8123,1.25,-0.40,60.0,58.5,141,147,57,23.4,MANUAL,-,-,0,812,1430,20110*";
  assert(strncmp(buf, body, strlen(body)) == 0);

  // checksum at the end must match the XOR of the body
  const char* star = strchr(buf, '*');
  assert(star != NULL && strlen(star) == 3);
  unsigned long cs = strtoul(star + 1, NULL, 16);
  assert(cs == xorChecksum(buf + 1, (size_t)(star - buf - 1)));
}

static void testBadTempPrintsNan() {
  char buf[160];
  TelemetryFrame f = sample();
  f.tempC = NAN;
  formatTelemetry(buf, sizeof(buf), f);
  assert(strstr(buf, ",nan,") != NULL);
}

static void testBufferTooSmall() {
  char buf[40];
  TelemetryFrame f = sample();
  assert(formatTelemetry(buf, sizeof(buf), f) == -1);
}

static void testLoopStats() {
  LoopStats s;
  statsReset(s);
  assert(statsAvg(s) == 0);
  statsAdd(s, 100);
  statsAdd(s, 300);
  statsAdd(s, 200);
  assert(statsAvg(s) == 200 && s.maxUs == 300 && s.count == 3);
  statsReset(s);
  assert(s.maxUs == 0 && s.count == 0);
}

int main() {
  testChecksum();
  testFormat();
  testBadTempPrintsNan();
  testBufferTooSmall();
  testLoopStats();
  printf("test_telemetry: all passed\n");
  return 0;
}
