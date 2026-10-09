# Architecture decision records

These records preserve decisions that shape Uni-Fi across both the radio and
the client. A decision is not silently rewritten after implementation. If it
changes, add a new record that supersedes the old one and link the two.

| ID | Decision | Status |
| --- | --- | --- |
| [0001](0001-radio-owns-emergency-path.md) | Radio owns the emergency path | Accepted |
| [0002](0002-provisioned-group-channel.md) | Provisioned group channel and key handling | Accepted |
| [0003](0003-fixed-radio-profile.md) | Fixed 910.525 MHz deployment profile | Accepted, regulatory review required |
| [0004](0004-local-four-hour-presence.md) | Local four-hour presence and incident state | Accepted |
| [0005](0005-alert-patterns-and-led-sharing.md) | Alert patterns and LED sharing | Accepted |
| [0006](0006-prune-at-product-boundaries.md) | Prune at product boundaries while retaining protocol compatibility | Accepted |
| [0007](0007-one-button-prototype.md) | One-button, GPS-free Heltec prototype and bounded fleet routing | Accepted; updates 0005 and 0006 |
| [0008](0008-stock-app-simulated-advert.md) | Original MeshCore app and triple-press public simulated location | Current prototype; older private target retained |
| [0009](0009-page-scoped-public-preset.md) | Home-page Public preset and separate location-advert page | Current button behavior; updates 0008 |
