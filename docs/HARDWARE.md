# Heltec V3 hardware and flashing

## Baseline hardware

- Heltec WiFi LoRa 32 V3 / LoRa 32 V3
- Whip antenna matched to the board and deployment band
- GPS is not required or compiled into the current prototype
- Single-cell LiPo connected through the Heltec battery connector

Never transmit without the LoRa antenna attached.

## Buttons and future GPS

The board's two switches are RESET and USER/BOOT (GPIO 0), not two programmable
buttons. RESET restarts the processor. During normal operation USER controls
Uni-Fi; holding USER while resetting can enter the chip bootloader.

GPS wiring and its driver target are TODO. Do not use the previous suggested
GPIO 26 enable connection: Heltec's current GPIO guidance reserves that pin.
Recheck the exact board revision, module voltage and free UART pins before
adding a location provider. Optional `lat`/`lon` protocol fields and Flutter's
map code remain available; the prototype sends no coordinates.

## Uni-Fi target

`Heltec_v3_unifi_companion_ble_us` sets:

- 910.525 MHz, BW 62.5 kHz, SF7, CR 4/5
- Locked waveform parameters even when older preferences remain in flash
- Adjustable TX power up to the board target limit
- BLE with a random session PIN displayed on OLED and a USB companion test path
- GPS absent; no sensor or external-clock probing
- No GPS in public MeshCore advertisements
- 10-second OLED timeout with active-low Vext power control and 3.4 V low-battery shutdown
- 128 contacts, 8 channels, and a 64-frame offline queue
- Private-key import/export disabled
- Unused environmental sensor drivers disabled
- Bounded fleet forwarding (three flood hops) with MeshCore duplicate suppression
- Firmware stored names survive reboot; roster and emergency state are RAM-only
- Image/raw datagrams, remote management, signing and rescue filesystem CLI disabled

## Network provisioning

From the repository root, generate the ignored deployment header:

```bash
python3 scripts/provision-unifi.py
```

The script creates a private, randomly generated 128-bit key without printing
it and refuses to overwrite an existing key. The ignored header is compiled
into the radio. Firmware installs it as channel slot 0 named `Uni-Fi`, refuses
companion writes to that slot, and masks the PSK in channel-read responses.
Builds without the header compile but start unprovisioned; they cannot send
Uni-Fi events or act as fleet relays. Flash the same provisioned build onto
every device in this test deployment. Keep the header and binaries private.

Use a different random key for each deployment. Store the provisioning secret in an access-controlled secret manager and define a key-revocation/reflash procedure before field use.

## LED contract

GPIO 35 is exclusively owned by Uni-Fi alert feedback in this target. The
ordinary TX LED writer is not compiled, so radio traffic cannot corrupt the
patterns:

- distress accepted by radio: three short pulses, 160 ms on / 180 ms off
- new distress or matching response received: two long pulses, 650 ms on / 300 ms off

Incoming patterns have priority. An unrelated ACK does not trigger the received
pattern. OLED distinguishes `SOS queued`, `SOS awaiting ACK`, `SOS acknowledged`
and failure. Queued is local acceptance, not proof of transmission or delivery.
Distinct colors require an external RGB LED in a later board revision.

## Build and upload

```bash
cd firmware
pio run -e Heltec_v3_unifi_companion_ble_us -t clean
pio run -e Heltec_v3_unifi_companion_ble_us
pio run -e Heltec_v3_unifi_companion_ble_us -t mergebin
pio run -e Heltec_v3_unifi_companion_ble_us -t upload
```

PlatformIO uploads each image at the correct offset. For a full-flash tool,
use `.pio/build/Heltec_v3_unifi_companion_ble_us/firmware-merged.bin` at offset
`0x0`; `firmware.bin` alone is the application image, not a full flash image.
Do not commit a provisioned binary: it contains this deployment's network key.
Clean the build after adding or changing a provisioning header so cached
objects from an unprovisioned or different-key build cannot be reused.

Erase flash when validating first-boot behavior. Before RF testing, verify the antenna, requested profile, output power, certification, and local rules for the actual deployment.

## Phone-independent operation

Single-click wakes the display or advances a page. Double-click USER queues an
encrypted SOS containing the username and node prefix. Hold USER for two
seconds to acknowledge the most recent pending distress; with none pending,
hold selects the current screen action. Presence, reception, expiry, fleet
forwarding and the emergency queue continue without a phone. USB connection
state expires after 30 seconds without client commands; the web client polls
every five seconds. BLE remains available when the OLED sleeps.

See [prototype validation](PROTOTYPE_VALIDATION.md) for the two-/three-device
acceptance procedure and what automated tests can establish.
