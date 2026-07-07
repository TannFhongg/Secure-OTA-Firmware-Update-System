# OTA Workflow

## Overview

The OTA path in this repository is a direct firmware-download flow. The device reads a single HTTPS URL from configuration, downloads the binary from that endpoint, validates the image, writes it to the inactive OTA slot, and reboots into the new partition.

## Step-by-Step Flow

1. Build the firmware image with `idf.py build`.
2. Configure the device with Wi-Fi credentials and an HTTPS firmware URL.
3. Host `build/esp32-secure-ota.bin` on a reachable HTTPS server.
4. Boot the device and connect it to Wi-Fi.
5. If auto OTA is enabled, `app_main()` calls `ota_update()`.
6. The OTA manager checks transport, response metadata, partition size, and image metadata.
7. ESP-IDF downloads the image into the inactive OTA slot.
8. The device reboots into the updated partition.
9. On the next boot, the new image is marked valid after the Wi-Fi checkpoint.

## How Firmware Binaries Are Built

The application name defined in the root `CMakeLists.txt` is `esp32-secure-ota`, so the OTA payload produced by ESP-IDF is:

```text
build/esp32-secure-ota.bin
```

Build command:

```bash
idf.py build
```

To test an actual update, the new binary needs a different application version from the currently running firmware. This repository compares the version string embedded in `esp_app_desc_t`, so a same-version image is skipped.

One practical way to rebuild with a different version is:

```bash
idf.py -D PROJECT_VER=1.0.1 build
```

## Expected Firmware URL and Server Behavior

The firmware source is configured through:

```text
Secure OTA Example -> Firmware upgrade HTTPS URL
```

Expected behavior based on the current code:

- The URL must start with `https://`.
- The server must return HTTP status `200`.
- The server should return a valid `Content-Length`.
- The payload should be a valid ESP-IDF application binary.
- The image must fit inside a single OTA slot.

For local testing, `scripts/ota_server.py` serves files from the `build/` directory over HTTPS and expects the firmware file name `esp32-secure-ota.bin`.

## Version Checking Behavior

Version handling is intentionally simple in the current implementation:

- The OTA manager reads the incoming `esp_app_desc_t`.
- It compares the incoming version string with the running firmware version.
- If the strings are identical, the update is skipped.

This means the code currently prevents redundant reflashing, but it does not enforce newer-versus-older ordering and does not use a separate version manifest.

## Download and Apply Behavior

Internally, the OTA manager performs these key steps:

1. Select the next OTA partition with `esp_ota_get_next_update_partition()`.
2. Start the transfer with `esp_https_ota_begin()`.
3. Check the HTTP status code with `esp_https_ota_get_status_code()`.
4. Check image size with `esp_https_ota_get_image_size()`.
5. Read the image descriptor with `esp_https_ota_get_img_desc()`.
6. Download and write the image with repeated calls to `esp_https_ota_perform()`.
7. Verify download completeness with `esp_https_ota_is_complete_data_received()`.
8. Finalize the update with `esp_https_ota_finish()`.

If the update succeeds, the firmware waits briefly and then calls `esp_restart()`.

## Reboot and Rollback Behavior

After a successful OTA:

- The bootloader is configured to boot the new OTA slot.
- The device restarts.
- The new image starts in a pending-verification state when rollback is enabled.
- This application marks the running image valid after Wi-Fi comes up successfully.

If the new image cannot boot far enough to reach that checkpoint, ESP-IDF rollback support can return the device to the previous image.

## What to Verify After an Update

- The serial log shows `OTA Update SUCCESS`.
- After reboot, the running partition label changes to the updated slot.
- The reported firmware version matches the new build.
- The device reconnects to Wi-Fi successfully.
- The image is marked valid instead of staying in pending verification.

## Common OTA Failure Cases

| Failure case | What to check |
| --- | --- |
| Wi-Fi connection failure | SSID/password, access point reachability, and that the ESP32 and server are on the same network |
| HTTPS certificate issue | Whether the server certificate is trusted by the ESP certificate bundle or matches the embedded local certificate |
| Invalid firmware URL | That the URL starts with `https://` and points to the actual `.bin` file |
| Wrong partition table | That the project still uses `partition_table/partitions.csv` and 4 MB flash |
| Insufficient flash | Whether the built firmware exceeds the `0x1F0000` OTA slot size |
| Interrupted download | Connectivity drops, server restarts, or partial transfers before `esp_https_ota_is_complete_data_received()` succeeds |
| Firmware validation failure | Invalid app descriptor, corrupted binary, or `esp_https_ota_finish()` reporting an error |
| Same-version image | The incoming firmware has the same `esp_app_desc_t.version` as the running app |
