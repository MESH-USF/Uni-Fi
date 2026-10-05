# Uni-Fi: a short guide to the radio mesh

**Uni-Fi** is MeshUSF’s emergency-first messaging prototype for Heltec V3
LoRa radios. Its purpose is to let nearby radios exchange short messages
directly, including a distress signal and a human acknowledgement, without
depending on a phone, Wi-Fi, cellular service, or a central server.

## The idea in one minute

Each Heltec is a small independent radio computer. It listens for nearby LoRa
packets, checks messages it can decrypt, and may relay eligible packets for
other nodes. A phone can connect over Bluetooth or USB to make configuration
and reading messages easier. The phone does not carry the mesh traffic.

```text
Person presses button
        ↓
Heltec builds a short Uni-Fi event
        ↓
MeshCore puts it in a radio packet and encrypts the group message
        ↓
LoRa carries it directly or through other Heltec relays
        ↓
Another Heltec decrypts it, updates its local state, and alerts its user
        ↓
That user may press the button to send a human acknowledgement back
```

The network has no automatic guarantee of delivery. The node can report that
it queued a message; a matching acknowledgement is evidence another node
received the event and someone chose to respond.

## MeshCore and Uni-Fi: who does what?

| MeshCore provides | Uni-Fi adds |
| --- | --- |
| LoRa packet format and radio scheduling | Emergency message types such as distress, medical, pickup, and acknowledgement |
| Encrypted group messages and direct messages | Rules for recognizing Uni-Fi events inside group messages |
| Node identities and public signed adverts | Button and LED behavior for distress and response |
| Packet forwarding, duplicate filtering, and routing | A locked product radio profile and a simpler companion interface |

Uni-Fi is a focused product built on MeshCore; it does not replace MeshCore’s
radio network. The current firmware keeps MeshCore’s established packet and
routing machinery while selecting a smaller Heltec V3 build for this project.

## How a message travels

1. A person presses the Heltec’s USER button, or a connected app asks the
   firmware to send an event.
2. Uni-Fi creates text describing the event. It includes a type and event ID
   so nodes can recognize distress and match a reply to the right event.
3. MeshCore wraps that text in a group-message packet, encrypts it with the
   shared Uni-Fi channel key, and schedules it for LoRa transmission.
4. Nearby compatible nodes receive the packet. Eligible nodes can relay it;
   relays suppress duplicates and the Uni-Fi build limits flood forwarding.
5. A recipient with the channel key verifies and decrypts the group message.
   Its Uni-Fi firmware updates its short-lived local presence/distress state,
   shows a preview, and signals a newly received distress with two long LED
   blinks.
6. A person can hold USER to send a new encrypted group message referring to
   the original event ID. The first node recognizes only a matching reply to
   its own outstanding event.

The two long blinks mean that a distress was received or that a matching
reply arrived. The display provides the context. Three short blinks indicate
that the local node accepted a distress for transmission; they do not prove
that it reached another radio.

## Does the node work without a phone?

Yes, when it is powered and within radio range of another compatible node,
directly or through relays. The firmware runs the receive, decrypt, display,
alert, and relay steps on the Heltec itself. It can send the current button
distress and a button acknowledgement with no app connected. It also sends a
periodic presence message.

The phone is a nearby companion. It can set the saved node name and time,
display messages and presence, and offer more sending controls. The current
firmware keeps a bounded queue of up to 64 received frames for a companion to
retrieve later. That queue and short-lived presence/distress records are held
in memory and do not survive a power restart. The screen is useful for alerts
and previews, but is not yet a persistent, searchable inbox.

## What does “presence” tell us?

The node periodically broadcasts an encrypted Uni-Fi presence message with
its saved name and node identifier. Other nodes can keep a local record and
refresh its expiry when they hear another message. This is not a GPS fix.
GPS is disabled in the current prototype, and the public MeshCore advert does
not include GPS coordinates. Therefore the current build cannot locate nodes
on a map or broadcast their current positions.

## What is implemented, and what comes later?

| In the current prototype | Later work |
| --- | --- |
| Heltec V3 firmware build for the selected 910.525 MHz radio profile | GPS module, location updates, and a verified map flow |
| Encrypted group channel and eligible multi-hop relaying | Persistent on-device message history |
| Button distress and button human acknowledgement | More physical quick-message choices and usability testing |
| LED alerts, OLED previews, periodic presence, and temporary local records | Retry policy, stronger sender identity binding, and key lifecycle tools |
| BLE/USB companion and lightweight browser test client | Flutter build and end-to-end mobile hardware validation |

“Implemented” describes code in this prototype. It does not mean the behavior
has passed physical radio, battery, button, LED, or multi-node testing. Those
checks require the actual hardware.

## A few useful terms

- **LoRa:** the low-power radio link between nearby devices. It is not Wi-Fi
  and it carries much less data than a phone internet connection.
- **Mesh:** nodes can forward packets for one another, allowing a message to
  travel beyond a single radio link when relays are present.
- **Group channel:** messages use a shared key for a group of radios. A node
  without that key cannot read the encrypted group text.
- **Advert:** a public signed announcement of a MeshCore node identity; it is
  different from an encrypted Uni-Fi presence message.
- **Acknowledgement:** in Uni-Fi, a person’s reply to a particular distress
  event. It is not the same as the radio’s internal packet handling.

## One important limitation

The group key is shared by provisioned devices. The current message’s name
and node fields help the application identify a claimed sender, but do not
prove which person created it. Uni-Fi also inherits the current MeshCore
group-encryption design. These limits, exact packet layouts, code paths, and
validation results are described in the [technical walkthrough](TECHNICAL_WALKTHROUGH.md).

Uni-Fi is a coordination aid, not a guaranteed emergency-service connection.
