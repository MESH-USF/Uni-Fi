# Uni-Fi 0.3.0: original MeshCore app prototype

Current target: `Heltec_v3_unifi_stock_ble_us`. The earlier
`Heltec_v3_unifi_companion_ble_us` remains a separate private-network build.
Its SOS, receipts, encrypted presence and four-hour roster do not run in the
stock-app target. Earlier Word/PDF walkthroughs describe that older slice.

## What happens when you press three times?

```text
Three USER/BOOT clicks (GPIO 0)
  → debounced gesture detector (30 ms, 280 ms inter-click window)
  → Uni-Fi OLED UI calls sendSimulatedLocationAdvert()
  → MeshCore builds and signs a standard CHAT advertisement
  → radio queues a flood at 910.525 MHz
  → nearby matching MeshCore node verifies it and updates contact/location
  → original MeshCore app receives the ordinary contact/advert notification
```

The name is the saved device name; change it using the stock app. The location
is always the test point **28.0587, -82.4139**, not a GPS fix. Other devices will
report that test point too. The OLED says simulated GPS, but the standard
advertisement has no simulation flag, so receiving apps cannot distinguish it
from a real fix. Do not use this build for actual distress/location reporting.

The action works with no phone attached. The receiving node handles the radio
packet independently; a phone is needed only for its richer contact/map UI.
No chat message, SOS, delivery acknowledgement or automatic presence timer is
added by this action. "Queued" means local acceptance, not remote delivery.
Very rapid identical adverts within one RTC second can be deduplicated.

## Standard packet, not a new protocol

The broadcast is signed, **not encrypted**. Standard MeshCore flood routing
sets the route and path fields. Advertisement payload:

| Field | Size / value |
| --- | --- |
| Node public key | 32 bytes |
| RTC timestamp | 4 bytes |
| Ed25519 signature | 64 bytes |
| App-data flags | 1 byte, `0x91` (chat + location + name) |
| Latitude | signed int32 microdegrees, approximately `28058700` |
| Longitude | signed int32 microdegrees, approximately `-82413900` |
| Saved name | remaining name bytes, bounded by MeshCore advert capacity |

Multibyte integers use the inherited little-endian encoding on ESP32.
Coordinates are converted with the upstream double-to-int truncation.
The 32-byte advert app-data budget leaves **23 UTF-8 bytes for the name**
when coordinates are included; longer saved names are truncated at a complete
UTF-8 character by the stock encoder.
MeshCore handles signatures, packet allocation, LoRa scheduling and receive
notifications unchanged. See [upstream packet format](https://github.com/meshcore-dev/MeshCore/blob/main/docs/packet_format.md)
and local `BaseChatMesh.cpp`, `Mesh.cpp`, and `AdvertDataHelpers.cpp`.

Anyone receiving a matching MeshCore radio waveform can read the name and
coordinates; there is no authorized-fleet filter. Normal chat still uses
MeshCore's standard identity/channel cryptography. "No encryption module"
here means no custom Uni-Fi private channel provisioning, not removal of the
upstream cryptographic primitives required for app interoperability.

## Setup in VS Code / PlatformIO

1. Open `firmware/` with PlatformIO and select `Heltec_v3_unifi_stock_ble_us`.
2. Build, then upload over USB with an appropriate antenna attached.
3. Pair the original MeshCore app over BLE using the PIN shown on the OLED.
4. Set your saved device name. A fresh device has the standard Public channel.
5. Match peers/repeaters to 910.525 MHz, BW 62.5 kHz, SF7, CR4/5.
6. Press USER three times quickly. The screen should say `SIM location queued`.
7. On a second node's app, inspect the received advertisement/contact and its
   coordinates. Discovery/auto-add and map display depend on app settings.

Existing saved preferences/channels survive flashing. A previously provisioned
device may retain its old channel; configure Public using the app if needed.
No private provisioning header is loaded by this target, even if one exists
locally. The radio waveform is locked; other advanced settings may report
unsupported because the original minimal companion pruning is retained.

Button behavior: single wakes/advances, double wakes/navigates backward, hold
selects, triple broadcasts. RESET remains hardware reset, not a second
application button. No new LED distress pattern is assigned to this advert.

## Clean scope and verification

The implementation reuses the existing button detector, advert encoder,
stock companion text operations and OLED. No GPS abstraction, custom packet
schema, new app, cloud service, or encryption replacement is introduced.
The stock companion role does not relay others' packets; compatible repeaters
may relay its flood. Ordinary app-triggered adverts retain their existing
location policy; the simulated fix is used only by the triple-press action.

Automated checks cover gesture classification and advert encoding, plus
inherited protocol/native/browser regression tests and ESP32 compilation.
They cannot prove actual BLE pairing, button timing, RF delivery, battery
performance or stock-app map behavior. Test those on two physical nodes before
claiming end-to-end compatibility. Real GPS and Flutter work remain deferred.
