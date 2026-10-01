# Uni-Fi

**Uni-Fi — mesh network developed by MeshUSF** is an emergency-first, off-grid messaging system built on the open MeshCore protocol. The initial hardware is a Heltec LoRa 32 V3, band-matched whip antenna, and UART GPS. The radio remains useful without a phone; the phone adds the larger map, direct chat, QR contact import, and group-chat interface.

## Repository layout

- `firmware/` — trimmed MeshCore companion firmware and the locked Heltec V3 Uni-Fi target
- `app/` — Flutter web/mobile client derived from MeshCore Open
- `docs/` — architecture, provisioning, protocol, hardware, and product boundaries

The upstream MIT license files and source history documents remain in each source tree. Uni-Fi-specific work is also released under the MIT license at the repository root.

## Implemented product slice

- Fixed radio profile: 910.525 MHz, BW 62.5 kHz, SF7, CR 4/5
- A provisioned, encrypted `Uni-Fi` network channel in locked firmware slot 0
- Network PSK is compiled into each deployed radio but excluded from source control
- Device PSK export and replacement are blocked through the companion protocol
- Public advertisements use an opaque `UniFi-<key prefix>` label and contain no GPS; usernames and GPS presence travel inside the encrypted channel
- Encrypted presence is flooded every 15 minutes, deduplicated by node prefix, and retained locally for four hours
- Double-click sends an SOS without a phone
- One onboard LED is time-shared with radio TX: three short pulses after a sent alert and two long pulses for a received alert or response
- Emergency-first app with SOS, medical, pickup, safe, en-route, location, and responder acknowledgement
- Only recently authenticated Uni-Fi users appear in the main Contacts and Map views
- Distress markers remain on the map for four hours and change from red to green after an acknowledgement
- Direct messages, QR contact import, group channels, node naming, battery status, GPS, and screen timeout remain available

## Provision and build

The repository intentionally contains no deployable network secret.

```bash
cp firmware/examples/companion_radio/UniFiProvisioning.example.h \
   firmware/examples/companion_radio/UniFiProvisioning.h
openssl rand -base64 16
# Put that output in UNIFI_NETWORK_PSK_B64 in UniFiProvisioning.h.
# Use the same generated key only for devices in the same deployment.

cd firmware
pio run -e Heltec_v3_unifi_companion_ble_us
```

Client:

```bash
cd app
flutter pub get
flutter test
flutter run
```

See [architecture](docs/ARCHITECTURE.md), [security](docs/SECURITY.md), [hardware](docs/HARDWARE.md), [wire format](docs/EMERGENCY_PROTOCOL.md), [decision records](docs/decisions/README.md), and the [development log](docs/DEVELOPMENT_LOG.md).

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
