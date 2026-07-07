# Security Notes

## Security Goals

This project is designed to explore a safer OTA update path for ESP32 by:

- Preventing plain HTTP firmware delivery.
- Verifying the server certificate before accepting firmware.
- Rejecting obviously invalid or oversized firmware responses early.
- Reducing accidental reflashing of the same build.
- Using rollback-aware boot behavior to reduce update risk after reboot.

## Threat Model

| Threat | Risk | Current mitigation | Remaining gap |
| --- | --- | --- | --- |
| Network attacker | Tampering with firmware in transit | OTA URL must use HTTPS and the TLS server certificate must be trusted | No certificate pin rotation workflow or mTLS |
| Invalid firmware source | Device downloads from an unintended endpoint | URL is statically configured and HTTPS trust is enforced | No signed manifest, backend authentication, or release authorization layer |
| Interrupted OTA transfer | Partial image causes broken update | Incomplete transfers are rejected and `esp_https_ota_finish()` is only called after completion | No resume support or retry policy |
| Misconfigured certificate | Device cannot verify the OTA server | Supports ESP certificate bundle or a locally embedded certificate | Operational setup is manual and easy to misconfigure during local testing |
| Exposed credentials | Wi-Fi secrets leak into source control | `sdkconfig.defaults` uses placeholders and local `sdkconfig` is ignored | The project does not yet use secure provisioning or encrypted credential storage |

## What Is Currently Protected

- Plain HTTP firmware URLs are rejected.
- OTA requests rely on certificate validation through either the ESP-IDF certificate bundle or an embedded local certificate file.
- The code checks for HTTP `200` before proceeding.
- The image size must be present and must fit the inactive OTA partition.
- The incoming app descriptor is inspected before the full image is accepted.
- A same-version image is skipped.
- Bootloader rollback is enabled and the new image is only confirmed after a post-boot checkpoint.

## What Is Not Yet Protected

- Secure Boot is not enabled.
- Flash Encryption is not enabled.
- Firmware signing is documented but not enabled by default.
- Anti-rollback is not implemented.
- Credentials are not provisioned through a secure onboarding flow.
- There is no CI/CD release validation, artifact signing pipeline, or deployment approval gate.
- The project does not implement device identity, mutual TLS, or backend authorization.

## Recommended Future Hardening

### Secure Boot

Enable Secure Boot to reduce the risk of unauthorized firmware being executed on the device.

### Flash Encryption

Protect firmware and stored data at rest in flash, especially for devices deployed outside controlled environments.

### Firmware Signing

Use ESP-IDF signed app support with signing keys stored outside the repository. This would add a stronger trust boundary than HTTPS transport alone.

### Anti-Rollback

Add anti-rollback controls if downgrade protection becomes a requirement.

### Certificate Strategy

Use a managed CA chain, certificate pinning strategy, or an internal CA workflow that includes rotation and renewal practices.

### Secure Credential Storage

Move beyond manually configured local credentials toward secure provisioning and protected storage, such as NVS-based secure configuration or a dedicated onboarding flow.

### CI/CD Release Validation

Automate build verification, artifact integrity checks, and release gating before firmware is published to an OTA endpoint.

## Practical Guidance for This Repository

- Keep real Wi-Fi credentials and OTA URLs out of tracked files.
- Treat everything under `certs/` as local development material unless there is a deliberate reason to distribute public certificates.
- Do not commit private keys or signing keys.
- If you test with a self-signed server, rebuild the firmware after updating the pinned certificate file.
