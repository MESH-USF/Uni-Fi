# Uni-Fi

**Uni-Fi — mesh network developed by MeshUSF** is an emergency-first, off-grid messaging system built on the open MeshCore protocol. The prototype uses a Heltec LoRa 32 V3 and band-matched whip antenna. GPS is deferred. The radio can send SOS and acknowledgement without a phone; Flutter retains the foundations for maps, direct chat, QR import and groups.

## Repository layout

- `firmware/` — trimmed MeshCore companion firmware and the locked Heltec V3 Uni-Fi target
- `app/` — Flutter web/mobile client derived from MeshCore Open
- `web/` — lightweight browser client for real-device BLE/USB prototype testing
- `docs/` — architecture, provisioning, protocol, hardware, and product boundaries

The upstream MIT license files and source history documents remain in each source tree. Uni-Fi-specific work is also released under the MIT license at the repository root.

## Implemented product slice

- Fixed radio profile: 910.525 MHz, BW 62.5 kHz, SF7, CR 4/5
- A provisioned, encrypted `Uni-Fi` network channel in locked firmware slot 0
- Network PSK is compiled into each deployed radio but excluded from source control
- Device PSK export and replacement are blocked through the companion protocol
- Public advertisements use an opaque `UniFi-<key prefix>` label and contain no GPS; usernames travel inside encrypted presence, with coordinates reserved for later
- Encrypted presence is flooded every 15 minutes, deduplicated by node prefix, and retained locally for four hours
- Double-click sends SOS; a two-second hold acknowledges pending distress without a phone
- GPIO 35 LED is dedicated to alerts: three short pulses for local acceptance and two long pulses for received distress or a matching response
- Fleet forwarding with a three-hop flood cap and the original MeshCore duplicate suppression
- Minimal build excludes GPS, environmental sensors, external RTC discovery, Wi-Fi/TCP, raw/image datagrams, remote administration and filesystem rescue CLI
- Web test client with SOS, medical, pickup, safe, en-route and matching receipt responses
- Inherited Flutter Emergency/Contacts/Map foundations filter encrypted presence and correlate distress acknowledgements; mobile builds and GPS/map flows remain unverified
- Firmware retains direct messages, contacts, group channels, saved names, battery status and screen timeout; QR/maps remain inherited Flutter foundations

## Provision and build

The repository intentionally contains no deployable network secret.

```bash
python3 scripts/provision-unifi.py
# The ignored header and compiled binaries contain a private deployment key.
# Flash the same build to devices belonging to this deployment.

cd firmware
pio run -e Heltec_v3_unifi_companion_ble_us -t clean
pio run -e Heltec_v3_unifi_companion_ble_us
pio run -e Heltec_v3_unifi_companion_ble_us -t mergebin
```

Hardware-test web client (Chrome/Edge desktop):

```bash
python3 -m http.server 8765 --bind 127.0.0.1 --directory web
# Open http://localhost:8765
```

Automated firmware-control and browser-protocol tests:

```bash
bash scripts/test-unifi.sh
cd firmware
pio test -e native
```

Flutter client (SDK required; mobile builds remain a separate validation task):

```bash
cd app
flutter pub get
flutter test
flutter run
```

See [architecture](docs/ARCHITECTURE.md), [security](docs/SECURITY.md), [hardware](docs/HARDWARE.md), [wire format](docs/EMERGENCY_PROTOCOL.md), [decision records](docs/decisions/README.md), and the [development log](docs/DEVELOPMENT_LOG.md).

For a ground-up explanation with original/current directory trees, module
diagrams, feature implementations and exact RF/companion/event layouts, read
the [technical walkthrough](docs/TECHNICAL_WALKTHROUGH.md).

The compiled prototype and host tests do not establish physical RF/BLE or
button/LED performance. Use the [hardware acceptance procedure](docs/PROTOTYPE_VALIDATION.md)
before claiming those behaviors work on a device.

## Git history

This workspace includes a real Git repository and a small wrapper at
`scripts/gitw`. The wrapper exists because the managed development environment
mounts the conventional root `.git` path read-only. Use it exactly like Git:

```bash
./scripts/gitw status
./scripts/gitw log --oneline
```

See [repository maintenance](docs/REPOSITORY.md) for the layout and the one-time
conversion to a conventional `.git` directory outside this restricted
environment.

## Safety and spectrum notice

Uni-Fi is a coordination aid, not a guaranteed life-safety service. “Sent” means accepted by the local radio; only a received acknowledgement indicates that another network member responded. RF use, antenna choice, output power, certification, and the exact permitted operating parameters must be verified for the deployment jurisdiction before transmission. The included target implements the requested 910.525 MHz profile and is not a substitute for regulatory review.
