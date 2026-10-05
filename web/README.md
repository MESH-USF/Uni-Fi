# Uni-Fi hardware test web app

This dependency-free browser client tests the actual Heltec V3 Uni-Fi companion protocol. It does not simulate a device or report a generic transmit response as remote delivery.

Run from this directory:

```sh
python3 -m http.server 8080 --bind 127.0.0.1
```

Open `http://localhost:8080` in Chrome or Edge on a supported desktop. For another computer or deployment, use HTTPS; Web Bluetooth and Web Serial require a secure context. Phone/browser Bluetooth support varies; Flutter remains the intended mobile client.

## Hardware connection

- Bluetooth uses the firmware's Nordic UART service. Pair using the PIN on the device. The firmware requires encrypted, authenticated Bluetooth pairing. Commands are complete GATT writes; if the platform cannot negotiate a sufficient MTU, the command fails rather than being split into invalid commands.
- USB is an explicit prototype test path. Select the Heltec serial port at 115200 baud. The firmware must include the USB companion interface. Close serial monitors and flash tools before connecting. Opening the port can reset some boards; retry after boot if initialization times out.
- Both paths start the application, query protocol version 3, synchronize the clock, verify slot 0's `Uni-Fi` metadata and the 910.525 MHz / 62.5 kHz / SF7 / CR5 profile, query Uni-Fi status, then drain the offline message queue. Status polling every five seconds keeps the USB session active and shows physical-button distress/receipt state. Legacy firmware that rejects the status command falls back to time polling every 15 seconds and explicitly reports that provisioning could not be independently verified. An explicitly unprovisioned device cannot send. The network secret is masked by firmware and never shown or edited in this UI.

The client provides names saved/read back from the device, SOS/medical/pickup/safe/enroute messages, plain text, explicit receipt responses, received-event diagnostics and a RAM roster with four-hour local expiry. A network key proves group access, not a unique person's identity; node IDs and names remain self-reported. The roster is reconstructed from channel messages available to this browser session, not a complete readout of the firmware's retained roster.

“Queued on device” means firmware accepted the message into its transmit queue. A matching `mc:v1` ACK received from a different node is labeled “Remote receipt.” Neither state promises emergency assistance. The ACK uses the original event ID; it is a separate encrypted group message. Physical-button SOS and pending received distress are read using the Uni-Fi status command (`0x70`, version 1, 101-byte response); physical distress acknowledged by firmware appears as “Remote receipt confirmed by device.” GPS remains disabled in this build; optional coordinates in valid envelopes are parsed for later Flutter/map support.

The browser retains up to 128 presence records, 150 incoming messages and 100 incidents in memory, clearing on reload or a fresh connection. Presence heartbeats stay out of chat; duplicate structured events coalesce by node and event ID. Incidents expire four hours after their latest local receipt or matching ACK. Message transmission is limited to the radio's 160 UTF-8 bytes including the device-name prefix and a conservative 172-byte companion frame. Event labels are compact to avoid firmware truncating their metadata. Rename validation prevents colons, brackets or control characters from breaking sender parsing.

## Verification

```sh
npm test
```

Tests run using Node's built-in test runner (Node 20+) with no package installation. They cover actual wire offsets, USB fragmentation/noise, UTF-8 budgets, strict envelopes, four-hour expiry, ACK correlation, serialized commands, errors, timeout isolation and offline/push queue draining. Test transports are confined to tests; the web UI has no mock connection mode.

For an end-to-end hardware test, provision two nodes with the same network key, connect one browser to each, send SOS from A, receive it on B, and select **Respond: received** on B. A must show **Remote receipt** only after receiving B's matching ACK. Repeat with medical/pickup, disconnect/reconnect to drain queued incoming messages, verify name survives restart, and test the physical button separately with the device display/LED indicators. Hardware testing is required to verify radio range, Bluetooth pairing/MTU and physical-device behavior.
