# ADR 0007: Minimal Heltec V3 prototype without GPS

- Status: Accepted
- Date: 2026-10-05
- Supersedes the LED-sharing implementation in [0005](0005-alert-patterns-and-led-sharing.md)
- Narrows the first deliverable in [0006](0006-prune-at-product-boundaries.md)

## Context

The first prototype needs a compiled, testable firmware for the existing Heltec
V3. GPS hardware is deferred. The board has a reset switch and one GPIO 0 user
switch; reset cannot provide a second application input. The existing LED also
has a radio TX writer, which makes timed blink patterns unreliable.

## Decision

- A single click wakes the OLED or advances its existing pages.
- A double click queues an encrypted SOS from any screen.
- A two-second hold acknowledges the newest pending received distress. When
  none is pending it selects the current screen action (Bluetooth/hibernate).
- Keep the onboard GPIO 35 LED exclusively for product indications. Disable
  the ordinary TX LED writer in this target. Three short pulses mean locally
  queued distress; two long pulses mean received distress or a matching reply
  to this device's current distress. OLED text identifies the state.
- Debounce the button and test gesture, held-at-boot, and timer rollover cases
  using the same code compiled into the firmware.
- Connect the OLED to the existing reference-counted Vext controller using
  Heltec's active-low enable polarity. Timeout cuts the OLED rail; waking
  reinitializes the display. Physical power measurements remain required.
- Compile only the Heltec hardware, SX1262 LoRa, OLED, BLE, USB companion
  transport, storage and essential MeshCore messaging/routing. No GPS,
  environmental sensor drivers, external RTC discovery, Wi-Fi/TCP, image/raw
  datagrams, remote administration, signing workflows or rescue filesystem CLI
  in the product runtime. Retain upstream sources for attribution and review.
- Keep the existing direct text/contact/group protocols; GPS coordinates
  remain optional envelope fields for later Flutter map integration.
- Enable bounded fleet forwarding for Uni-Fi group text, companion adverts,
  direct text and its ACK/PATH routing; cap flood forwarding at three hops and
  retain MeshCore duplicate suppression. This supports a small fleet without
  requiring a separate repeater. A group-channel hash is a traffic filter,
  not authentication; receivers still need the provisioned group key.
- Provide a small dependency-free hardware-test web client with BLE and USB.
  Add a read-only product status command (0x70) to report provisioning and
  hardware-button incident state without exporting the network key.

## Consequences

Hardware SOS and response do not depend on a phone or browser. All presets are
available through the test client. A single pending received distress and a
single current local distress are retained in firmware; the app can display
more incidents. RAM state expires after four hours and resets on power loss.
BLE remains available while the OLED sleeps; no claim of measured battery
life or RF reliability follows from a successful build or host tests.

GPS hardware, external controls, Flutter release verification and physical
two-/three-node validation are separate milestones. GPS will require a fresh
pinout review and an explicit target/provider configuration, not merely
turning on a preference in this GPS-free build.
