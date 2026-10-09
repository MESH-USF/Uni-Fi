# Uni-Fi

**Uni-Fi — mesh network developed by MeshUSF** is an off-grid messaging prototype built on MeshCore for Heltec LoRa 32 V3. The current prototype targets the original MeshCore app: triple-press the USER button to flood a public advertisement containing the saved name and a simulated location. Real GPS is deferred.

## Current stock-app prototype

- Build target: `Heltec_v3_unifi_stock_ble_us` (Uni-Fi 0.3.0).
- Uni-Fi / MeshUSF OLED branding, BLE pairing PIN, message preview and screen timeout.
- Three short USER/BOOT presses broadcast the saved node name and test coordinates **28.0587, -82.4139**. No phone or GPS module is needed.
- Standard signed, unencrypted MeshCore advertisement; no private Uni-Fi provisioning key or custom mobile decoder.
- Standard contacts, direct/group text, Public channel, BLE and USB companion interfaces remain; advanced pruned commands are not promised.
- Radio remains locked to 910.525 MHz / 62.5 kHz / SF7 / CR4/5; peers must match. This companion does not relay other nodes' traffic; repeaters provide additional hops.
- Double press navigates backward, hold selects. The old SOS/receipt/presence module is **not enabled** in this target.

Normal MeshCore messaging still uses its built-in cryptography. Removing that would break stock protocol compatibility. Simulated coordinates are marked on the OLED, **not tagged as fake in the standard packet**; never treat them as a person's real location. See [setup, packet flow and hardware checks](docs/STOCK_APP_PROTOTYPE.md).

```bash
cd firmware
pio run -e Heltec_v3_unifi_stock_ble_us -t mergebin
pio run -e Heltec_v3_unifi_stock_ble_us -t upload
```

No provisioning step is required. Stock-app pairing and RF reception still need physical hardware verification. The custom `web/` client and exported walkthroughs below describe the earlier private-network prototype, not this stock-app target.

## Repository layout

- `firmware/` — trimmed MeshCore companion firmware and the locked Heltec V3 Uni-Fi target
- `app/` — Flutter web/mobile client derived from MeshCore Open
- `web/` — lightweight browser client for real-device BLE/USB prototype testing
- `docs/` — architecture, provisioning, protocol, hardware, and product boundaries

The upstream MIT license files and source history documents remain in each source tree. Uni-Fi-specific work is also released under the MIT license at the repository root.

## Earlier private-network product slice (retained separately)

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

## Provision and build the earlier private-network target

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
Formatted copies with rendered charts are available as an editable
[Word document](docs/Uni-Fi-Technical-Walkthrough.docx) and a
[PDF](docs/Uni-Fi-Technical-Walkthrough.pdf). See the
[document export guide](docs/DOCUMENT_EXPORT.md) to regenerate them.
For a shorter introduction, see the [Uni-Fi quick guide](docs/UNIFI_QUICK_GUIDE.md),
available as [Word](docs/Uni-Fi-Quick-Guide.docx) and [PDF](docs/Uni-Fi-Quick-Guide.pdf).

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
