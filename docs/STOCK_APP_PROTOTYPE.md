# Uni-Fi 0.3.1: original MeshCore app prototype

Current target: `Heltec_v3_unifi_stock_ble_us`. The earlier
`Heltec_v3_unifi_companion_ble_us` remains a separate private-network build.
Its SOS, receipts, encrypted presence and four-hour roster do not run in the
stock-app target. Earlier Word/PDF walkthroughs describe that older slice.

## Two action pages

| Selected OLED page | Triple USER/PRG press |
| --- | --- |
| Home: `Uni-Fi / Public` | Standard Public-channel preset chat message |
| Next: `Location broadcast` | Signed public simulated-location advertisement |
| Recent / Radio / Bluetooth / Shutdown / message preview | No transmission |

Single press wakes the screen or advances to the next page. Let a navigation
click finish before starting the next gesture (at least 280 ms after release).
Double press goes backward; hold selects the page's existing action. This is
a page selector, not a persistent transmit-mode switch or an automatic beacon.
Triple press wakes a sleeping display and acts on its already-selected page.

## Home-page Public chat

The preset is:

```text
<saved name>: TEST: Uni-Fi check-in. SIM GPS: 28.0587, -82.4139.
```

The firmware locates the configured standard Public channel by its actual
key, not its name or slot number. It does not create/replace saved channels.
If Public is absent or the packet pool is full, the OLED reports
`Public send failed`; otherwise it reports `Public msg queued`.

It calls the existing MeshCore group-text sender with a unique RTC timestamp,
plain-text message type and saved-name prefix. MeshCore then encrypts/MACs
the standard group packet using the publicly known Public-channel key and
queues its flood. There is no custom encryption module or private Uni-Fi key.
The short fixed text plus the maximum saved name fits the 160-byte text budget.

On a **second receiving node**, MeshCore processes that group text and queues
the standard channel-message frame for the app, including its normal waiting
notification. Its app should display this as Public chat, not as a map update.
The sender's own app does **not** receive a synthetic echo of hardware sends;
check the second node's app to verify radio reception. Neither OLED success
nor a synthetic local echo would establish remote delivery.

## Location-page advertisement

```text
Location page + three USER/BOOT clicks (GPIO 0)
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

Both actions work with no phone attached. The receiving node handles the radio
packet independently; a phone is needed only for its richer contact/map UI.
No chat message, SOS, delivery acknowledgement or automatic presence timer is
added by the location-page action. "Queued" means local acceptance, not remote delivery.
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
6. On the home page, press USER three times quickly. Expect `Public msg queued`.
7. On a second node's app with the standard Public channel configured, check
   Public chat for the saved name, test check-in and simulated coordinates.
   A stock app may not show the sender's hardware-originated text on its own
   connected phone. Match any non-default flood scopes as well as the waveform.
8. Wake the sender's screen if needed, then single-click to `Location broadcast`.
   Pause after navigation, then triple-click. Expect `SIM advert queued`.
9. On the second node's app, inspect the received advertisement/contact and its
   coordinates. Discovery/auto-add and map display depend on app settings.
10. Triple-click on Radio or Bluetooth: nothing should be transmitted.

Existing saved preferences/channels survive flashing. A previously provisioned
device may retain its old channel; configure Public using the app if needed.
No private provisioning header is loaded by this target, even if one exists
locally. The radio waveform is locked; other advanced settings may report
unsupported because the original minimal companion pruning is retained.

Button behavior: single wakes/advances, double wakes/navigates backward, hold
selects, triple performs only the selected action page. RESET remains hardware reset, not a second
application button. No new LED distress pattern is assigned to this advert.

## Clean scope and verification

The implementation reuses the existing button detector, advert encoder,
group-text sender, channel-key lookup and OLED. No GPS abstraction, custom packet
schema, new app, cloud service, or encryption replacement is introduced.
The stock companion role does not relay others' packets; compatible repeaters
may relay its flood. Ordinary app-triggered adverts retain their existing
location policy; the simulated fix is used only by the triple-press action.

Automated checks cover gesture classification (including navigation followed
by a separate triple) and advert encoding, plus
inherited protocol/native/browser regression tests and ESP32 compilation.
The native target does not execute the hardware-dependent UI/group sender;
page selection and Public-chat reception still require the physical procedure.
They cannot prove actual BLE pairing, button timing, RF delivery, battery
performance or stock-app map behavior. Test those on two physical nodes before
claiming end-to-end compatibility. Real GPS and Flutter work remain deferred.
