# ADR 0003: Fixed 910.525 MHz deployment profile

- Status: Accepted, regulatory review required
- Date: 2026-09-28

## Context

Non-expert users should not need to understand or synchronize radio settings.
The requested United States/Canada operating profile is 910.525 MHz with a
narrow MeshCore waveform.

## Decision

The Uni-Fi Heltec V3 build fixes frequency at 910.525 MHz, bandwidth at 62.5
kHz, spreading factor at 7, and coding rate at 4/5. It restores these values at
boot and rejects companion attempts to alter the waveform. The build also
disables repeat mode by default.

## Consequences

- All deployed nodes start interoperable without user radio configuration.
- The app does not need to expose radio tuning for the product build.
- A separate target and review are required for other regions or profiles.
- This software choice is not regulatory approval. MeshUSF must verify local
  rules, equipment authorization, power, antenna, and duty-cycle obligations
  before transmitting in each jurisdiction.
