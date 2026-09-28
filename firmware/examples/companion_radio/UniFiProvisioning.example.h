#pragma once

// Copy this file to UniFiProvisioning.h and replace the value with a unique,
// randomly generated 16-byte key encoded as Base64. The real provisioning
// header is ignored by git and must never be committed.
//
// Example key generation:
//   openssl rand -base64 16
//
// Each device in one Uni-Fi deployment must be flashed with the same key.
#define UNIFI_NETWORK_PSK_B64 "REPLACE_WITH_BASE64_PSK"
