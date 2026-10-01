# ADR 0001: Radio owns the emergency path

- Status: Accepted
- Date: 2026-09-28

## Context

Uni-Fi must remain useful when a phone is absent, discharged, disconnected, or
too cumbersome to operate. The Heltec V3 has enough capability to own RF,
identity, GPS, a button, a small display, and simple light feedback, while the
phone is much better at maps and message composition.

## Decision

The radio owns all functions required to send and recognize an emergency:
MeshCore identity and routing, the provisioned group key, the fixed RF profile,
GPS sampling, encrypted presence, a four-hour RAM roster, emergency-button
handling, and LED/OLED feedback. The phone owns richer history, maps, contact
management, QR import, direct chat, and composed group messages.

The initial independent gesture is a double-click that sends an SOS. Losing the
phone must not change network identity or prevent this action.

## Consequences

- Emergency traffic does not depend on BLE or app lifecycle state.
- The radio requires enough local state to identify recent authorized traffic.
- The phone is an enhanced console, not the source of network membership.
- Hardware-button behavior must be tested on physical units before field use.
