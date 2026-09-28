# Development log

This append-only log records meaningful implementation work, verification, and
known gaps. Architecture choices belong in `docs/decisions/`; release-facing
changes should also appear in the eventual changelog.

## 2026-09-28 — Initial Uni-Fi product slice

### Repository and scope

- Imported the supplied MeshCore firmware and MeshCore Open client sources into
  `firmware/` and `app/`, retaining their MIT licenses.
- Removed the redundant source archives after checking that the extracted trees
  were present.
- Added the Uni-Fi name, project README, root MIT license, architecture,
  security, hardware, protocol, product-scope, and architecture decision docs.
- Kept internal Flutter package/protocol identifiers where renaming would break
  compatibility; branded user-visible application and platform labels Uni-Fi.

### Firmware

- Added `Heltec_v3_unifi_companion_ble_us`, fixed at 910.525 MHz, BW 62.5 kHz,
  SF7, and CR 4/5, with companion radio changes disabled.
- Added an ignored provisioning header and locked encrypted `Uni-Fi` channel in
  slot 0. No production PSK is stored in the repository; an unedited template
  now fails its compile-time key-length check.
- Added opaque public advertisements without GPS, a 15-minute encrypted
  presence heartbeat, a 32-entry RAM roster, duplicate refresh, and four-hour
  expiry.
- Added phone-independent double-click SOS, OLED state, and distinct sent and
  received light patterns.
- Reduced the product target's library dependencies and offline/contact limits
  while leaving shared protocol code available for compatibility.

### Client

- Made Emergency the primary flow and added quick SOS, medical, pickup, safe,
  en-route, location, and acknowledgement messages.
- Added structured `mc:v1` envelope parsing with node-prefix validation.
- Added local `receivedAt` persistence, duplicate refresh, and four-hour
  retention independent of sender clock.
- Filtered Contacts and Map to recent encrypted Uni-Fi presence; kept QR import
  while removing public discovery from the primary product flow.
- Removed USB and TCP/local-network choices from the end-user scanner, leaving
  Bluetooth as the primary onboarding flow while retaining recovery code.
- Added distress map markers that turn from red to green after a matching
- Added encrypted presence and one-tap location markers for nodes whose full
  public-key advert has not arrived yet; they remain display-only until it does.
- Hid presence heartbeats from chat, notifications, and unread counts.
- Added unit coverage for emergency-envelope parsing and authorized presence.

### Verification and known gaps

- PlatformIO 6.2.0 configuration parsing passed for the Uni-Fi target,
  including fixed-profile flag ordering, locked product flags, GPS/OLED
  dependencies, and removed sensor dependencies.
- Local XML parsing, documentation-link checks, Dart delimiter checks, and the
  no-provisioning-secret check passed.
- A firmware build was attempted. It reached installation of
  `platformio/espressif32@6.11.0` and stopped with `HTTPClientError` because the
  environment cannot download the absent platform/toolchain packages; project
  source compilation therefore did not run here.
- Flutter and Dart SDKs are not installed in this environment, so formatting,
  analyzer, unit tests, and platform builds must run in CI or a Flutter-enabled
  workstation before release.
- Physical Heltec V3 testing remains mandatory for GPS pinout, double-click
  timing, OLED messages, LED polarity/patterns, RF interoperability, power
  consumption, range, and acknowledgement behavior.
