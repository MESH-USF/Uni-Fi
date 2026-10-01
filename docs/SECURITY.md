# Security model

## Provisioned group membership

Each deployment receives a random 128-bit MeshCore group PSK. The same key is compiled into every authorized radio and installed as locked channel slot 0. The open repository includes only a template; production secrets must be supplied by the provisioning process.

Firmware masks slot 0 on `GET_CHANNEL`, rejects `SET_CHANNEL` for slot 0, and disables private identity import/export. The app sends by channel index and receives plaintext only after the radio has decrypted a valid packet.

## What this protects

- Radios without the group PSK cannot decrypt Uni-Fi username, GPS, presence, or emergency text.
- Unrelated public MeshCore advertisements do not authorize a user in the main Contacts/Map views.
- Public adverts from Uni-Fi firmware use an opaque key-derived label and omit GPS.
- Committing the open repository does not publish a deployment key.

## Limits of a shared key

A group PSK authenticates membership in the group, not an individual sender. Any member holding the PSK can create a message claiming another username or node prefix. The `node` field is correlation metadata, not a signature. Strong per-device identity requires signing the application envelope with the existing MeshCore device identity and verifying it against the full public key; that is deferred.

The key exists in device flash and can be recovered by an attacker with sufficient physical access. A connected phone receives decrypted content. The firmware masking controls normal companion API access; it is not tamper-resistant hardware.

## Removal and bans

Local hide/mute is not a network ban. Removing a device requires generating a new group key and reflashing every remaining authorized device. Before deployment, MeshUSF should define key custody, authorized provisioners, lost-device reporting, key rotation, and firmware signing/release procedures.

## Recommended provisioning controls

- Generate keys with a cryptographically secure random generator.
- Keep keys outside source control and build logs.
- Separate development, training, and production keys.
- Record which device was flashed into which deployment without storing its secret in the public inventory.
- Rotate after loss, theft, suspected firmware extraction, or membership removal.
- Do not reuse a key across independent organizations or events.
