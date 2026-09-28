# ADR 0006: Prune at product boundaries while retaining protocol compatibility

- Status: Accepted
- Date: 2026-09-28

## Context

The upstream firmware and client support many boards, sensor modes, repeater
roles, diagnostics, and expert settings. Deleting shared protocol code too
early would make the fork fragile and difficult to audit against upstream.

## Decision

Create one explicit Uni-Fi firmware target and a focused client flow. Remove
features from the product surface and product dependency set first; retain the
shared MeshCore protocol implementation required for interoperability and
upstream comparison. Keep firmware and app in separate source subtrees with
their upstream license and history documents.

The retained product surface is emergency quick messages, encrypted presence,
authorized map/contacts, direct messaging, QR import, simple group channels,
GPS, battery/screen controls, and basic naming/connectivity. Expert radio
tuning, public discovery as authorization, sensor extras, repeater features,
and unrelated management flows are outside the Uni-Fi product target. BLE is
the end-user onboarding path; inherited USB/TCP transports remain available to
developers but are not presented in the scanner.

## Consequences

- Builds remain recognizable as MeshCore implementations while the user
  experience is deliberately smaller.
- Unused upstream source may remain in the tree until dependency and protocol
  tests prove it can be deleted safely.
- Every future deletion should include a size measurement and a compatibility
  check; file count alone is not the optimization target.
