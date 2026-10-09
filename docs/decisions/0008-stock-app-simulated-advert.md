# 0008 — Stock-app prototype with simulated public location

Date: 2026-10-08. Status: accepted for the current prototype.

The user now wants original MeshCore app compatibility, no private encryption
module, three USER presses to broadcast name/location without GPS hardware,
Uni-Fi UI branding, and clean YAGNI code.

Use a separate trimmed `Heltec_v3_unifi_stock_ble_us` target. Reuse the existing
triple-click detector and standard signed public advert encoder. Coordinates
are a fixed test point; keep the saved name unchanged and label simulation on
the OLED. There is no standard fake-location flag, so receiving apps may show
the test point as real. This build is not for emergency deployment.

Exclude private provisioning, custom encrypted presence and SOS/ACK behavior
from this target. Retain standard MeshCore cryptography and companion text
operations, since removing those primitives breaks original-app messaging.
Expose read-only tuning and empty custom-settings responses for stock app
settings queries. Keep advanced remote/raw/sensor features pruned and retain
the requested locked radio profile. Disable companion relaying by default;
dedicated repeaters relay floods. No new custom mobile protocol is necessary.

The earlier private-network target remains reproducible rather than deleting
upstream/shared modules. Decisions 0002, 0004, 0005 and the SOS portions of
0007 apply only to that older target for now. Physical stock-app compatibility
is an acceptance test, not inferred from a successful firmware build.
