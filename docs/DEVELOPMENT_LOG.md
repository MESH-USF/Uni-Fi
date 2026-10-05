# Development log

This append-only log records meaningful implementation work, verification, and
known gaps. Architecture choices belong in `docs/decisions/`; release-facing
changes should also appear in the eventual changelog.

## 2026-10-01 — Repository synchronization

- Fetched the existing `MESH-USF/Uni-Fi` repository history over the
  authenticated HTTPS remote.
- Integrated its initial README and MIT license history without rewriting the
  remote branch; the README retains the complete Uni-Fi implementation notes.
- Kept `main` untouched and prepared the integrated history on
  `feat/unifi-initial-import` for review.

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

## 2026-10-05 — GPS-free Heltec V3 prototype and hardware-test client

### Decisions and implementation

- Started `feat/heltec-v3-prototype` from the organization's integrated main
  history. Recorded the one-button, minimal-runtime decisions in ADR 0007.
- Corrected the control assumption: RESET is not an application input. USER
  single-click wakes/pages, double-click queues SOS, and a two-second hold
  acknowledges the newest pending received distress. Holding through boot
  does not generate an emergency action.
- Added shared, host-testable debounce/gesture and nonblocking LED controllers.
  GPIO 35 now has one writer: three short pulses for queued distress, two long
  pulses for received distress or a matching remote receipt. OLED state
  distinguishes these outcomes; queued is not proof of radio delivery.
- Used active-low Vext power control for the OLED timeout and wake/reinit.
  Fixed alert-copy bounds and uninitialized preview/message counters.
- Isolated the product build from inherited GPS, environmental sensors,
  external RTC probing, Wi-Fi/TCP, private-key transfer, image/raw datagrams,
  remote administration, signing workflows and rescue filesystem CLI. Upstream
  source and licenses remain; these features are excluded from this target.
- Kept BLE/USB companion transport, stored device names, direct text, contacts,
  configured group channels, storage, battery reporting and essential routing.
  Locked the requested 910.525 MHz profile and enabled provisioned fleet
  forwarding with a three-hop flood cap and existing duplicate suppression.
- Replaced substring envelope checks with strict bounded parsing, compact
  messages and explicit UTF-8 length rejection instead of silent truncation.
  Added boot-random/sequence event IDs after radio RNG initialization.
- Added matching standalone receipt state, replay/duplicate suppression,
  wrap-safe four-hour local expiry and a 32-member RAM roster. Prioritized the
  64-frame offline queue over coalesced presence heartbeats. State and queues
  are bounded and do not promise lossless delivery under overload.
- Added read-only companion status command 0x70 so a connected client can
  verify provisioning and track physical-button incidents. USB activity
  expires after 30 seconds; the new client polls every five seconds.
- Added a private provisioning-header generator that refuses overwrite and
  never prints the key. The deployment header and provisioned images remain
  ignored and must not be published. Shared-key membership does not prove
  individual identity; short routing hashes are not authentication.
- Developed the dependency-free web client in parallel: real BLE/USB paths,
  device-name readback, presets/text, exact-event receipt responses, retained
  presence/incidents, serialized commands, offline draining and diagnostics.
  Its RAM roster is reconstructed from received messages, not a firmware dump.
- Deferred GPS hardware/driver wiring and maps; preserved optional coordinate
  envelopes and existing Flutter sources for the later mobile milestone.

### Verification

- PlatformIO toolchain downloads now work. Both isolated unprovisioned and
  private provisioned Heltec builds succeed; the latter produces a merged
  ESP32-S3 flash image at offset 0x0.
- Provisioned image uses 94,668 bytes static RAM (28.9%) and 1,189,513 bytes
  application flash (35.6% of its partition). These are build-size figures,
  not measured free heap, latency, battery life or savings against a baseline.
- Private merged-image SHA-256:
  `3edf3cc6340deba33f4c3518107a17e3333e762d15d87867a299cd131d145426`.
- Host controls tests pass; seven protocol test groups pass 4,426 assertions;
  all 20 Node web tests and all 40 existing MeshCore native tests pass.
  C++ host tests compile with strict warnings. Sanitizers could not run because
  the installed runtime library links are broken; this is not sanitizer coverage.
