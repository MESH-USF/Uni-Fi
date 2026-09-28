# Architecture and load distribution

The radio is the trusted, always-available core. The phone is a replaceable interface and history/cache layer.

```text
GPS -> Heltec V3 -> MeshCore encrypted RF flood -> other Uni-Fi radios
          |
          +-- button, OLED, LED, 4-hour RAM roster
          |
          +-- BLE/USB companion frames -> Uni-Fi app -> map/chat/history
```

## Why the radio owns the critical path

- It already owns MeshCore identity, encryption, routing, and RF timing.
- A hardware SOS must work without BLE or a running phone app.
- A compiled network key does not need to be exported to the phone.
- GPS presence can continue at low duty cycle while the phone sleeps.
- Local receipt time avoids trusting an unset or incorrect sender clock.

The Heltec therefore owns the network key, locked radio parameters, opaque public advertisement, encrypted username/GPS heartbeat, four-hour RAM presence table, emergency button, offline queue, and LED/OLED feedback.

## Why the phone owns the extended experience

Maps, searchable history, QR scanning, group management, and rich chat consume storage, display area, and user input that the Heltec does not have. None is required to enqueue or receive an emergency packet. Replacing or disconnecting the phone does not change radio identity or network membership.

## Data flow

1. Each radio emits an ordinary opaque MeshCore advert for routing/contact correlation.
2. Every 15 minutes it floods a `presence` envelope on the encrypted Uni-Fi channel.
3. A provisioned receiver decrypts the sender name, node prefix, and optional GPS.
4. Firmware upserts the node prefix in a 32-entry RAM table using local receive time.
5. The companion app receives the plaintext channel frame and records its own `receivedAt` time.
6. Contacts and Map correlate the six-byte prefix with the full advertised public key; unrelated adverts stay hidden.
7. After four hours without a valid envelope, both layers stop presenting that node as available.

## Resource choices

- Presence interval: 15 minutes, giving up to 16 refresh opportunities during the four-hour window
- Firmware roster: 32 entries in RAM, no flash wear and no stale roster after power loss
- Offline queue: 64 frames, enough for several presence cycles plus emergency traffic without the previous 256-frame RAM cost
- GPS poll: 60 seconds; presence packets use the latest valid fix

## Protocol compatibility

Uni-Fi does not replace MeshCore routing or cryptography. It reserves a locked group-channel slot and adds a readable `mc:v1` application envelope. Direct messages and other channels continue to use upstream companion and over-the-air formats.
