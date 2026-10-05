#pragma once

#include <stdint.h>

namespace unifi {

class AlertLed {
 public:
  enum Pattern { None, Sent, Received };

  void start(Pattern next, uint32_t now) {
    if (pattern == Received && next == Sent) return;
    pattern = next;
    remaining = next == Received ? 2 : next == Sent ? 3 : 0;
    lit = false;
    deadline = now;
  }

  bool sample(uint32_t now) {
    if (pattern == None || int32_t(now - deadline) < 0) return lit;
    if (lit) {
      lit = false;
      if (--remaining == 0) pattern = None;
      deadline = now + (pattern == Received ? 300 : 180);
    } else {
      lit = true;
      deadline = now + (pattern == Received ? 650 : 160);
    }
    return lit;
  }

 private:
  Pattern pattern = None;
  uint8_t remaining = 0;
  bool lit = false;
  uint32_t deadline = 0;
};

}  // namespace unifi
