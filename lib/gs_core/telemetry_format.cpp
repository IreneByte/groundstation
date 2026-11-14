#include "telemetry_format.h"

#include <stdio.h>

uint8_t xorChecksum(const char* s, size_t n) {
  uint8_t cs = 0;
  for (size_t i = 0; i < n; i++) {
    cs ^= (uint8_t)s[i];
  }
  return cs;
}

int formatTelemetry(char* buf, size_t size, const TelemetryFrame& f) {
  // body first: everything from '$' up to (not including) '*'
  int n = snprintf(buf, size,
                   "$GS,%lu,%lu,%.2f,%.2f,%.1f,%.1f,%d,%d,%d,%.1f,%s,%s,%s,%lu,%lu,%lu,%lu",
                   (unsigned long)f.seq, (unsigned long)f.timeMs,
                   f.pitchDeg, f.rollDeg, f.speedL, f.speedR,
                   f.pwmL, f.pwmR, f.distanceCm, f.tempC,
                   f.state, f.fault, f.warn, (unsigned long)f.faultCount,
                   (unsigned long)f.loopAvgUs, (unsigned long)f.loopMaxUs,
                   (unsigned long)f.periodMaxUs);
  // need room for "*XX" and the ending '\0'
  if (n < 0 || (size_t)n + 4 > size) return -1;

  uint8_t cs = xorChecksum(buf + 1, (size_t)n - 1);  // skip the '$'
  int m = snprintf(buf + n, size - n, "*%02X", cs);
  return n + m;
}
