#pragma once

#include <stdint.h>

namespace unifi {

// Independent of GPIO and Arduino so the exact gesture logic can run in host tests.
class Button {
 public:
  enum Event { None = 0, Click = 1, Hold = 2, DoubleClick = 3, TripleClick = 4 };

  void configure(uint32_t hold_ms, bool multiclick = true) {
    hold_duration = hold_ms;
    click_window = multiclick ? 280 : 0;
  }

  void cancel() {
    clicks = 0;
    consumed = true;
  }

  Event sample(bool pressed, uint32_t now) {
    if (!initialized) {
      initialized = true;
      raw = stable = pressed;
      changed_at = pressed_at = now;
      // A button held through reset is not an emergency gesture.
      consumed = pressed;
      return None;
    }
    if (pressed != raw) {
      raw = pressed;
      changed_at = now;
    }
    if (raw != stable && uint32_t(now - changed_at) >= 30) {
      stable = raw;
      if (stable) {
        // Flush an expired sequence before starting a new one.
        if (clicks && uint32_t(now - released_at) >= click_window) clicks = 0;
        pressed_at = now;
        consumed = false;
      } else if (!consumed) {
        if (clicks < 3) ++clicks;
        released_at = now;
      }
    }
    if (stable && !consumed && hold_duration &&
        uint32_t(now - pressed_at) >= hold_duration) {
      consumed = true;
      clicks = 0;
      return Hold;
    }
    if (!raw && !stable && clicks && uint32_t(now - released_at) >= click_window) {
      const Event result = clicks == 1 ? Click : clicks == 2 ? DoubleClick : TripleClick;
      clicks = 0;
      return result;
    }
    return None;
  }

 private:
  bool initialized = false, raw = false, stable = false, consumed = false;
  uint8_t clicks = 0;
  uint32_t hold_duration = 2000, click_window = 280;
  uint32_t changed_at = 0, pressed_at = 0, released_at = 0;
};

}  // namespace unifi
