#include <cassert>
#include <cstdint>
#include <iostream>
#include "helpers/ui/UniFiButton.h"
#include "helpers/ui/UniFiLed.h"

using unifi::Button;

static void press(Button& b, uint32_t at) {
  assert(b.sample(true, at) == Button::None);
  assert(b.sample(true, at + 30) == Button::None);
}

static void release(Button& b, uint32_t at) {
  assert(b.sample(false, at) == Button::None);
  assert(b.sample(false, at + 30) == Button::None);
}

int main() {
  Button b;
  b.configure(2000);
  b.sample(false, 0);
  // Switch bounce must not produce a click.
  b.sample(true, 10);
  b.sample(false, 15);
  assert(b.sample(false, 500) == Button::None);
  press(b, 600);
  release(b, 700);
  assert(b.sample(false, 1009) == Button::None);
  assert(b.sample(false, 1010) == Button::Click);
  assert(b.sample(false, 1100) == Button::None);
  press(b, 1200);
  release(b, 1300);
  press(b, 1400);
  release(b, 1500);
  assert(b.sample(false, 1810) == Button::DoubleClick);
  press(b, 2000);
  assert(b.sample(true, 4029) == Button::None);
  assert(b.sample(true, 4030) == Button::Hold);
  assert(b.sample(true, 8000) == Button::None);
  release(b, 8100);
  assert(b.sample(false, 8500) == Button::None);
  // Press+hold after a pending click yields only hold, never a delayed SOS.
  press(b, 9000);
  release(b, 9100);
  press(b, 9200);
  assert(b.sample(true, 11230) == Button::Hold);
  release(b, 11300);
  assert(b.sample(false, 12000) == Button::None);

  Button held_at_boot;
  held_at_boot.sample(true, 0);
  assert(held_at_boot.sample(true, 5000) == Button::None);
  release(held_at_boot, 5100);
  assert(held_at_boot.sample(false, 5500) == Button::None);

  Button rollover;
  const uint32_t start = UINT32_MAX - 100;
  rollover.sample(false, start - 100);
  press(rollover, start);
  release(rollover, start + 60);
  assert(rollover.sample(false, start + 370) == Button::Click);

  unifi::AlertLed led;
  led.start(unifi::AlertLed::Sent, 0);
  assert(led.sample(0));
  assert(led.sample(159));
  assert(!led.sample(160));
  assert(led.sample(340));
  assert(!led.sample(500));
  assert(led.sample(680));
  assert(!led.sample(840));
  assert(!led.sample(2000));
  led.start(unifi::AlertLed::Received, 3000);
  assert(led.sample(3000));
  // Pending receive pattern has priority over send notifications.
  led.start(unifi::AlertLed::Sent, 3010);
  assert(led.sample(3649));
  assert(!led.sample(3650));
  assert(led.sample(3950));
  assert(!led.sample(4600));
  assert(!led.sample(6000));
  led.start(unifi::AlertLed::Received, UINT32_MAX - 10);
  assert(led.sample(UINT32_MAX - 10));
  assert(!led.sample(uint32_t(UINT32_MAX - 10 + 650u)));
  std::cout << "Uni-Fi controls: button gestures, debounce, boot-held, rollover and LED patterns passed\n";
}
