# Heltec V3 prototype validation

This document distinguishes a compiled prototype from a tested physical
device. Record hardware results here or in a dated development-log entry;
do not infer them from automated tests.

## Automated checks

Run from the repository root:

```bash
bash scripts/test-unifi.sh
pio test -d firmware -e native
pio run -d firmware -e Heltec_v3_unifi_companion_ble_us -t mergebin
```

The first command compiles and executes the firmware's actual pure C++
button, LED, envelope, retention, emergency and queue logic on the host,
then runs the browser protocol/session tests. Existing MeshCore native tests
cover routing, preferences, serialization, packet deduplication and UTF-8.
The final command compiles the ESP32-S3 code and creates a merged flash image.

CI builds both without a key and with a fresh private key. An unprovisioned
build must still compile and report an unprovisioned status; it cannot send
Uni-Fi events. CI does not publish provisioned binary artifacts.

## First physical acceptance: two devices

Use two Heltec V3 boards with antennas and the same provisioned build.
Keep a record of exact board revisions and image checksum. The browser client
can be served with `python3 -m http.server 8765 --bind 127.0.0.1 --directory web`.
Connect using Chrome/Edge at `http://localhost:8765`.

| Test | Required observation | Current physical result |
| --- | --- | --- |
| Boot without GPS | OLED boots; radio and BLE remain responsive; no GPS required | Not run |
| Name persistence | Rename through web, restart, read back the same name | Not run |
| Locked profile/key | 910.525 MHz / BW62.5 / SF7 / CR5; channel 0 key masked; changes refused | Not run |
| Single click | Wake an off screen; otherwise advance pages, with no transmit | Not run |
| Double-click A | One SOS queued, three short pulses; A says awaiting ACK | Not run |
| Receive on B | Sender name and SOS appear; two long pulses; B offers hold-to-ACK | Not run |
| Hold B for two seconds | One response queued for A's exact event ID; no accidental SOS | Not run |
| Match ACK on A | Two long pulses; A OLED and web status show acknowledged | Not run |
| Unrelated/self ACK | No success state or receipt pattern on A | Not run |
| App presets | SOS, medical, pickup, safe, enroute and text arrive from the named sender | Not run |
| Phone absent | Repeat physical SOS/hold workflow with both clients disconnected | Not run |
| Reconnect | Pending messages drain; physical SOS state is readable in browser | Not run |
| Different deployment key | C cannot decrypt/present A/B presence or SOS | Not run |
| Display timeout | OLED sleeps after ten seconds; messages and button SOS still work | Not run |
| USB disconnect | App-connected state clears within 30 seconds; no false connected status | Not run |
| Battery | Measure current with OLED on/off; verify low-battery behavior | Not run |

For Bluetooth, verify OS pairing using the displayed PIN, the supported
browser/OS combination, complete GATT frame writes, disconnect/reconnect and
recovery after reset. USB provides a second test path if pairing is unavailable.
Use one controlling client per device at a time: the MeshCore command stream
has no request IDs for independent simultaneous controllers.

## Mesh acceptance: three devices

Place A and C so they cannot hear each other directly, with B providing the
only usable relay path. A's presence, opaque signed advert and SOS must reach
C; C's receipt must return to A. Then test direct text A-to-C after contact
discovery. Remove B and confirm the client does not claim remote receipt.

Capture event IDs and frame diagnostics. Confirm duplicates do not create
extra incidents or repeated blink alerts and that ordinary floods stop at the
three-hop forwarding cap. Measure range and airtime on the intended antenna
and radio profile rather than extrapolating from desk testing.

## Retention and bounded capacity

Firmware keeps 32 recently heard members in RAM for four hours using local
monotonic receive time; duplicates refresh the record. The browser maintains
its own RAM view from drained/received events and clears it on reload. It is
not a full dump of the firmware roster.

The 64-frame offline queue coalesces heartbeat entries and prioritizes
distress/ACK/direct messages over presence. Finite queues cannot guarantee
delivery during prolonged disconnection or sustained overload. Verify queue
behavior under realistic traffic and retain the newest emergency state on
the standalone display independently of browser availability.

## Deferred milestones

GPS module/pinout/driver integration, valid-fix and stale-fix rules, Flutter
mobile builds/tests, location/map flows, QR onboarding regression testing,
multiple preset hardware actions, enforceable moderation and key rotation.
