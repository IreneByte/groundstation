#pragma once
// Average and worst loop time over a window. Plain C++.
#include <stdint.h>

struct LoopStats {
  uint32_t count;   // samples in this window
  uint64_t sumUs;   // total time, for the average
  uint32_t maxUs;   // worst sample
};

void statsReset(LoopStats& s);
void statsAdd(LoopStats& s, uint32_t us);
uint32_t statsAvg(const LoopStats& s);  // 0 if empty
