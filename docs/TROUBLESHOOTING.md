# Troubleshooting

## Build Errors

### `idf.py` is not recognized

- Open an ESP-IDF shell or run your ESP-IDF `export.ps1` script first.
- Verify that `idf.py --version` works before building.

### `protocol_examples_common` component not found

- The root `CMakeLists.txt` depends on the ESP-IDF examples common components path.
- Check that `IDF_PATH` points to a valid ESP-IDF installation.

### Partition or size errors during build

- Confirm the project is using `partition_table/partitions.csv`.
- Confirm the flash size is still configured for 4 MB.
- If the application grows too large, review logs from `idf.py build` and reduce image size.

## Flash Errors

### Device will not enter flashing mode

- Check the selected COM port.
- Hold `BOOT`, tap `EN` or `RST`, then release `BOOT` when flashing starts if your board needs manual boot mode.
- Try a different USB cable or USB port if the serial device is unstable.

### Flash succeeds but device will not boot

- Verify that the correct target is selected for ESP32.
- Check the serial monitor immediately after reset for bootloader and app logs.
- Make sure the flashed image matches the partition layout expected by the project.

## Monitor and Logging Issues

### No serial output

- Confirm the correct serial port.
- Reset the board after starting the monitor.
- Close other tools that may already be connected to the same port.

### Logs are unreadable

- Check the monitor baud rate and board reset behavior.
- Re-run with `idf.py -p COM3 monitor` from a known-good ESP-IDF shell.

## Wi-Fi Connection Problems

- Re-check the SSID and password in local `sdkconfig`.
- Confirm the access point uses a mode supported by the ESP32 and is in range.
- Make sure the board and OTA host machine are on a network where peer communication is allowed.

## HTTPS Certificate Problems

### TLS handshake fails

- If using a public HTTPS server, make sure the server certificate chain is valid and the ESP certificate bundle is enabled.
- If using a self-signed or private server, make sure `certs/ota_server_cert.pem` matches the current server certificate and was present before the last firmware build.
- Rebuild and reflash after changing the certificate file.

### Local OTA server URL looks correct but HTTPS still fails

- Check that the certificate subject alternative name matches the IP or hostname used in the OTA URL.
- Do not use `http://`; the OTA manager rejects it immediately.

## OTA Download Failure

### HTTP status is not `200`

- Confirm the URL points directly to `esp32-secure-ota.bin`.
- Confirm the server is serving the file from the `build/` directory.

### Invalid content length or incomplete download

- Make sure the server provides a normal file response with `Content-Length`.
- Check network stability and keep the server running for the full transfer.

### Firmware is larger than the target partition

- The current OTA slot size is `0x1F0000`.
- Review the build output and reduce image size if needed.

## Partition and Boot Issues

### OTA completes but the new app does not stick

- The device must reach the post-boot Wi-Fi checkpoint so the app can call `esp_ota_mark_app_valid_cancel_rollback()`.
- If the new firmware crashes or never connects to Wi-Fi, rollback may return the device to the previous image.

### Wrong partition layout

- Do not change the partition CSV casually.
- If you do change it, rebuild, reflash, and revalidate the OTA slot sizes before testing updates.

## Firmware Does Not Update

- Check whether `Run OTA check automatically on boot` is enabled.
- Confirm the incoming firmware version differs from the running version.
- Confirm the OTA URL points to the latest binary, not an old cached file.

## Device Reboot Loop

- Capture logs from power-on through Wi-Fi initialization.
- Look for repeated rollback or crash behavior.
- Test with auto OTA disabled first to confirm the base firmware is stable.

## Collecting Useful Logs

For debugging, capture:

- Full `idf.py build` output if the problem is compile-time
- Full boot log from `idf.py monitor`
- The configured OTA URL, with secrets removed if needed
- Whether the update server uses a public CA or a local self-signed certificate
- Whether the running and incoming firmware versions are different
