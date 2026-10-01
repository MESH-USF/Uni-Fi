# ADR 0004: Local four-hour presence and incident state

- Status: Accepted
- Date: 2026-09-28

## Context

Uni-Fi needs a useful nearby-user map without cloud accounts or a permanent
central roster. RF messages may be duplicated by flooding, and sender clocks
may be unset or incorrect.

## Decision

Each radio floods an encrypted presence envelope every 15 minutes. Firmware
stores at most 32 authorized node prefixes in RAM and expires them four hours
after local receipt. A duplicate for the same node refreshes the local receipt
time instead of creating another entry.

The app independently stores a `receivedAt` value and applies the same
four-hour window to presence, distress markers, and acknowledgements. It uses
the encrypted six-byte node prefix to correlate with a full MeshCore public
key. Display name alone never authorizes a contact.

## Consequences

- Presence is decentralized, ephemeral, and restored through RF traffic after
  a radio reboot.
- At a 15-minute interval, an active node has up to 16 refresh opportunities in
  one retention window.
- Delayed duplicated RF traffic can extend visibility from the time it is
  locally received; sender timestamps do not control authorization.
- The map hides public advertisements that lack recent authenticated presence.
