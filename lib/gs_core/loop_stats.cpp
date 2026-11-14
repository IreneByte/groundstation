#include "loop_stats.h"

void statsReset(LoopStats& s) {
  s.count = 0;
  s.sumUs = 0;
  s.maxUs = 0;
}

void statsAdd(LoopStats& s, uint32_t us) {
  s.count++;
  s.sumUs += us;
  if (us > s.maxUs) s.maxUs = us;
}

uint32_t statsAvg(const LoopStats& s) {
  if (s.count == 0) return 0;
  return (uint32_t)(s.sumUs / s.count);
}
