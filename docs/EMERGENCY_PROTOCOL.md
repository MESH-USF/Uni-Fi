# Uni-Fi text envelope

Uni-Fi state is carried in normal MeshCore group text on the encrypted `Uni-Fi` channel. The readable prefix provides a useful fallback while a compact final block provides structured state.

```text
SOS - immediate assistance needed [mc:v1;type=sos;id=AABBCCDDEEFF12345678;node=AABBCCDDEEFF;lat=28.063000;lon=-82.413900]
```

Acknowledgement:

```text
Received - help is responding [mc:v1;type=ack;id=11223344556612345678;node=112233445566;ack=AABBCCDDEEFF12345678]
```

Periodic presence:

```text
Available [mc:v1;type=presence;id=AABBCCDDEEFF12345678;node=AABBCCDDEEFF;lat=28.063000;lon=-82.413900]
```

## Fields

The prototype uses compact readable labels such as `SOS` and `Received`.
An ACK indicates receipt only. Envelope parsing validates the entire final
block and rejects missing/duplicate/unknown fields, invalid node prefixes and
oversized IDs. IDs are 1–32 ASCII letters/digits/underscores/hyphens. The
160-byte UTF-8 radio limit includes the sender name and `: ` prefix; outgoing
structured text is rejected rather than truncated. Coordinates are optional
and not transmitted by the GPS-free build.

| Field | Meaning |
| --- | --- |
| `type` | `presence`, `sos`, `medical`, `pickup`, `safe`, `enroute`, `location`, or `ack` |
| `id` | Sender-generated event identifier |
| `node` | First six public-key bytes as 12 hexadecimal characters |
| `lat`, `lon` | Optional decimal GPS; both must be present and valid |
| `ack` | Original event ID; required for `type=ack` |

MeshCore prepends the encrypted sender name as `sender: message`. Uni-Fi uses the `node` prefix to correlate that decrypted name with a separately received MeshCore contact advertisement. Matching by display name alone is never used.

## Retention and duplicates

- Firmware records local receive time in RAM and removes presence after four hours.
- A later valid envelope with the same node prefix replaces the entry and refreshes local receive time.
- The app persists `receivedAt` separately from the sender timestamp. Repeated packet copies update `receivedAt` without creating another visible chat row.
- Presence packets are hidden from chat, notifications, and unread counts.
- Incidents and their map state expire four hours after the latest locally received packet.
- Encrypted presence with a valid location can appear on the map before a
  matching full public-key advert arrives. It is display-only until that advert
  makes direct contact import possible.

An acknowledgement proves only that another holder of the network key sent a response. It does not prove that emergency services were contacted or that help will arrive.

## Product status (BLE/USB, not over the radio)

The prototype reserves companion command `0x70` (one-byte request) and
response `0x70` for read-only local Uni-Fi status. Version 1 is 101 bytes:

| Offset | Length | Content |
| --- | --- | --- |
| 0 | 1 | `0x70` |
| 1 | 1 | version `1` |
| 2 | 1 | flags |
| 3 | 1 | network channel index `0` |
| 4 | 32 | current local distress ID, zero-padded |
| 36 | 32 | latest pending received distress ID, zero-padded |
| 68 | 32 | pending sender name, zero-padded |
| 100 | 1 | retained firmware member count |

Flag bits: 0 provisioned, 1 local distress awaiting ACK, 2 local distress
acknowledged, 3 received distress pending, 4 GPS compiled, 5 fleet relay.
The key is not included. The web client polls this every five seconds to
observe standalone button activity; an older client can ignore the extension.
Firmware retains one current local distress and one newest pending received
distress. Matching uses the event ID and excludes the local node as responder;
unrelated ACKs and duplicate events do not create receipt feedback.
