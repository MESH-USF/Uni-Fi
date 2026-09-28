# Product scope

## Product goal

Common mesh actions should be understandable without teaching users radio terminology. A deployed radio must retain the minimum emergency workflow when its phone is absent, discharged, or intentionally off.

## Primary workflow

1. Connect to a provisioned radio over Bluetooth.
2. Land on Emergency.
3. Send an SOS or fixed status with one action.
4. Include the device username and GPS fix automatically.
5. Show authenticated users and active incidents for four hours.
6. Let a responder acknowledge an incident and show that state to the sender.
7. Use Contacts for direct messages, Channels for groups, and Map for nearby authorized nodes.

## Radio responsibilities

- Own identity, RF configuration, network group key, encryption, and packet forwarding
- Read GPS and send a 15-minute encrypted presence heartbeat
- Maintain a 32-entry, four-hour presence table in RAM; a duplicate node prefix refreshes local receipt time
- Send an SOS through a double-click without a phone
- Display send/failure state on OLED and play deterministic single-LED alert patterns
- Queue received messages while the app is disconnected

## Phone responsibilities

- BLE onboarding and device status
- Emergency buttons and response acknowledgement
- Four-hour authorized roster, location map, and incident state
- QR contact import, direct chat, and simple group chat
- Longer history and settings that do not undermine the locked network profile

See [architecture](ARCHITECTURE.md) for why these duties are split this way.

## Kept from upstream

- MeshCore over-the-air routing, encryption primitives, and companion frames
- Direct private messages and group channels
- BLE as the only end-user onboarding path; USB/TCP transport foundations are
  retained for development recovery and protocol compatibility
- Device-specific name storage
- QR contact import
- GPS map, battery telemetry, OLED timeout, and low-battery shutdown

## Removed or hidden from the primary flow

- Public discovery and public-advert controls
- USB serial and TCP/local-network choices in the end-user scanner
- User-editable waveform settings on the Uni-Fi firmware target
- Public channel as the emergency transport
- Presence-heartbeat chat rows, notifications, and unread counts
- Advanced functionality is kept out of the four-destination navigation unless it supports Emergency, Contacts, Channels, or Map

Large inherited modules such as image codecs, local translation, repeater administration, and route diagnostics remain candidates for a later removal pass after release builds can be verified on every supported platform. They are not part of the Uni-Fi product contract.

## Deferred

- Per-device signatures for group messages
- Enforceable channel bans and moderator roles
- Network-key rotation UI
- External RGB status light
- Configurable hardware button combinations
- Offline map bundles

## Safety boundary

Delivery depends on RF conditions, matching profiles, powered relay nodes, correct antenna installation, and applicable spectrum rules. The UI must never equate local enqueue with human acknowledgement or emergency-service dispatch.
