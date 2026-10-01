# Heltec V3 hardware and flashing

## Baseline hardware

- Heltec WiFi LoRa 32 V3 / LoRa 32 V3
- Whip antenna matched to the board and deployment band
- UART GPS outputting standard NMEA at 9600 baud
- Single-cell LiPo connected through the Heltec battery connector

Never transmit without the LoRa antenna attached.

## GPS wiring

| GPS module | Heltec V3 |
| --- | --- |
| TX | GPIO 48 (Heltec receives GPS data) |
| RX | GPIO 47 (optional unless configuring GPS) |
| EN/PPS or enable, when supported | GPIO 26 |
| GND | GND |
| VCC | Supply required by the selected GPS module |

Heltec GPIO uses 3.3 V logic. Verify the GPS module supply and logic levels before wiring. A module without enable can leave GPIO 26 disconnected.

## Uni-Fi target

`Heltec_v3_unifi_companion_ble_us` sets:

- 910.525 MHz, BW 62.5 kHz, SF7, CR 4/5
- Locked waveform parameters even when older preferences remain in flash
- Adjustable TX power up to the board target limit
- BLE with a random session PIN displayed on OLED
- GPS enabled with a 60-second update interval
- No GPS in public MeshCore advertisements
- 10-second OLED timeout and 3.4 V low-battery shutdown
- 128 contacts, 8 channels, and a 64-frame offline queue
- Private-key import/export disabled
- Unused environmental sensor drivers disabled

## Network provisioning

The real key is a build input, never a source file:

```bash
cp examples/companion_radio/UniFiProvisioning.example.h \
   examples/companion_radio/UniFiProvisioning.h
openssl rand -base64 16
```

Insert the generated value into `UNIFI_NETWORK_PSK_B64`. The ignored header is compiled into the radio. Firmware installs it as channel slot 0 named `Uni-Fi`, refuses companion writes to that slot, and masks the PSK in channel-read responses. A build without a valid provisioning header starts unprovisioned and cannot send Uni-Fi emergency/presence packets.

Use a different random key for each deployment. Store the provisioning secret in an access-controlled secret manager and define a key-revocation/reflash procedure before field use.

## LED contract

The Heltec V3 exposes one onboard single-color LED on GPIO 35, which MeshCore also uses for LoRa TX indication. Uni-Fi arbitrates it by delaying product patterns until the transmission window:

- alert accepted by radio: three short pulses, 160 ms on / 180 ms off, after a 1.2 s delay
- alert or response received: two long pulses, 650 ms on / 300 ms off, after a 700 ms delay

Incoming patterns have priority over a pending sent pattern. OLED text distinguishes `SOS sent`, `SOS failed`, and the received message. Distinct colors require an external RGB LED in a later board revision.

## Build and upload

```bash
pio run -e Heltec_v3_unifi_companion_ble_us
pio run -e Heltec_v3_unifi_companion_ble_us -t upload
```

Erase flash when validating first-boot behavior. Before RF testing, verify the antenna, requested profile, output power, certification, and local rules for the actual deployment.

## Phone-independent operation

Double-click the onboard button. The radio creates an encrypted SOS with username, six-byte node prefix, and GPS when valid; it floods through channel slot 0 and reports success/failure on OLED. Presence and the receive queue continue while no phone is attached.
