# Portfolio Notes

## How to Describe This Project on a CV

Position it as an ESP-IDF firmware project focused on secure OTA delivery, update validation, and reliable boot behavior on ESP32.

## Suggested One-Line CV Bullet

Built an ESP32 secure HTTPS OTA firmware update prototype in ESP-IDF with dual OTA partitions, rollback-aware boot handling, and certificate-validated firmware delivery.

## Suggested Detailed CV Bullet

Designed and documented a modular ESP32 OTA update system in C with ESP-IDF, implementing HTTPS-only firmware delivery, image and partition validation, same-version skip logic, and bootloader rollback confirmation after a Wi-Fi health checkpoint.

## Interview Talking Points

- Why OTA systems need both transport security and boot reliability safeguards.
- Why the project uses `esp_https_ota` instead of a lower-level manual flash-write flow.
- How the partition table supports A/B style firmware updates on 4 MB flash.
- Why version equality checks matter even in a small prototype.
- How rollback confirmation is delayed until a basic runtime checkpoint is reached.

## Technical Questions an Interviewer May Ask

- Why is HTTPS alone not the same as signed firmware verification?
- What does `ESP_OTA_IMG_PENDING_VERIFY` mean in practice?
- Why does the project need `otadata` and dual OTA slots?
- What happens if the server returns a valid TLS connection but the wrong file?
- How would you extend this design for production fleet updates?

## Honest Limitations to Mention

- The project uses a static configured firmware URL rather than a backend update service.
- Version handling only skips identical versions and does not enforce update ordering.
- Secure Boot, Flash Encryption, anti-rollback, and signed firmware are not enabled by default.
- The included HTTPS server is a development convenience, not a production deployment tool.
