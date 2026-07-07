# Setup

## Assumptions

- Target board: ESP32 development board with at least 4 MB flash.
- SDK: ESP-IDF v5.3.3 was used for the latest verified build in this workspace.
- Host tools: Python 3 and the standard ESP-IDF command-line tools.
- Network: A Wi-Fi network that both the ESP32 and the HTTPS firmware host can access.
- Firmware hosting: A reachable HTTPS endpoint serving the built `.bin` file.

## Hardware Requirements

- ESP32 development board
- USB cable for flashing and serial monitoring
- Host computer with ESP-IDF installed
- Wi-Fi network
- HTTPS firmware hosting endpoint, either public or local

## Environment Setup

Open an ESP-IDF shell or export the environment from your ESP-IDF installation:

```powershell
. C:\path\to\esp-idf\export.ps1
```

Then move into the project root:

```powershell
cd "C:\path\to\Secure OTA Firmware Update System"
```

If your environment is already managed by the ESP-IDF VS Code extension or an ESP-IDF terminal, the explicit export step may not be needed.

## Initial Configuration

Open the project configuration menu:

```bash
idf.py menuconfig
```

Configure these settings:

- `Example Connection Configuration -> WiFi SSID`
- `Example Connection Configuration -> WiFi Password`
- `Secure OTA Example -> Firmware upgrade HTTPS URL`
- `Secure OTA Example -> Run OTA check automatically on boot`
- `Secure OTA Example -> Use ESP x509 certificate bundle for OTA HTTPS`

Notes:

- `sdkconfig.defaults` ships with placeholders, not real credentials.
- Real Wi-Fi credentials and internal OTA URLs should remain in the ignored local `sdkconfig`.
- Auto OTA on boot is disabled by default and should only be enabled when a trusted update endpoint is ready.

## Build

```bash
idf.py build
```

Expected OTA payload:

```text
build/esp32-secure-ota.bin
```

## Flash

```bash
idf.py -p COM3 flash
```

If you want to flash and open the serial monitor in one step:

```bash
idf.py -p COM3 flash monitor
```

## Serial Monitor

```bash
idf.py -p COM3 monitor
```

## Local HTTPS OTA Test Server

The repository includes a simple Python HTTPS file server for manual OTA testing:

```bash
python scripts/ota_server.py --cert certs/ota_server_cert.pem --key certs/ota_server.key
```

This server:

- Serves files from `build/`
- Exposes `build/esp32-secure-ota.bin`
- Prints a URL template such as `https://YOUR_IP:8070/esp32-secure-ota.bin`

## Local Certificate Workflow

For a private or self-signed local OTA server, generate a certificate whose subject alternative name matches the server IP or hostname:

```powershell
New-Item -ItemType Directory -Force certs | Out-Null
openssl req -x509 -newkey rsa:2048 -sha256 -days 365 -nodes `
  -keyout certs\ota_server.key `
  -out certs\ota_server_cert.pem `
  -subj "/CN=YOUR_IP" `
  -addext "subjectAltName=IP:YOUR_IP,DNS:localhost"
```

Important behavior:

- If `certs/ota_server_cert.pem` exists at build time, the OTA component embeds that certificate into firmware.
- If that file is absent, the project falls back to the ESP-IDF certificate bundle when enabled.
- If you change the local certificate, rebuild and reflash the device before testing again.
- If you serve firmware by hostname instead of IP, update the certificate subject and SAN entries to match that hostname.

## Customizing Firmware URL, Credentials, Certificates, and Version Metadata

### Wi-Fi Credentials

Set them in `idf.py menuconfig`. Do not store real values in `sdkconfig.defaults`.

### Firmware URL

Set `Secure OTA Example -> Firmware upgrade HTTPS URL` to the final HTTPS path for the `.bin` file.

### Certificate Trust

Choose one of these trust paths:

- Public HTTPS endpoint backed by a CA trusted by the ESP-IDF certificate bundle
- Local or private HTTPS server with `certs/ota_server_cert.pem` embedded into firmware at build time

### Version Metadata

The OTA logic compares the running and incoming firmware version strings. To test updates cleanly, build a new version:

```bash
idf.py -D PROJECT_VER=1.0.1 build
```

## Firewall and Connectivity Notes

- The ESP32 must be able to reach the OTA server on the selected HTTPS port.
- On Windows, you may need to allow inbound traffic for the chosen port.
- The ESP32 and host machine usually need to be on the same LAN for local OTA testing.
