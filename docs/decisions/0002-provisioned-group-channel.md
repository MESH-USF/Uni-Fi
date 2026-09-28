# ADR 0002: Provisioned group channel and key handling

- Status: Accepted
- Date: 2026-09-28

## Context

Usernames, GPS coordinates, and distress state should be readable only by
authorized Uni-Fi devices. The solution must remain compatible with MeshCore
group traffic and must not put a deployment secret in the public repository.

## Decision

Reserve channel slot 0 as an encrypted channel named `Uni-Fi`. Provision every
radio in one deployment with the same random 128-bit group PSK at build time.
The key is supplied through the ignored `UniFiProvisioning.h`, never through a
checked-in default.

Locked firmware masks slot 0 on companion reads, rejects companion replacement
of slot 0, and disables device-private-key import and export. The app selects
the provisioned channel by its exact name and never falls back to a public
channel for emergency traffic.

## Consequences

- A valid group packet proves possession of the deployment PSK, not the
  identity claimed in its plaintext. Any member can impersonate another name.
- Removing one device requires group-key rotation and reflashing the remaining
  devices.
- A determined attacker with physical access may extract secrets from flash.
- Per-device signed envelopes are a future hardening step, not an implied
  property of this release.
