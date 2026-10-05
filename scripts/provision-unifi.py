#!/usr/bin/env python3
"""Create an ignored, private deployment key header without printing the key."""

import base64
import os
from pathlib import Path
import secrets

header = Path(__file__).resolve().parents[1] / "firmware/examples/companion_radio/UniFiProvisioning.h"
key = base64.b64encode(secrets.token_bytes(16)).decode("ascii")
try:
    fd = os.open(header, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
except FileExistsError:
    raise SystemExit("Provisioning header already exists; retained the existing deployment key.")
with os.fdopen(fd, "w") as output:
    output.write('#pragma once\n#define UNIFI_NETWORK_PSK_B64 "' + key + '"\n')
print("Created private provisioning header. Flash the same build to devices in this deployment.")
