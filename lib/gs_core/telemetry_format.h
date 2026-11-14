#pragma once
// ===== TELEMETRY MESSAGE FORMAT (the one place it is written down) =====
//
// One text line per message, about 10 per second, sent on the WebSocket
// and printed on Serial. Fields are comma separated:
//
//   $GS,seq,t_ms,pitch,roll,spd_l,spd_r,pwm_l,pwm_r,dist_cm,temp_c,
//       state,fault,warn,faults,loop_avg_us,loop_max_us,period_max_us*CS
//
//   seq            counts up by 1 every message (uint32). Used to find
//                  dropped and out-of-order messages.
//   t_ms           millis() when the message was built
//   pitch, roll    filter output in degrees (2 decimals)
//   spd_l, spd_r   wheel speed in encoder ticks per second (1 decimal)
//   pwm_l, pwm_r   PID output 0..255
//   dist_cm        HC-SR04 distance, 0 = no echo
//   temp_c         DHT22 temperature (1 decimal), "nan" if the read failed
//   state          IDLE / ONLINE / MANUAL / FAULT / RESET_REQUIRED
//   fault          F01..F04 or "-"   (active fault)
//   warn           W01, W02 or "-"
//   faults         total faults since boot (fault history count)
//   loop_avg_us    control loop run time, average over the last ~100 ms
//   loop_max_us    control loop run time, worst over the last ~100 ms
//   period_max_us  worst time between two control loop wake-ups
//   CS             checksum: XOR of every byte between '$' and '*',
//                  written as 2 uppercase hex digits (like NMEA)
//
// Example:
//   $GS,42,8123,1.25,-0.40,60.0,58.5,141,147,57,23.4,MANUAL,-,-,0,812,1430,20110*2D
//
// Lines that start with '#' are human log lines (serial only), not telemetry.
#include <stddef.h>
#include <stdint.h>

struct TelemetryFrame {
  uint32_t seq;
  uint32_t timeMs;
  float pitchDeg, rollDeg;
  float speedL, speedR;
  int pwmL, pwmR;
  int distanceCm;
  float tempC;
  const char* state;
  const char* fault;
  const char* warn;
  uint32_t faultCount;
  uint32_t loopAvgUs, loopMaxUs, periodMaxUs;
};

// XOR of n bytes
uint8_t xorChecksum(const char* s, size_t n);

// Write one full line (no newline) into buf. Returns the length,
// or -1 if buf is too small. 160 bytes is enough.
int formatTelemetry(char* buf, size_t size, const TelemetryFrame& f);