- Inspected the final ELF: no WiFiClass, MicroNMEA, AutoDiscoverRTCClock,
  rescue CLI handler or group-datagram sender symbols remain.
- Playwright checked the actual served browser UI at desktop and 390 x 844:
  no console errors/warnings or horizontal overflow; transmit controls remain
  disabled without a real initialized connection. Fixed the missing favicon.
  Screenshots stay ignored under `output/playwright/`.
- Added CI for controls/web, native routing tests and both provisioning build
  paths in separate build directories to avoid provisioning-cache reuse; CI
  intentionally does not upload deployment-key-bearing images. Binary checks
  confirm the private image contains this deployment key and the isolated
  unprovisioned image does not.

### Remaining acceptance work

- No physical serial device is attached here. RF, BLE pairing/MTU, OLED/Vext,
  button/LED electrical behavior, battery current and two-/three-node delivery
  tests remain **not run**; see `docs/PROTOTYPE_VALIDATION.md` for the checklist.
- Flutter/Dart SDKs are absent. This iteration does not claim a verified mobile
  build or working physical GPS/map flow. GPS, key rotation, enforceable
  moderation and extra hardware preset controls remain later milestones.
- Changes are recorded locally on the feature branch; this iteration does not
  push to GitHub or rewrite published history.

## 2026-10-05 — Ground-up technical walkthrough

- Added `docs/TECHNICAL_WALKTHROUGH.md` for a technical reader with no project
  context: inherited/current root trees, build inheritance, module ownership,
  feature implementation, startup/state and ten Mermaid diagrams.
- Documented the three protocol layers separately, including RF header/path,
  inherited AES-128 ECB and truncated HMAC, exact BLE/USB companion framing,
  send/receive/status offsets, envelope grammar and retained-command allowlist.
- Audited structure/history and protocol independently against the current
  code. Qualified the baseline: the initial import was already customized;
  the prototype excluded product functionality but deleted no source files.
- Explained bounded incident/queue state, semantic versus RF duplicates,
  local receipt-age differences, no automatic SOS retry, shared-key identity
  limits and the saved name exposed by current BLE advertising.
- Made Flutter/GPS limitations explicit, including unverified mobile builds,
  older parser/preset differences and missing product-status integration.
- Updated README and architecture/protocol/security summaries to link the
  walkthrough and remove contradictory GPS/privacy/receipt descriptions.
- Verified all ten Mermaid diagrams parse; checked 51 local document links,
  contents anchors, fence balance and six concrete protocol examples against
  the actual web codec and 160-byte budget. Host controls, 4,426 protocol
  assertions, 20 web tests and all 40 existing native cases pass again.
  No firmware/app implementation changes,
  key publication, hardware testing or GitHub push are part of this task.

## 2026-10-05 — Word and PDF technical report

- Converted the full walkthrough into editable
  `docs/Uni-Fi-Technical-Walkthrough.docx` and a matching 38-page PDF, with
  a cover, linked reading guide, page numbers, tables and ten embedded charts.
- Added reusable local chart-rendering and Word-export scripts plus
  `docs/DOCUMENT_EXPORT.md`; dependency installs and intermediate screenshots
  remain outside committed product dependencies/artifacts.
- Corrected Mermaid label line breaks and state-transition punctuation;
  combined duplicate self-loop labels so both behaviors appear in the chart.
  These are presentation corrections, not firmware behavior changes.
- Used Playwright to render all charts with no browser console errors or
  warnings. Visually reviewed module, SOS and build charts and protocol-table
  PDF pages; portrait charts flow with nearby text and wide charts use
  landscape pages.
- Independent content audit found all 50 headings, 50 list items, 97 unchanged
  prose paragraphs, 478 table cells and 17 code/tree blocks retained. All ten
  embedded images match the refreshed renders; 50 bookmarks, 19 reading-guide
  links and 16 repeating table headers are present. Only the introductory
  Markdown/Mermaid-format sentence is adapted for the rendered document.
- Host controls, 4,426 protocol assertions, 20 browser protocol/session tests
  and all 40 inherited native test cases pass again. No additional physical
  hardware or Flutter validation and no GitHub push are claimed.
