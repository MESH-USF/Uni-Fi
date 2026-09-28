# ADR 0005: Alert patterns and LED sharing

- Status: Accepted
- Date: 2026-09-28

## Context

The first hardware revision uses the Heltec V3's available onboard light rather
than assuming extra indicators. The same light may already reflect LoRa
transmit activity, so emergency patterns must be distinct and scheduled after
normal transmission.

## Decision

- SOS accepted by the local radio: three short pulses, 160 ms on and 180 ms
  off, beginning after a 1.2-second delay.
- Emergency or acknowledgement received: two long pulses, 650 ms on and 300 ms
  off, beginning after a 700-ms delay.
- Incoming feedback has priority if patterns overlap.

The OLED also reports whether the independent SOS was accepted or failed. The
current implementation uses GPIO 35 through the board's existing onboard-light
path and time-shares it with LoRa TX indication.

## Consequences

- The patterns are count- and duration-distinct from ordinary single TX
  activity.
- “Sent” means queued by the local radio; the two-long-pulse acknowledgement is
  the stronger indication that another member responded.
- GPIO polarity and visibility must be verified on every production hardware
  revision. A later multi-LED board may alternate indicators without changing
  protocol semantics.
