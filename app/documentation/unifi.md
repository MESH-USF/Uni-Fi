# Uni-Fi product behavior

Uni-Fi is a MeshUSF product layer over the existing MeshCore companion and RF
protocols. The app treats a radio channel named `Uni-Fi` as the provisioned
network boundary. The firmware masks that channel's PSK, so the client selects
it by name/index and never needs the secret.

## Primary navigation

1. Emergency
2. Contacts
3. Channels
4. Map

Emergency supports fixed SOS, medical, pickup, safe, en-route, location, and
acknowledgement envelopes. Contacts and Map show a node only after an envelope
on the encrypted Uni-Fi channel correlates its six-byte `node` prefix with a
full MeshCore contact key. Presence and incidents expire four hours after the
latest local receipt time.

Periodic `presence` envelopes are persisted for roster calculation but hidden
from chat rows, notifications, and unread counts. Map incidents are red until
an acknowledgement for the same event ID arrives, then green.

The canonical protocol and threat model are in
`../../docs/EMERGENCY_PROTOCOL.md` and `../../docs/SECURITY.md`.
