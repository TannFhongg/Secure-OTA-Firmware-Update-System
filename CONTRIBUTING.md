# Contributing

## Scope

Contributions are welcome if they improve the ESP-IDF firmware, OTA reliability, security posture, or documentation quality of the project.

## Code Style

- Follow the existing C and ESP-IDF coding style in the repository.
- Keep functions focused and avoid mixing boot logic with OTA implementation details.
- Prefer clear logging around OTA state transitions and failures.
- Add brief comments only where behavior is not obvious from the code itself.

## Commit Style

- Use small, focused commits with clear messages.
- Prefer commit subjects that describe the outcome, such as `Add OTA certificate fallback notes` or `Validate OTA image size before flashing`.
- Avoid mixing unrelated refactors, feature work, and documentation cleanup in one commit when possible.

## Testing Expectations

- Run `idf.py build` after firmware changes.
- If you modify OTA behavior, test at least one successful OTA path and one failure path when hardware is available.
- If you change the Python OTA server, verify the script still starts and serves the expected firmware file.
- Include relevant logs or reproduction notes when reporting or fixing bugs.

## Documentation Expectations

- Update `README.md` and the relevant files under `docs/` when behavior, configuration, or security assumptions change.
- Keep examples free of real credentials, real internal URLs, and private keys.
- Prefer honest wording such as `prototype`, `demonstrates`, or `planned` unless the repository clearly proves a stronger claim.

## Security-Sensitive Contributions

- Do not commit Wi-Fi credentials, private keys, signing keys, or local certificates unless there is an explicit and reviewed reason to do so.
- Do not weaken the HTTPS-only OTA requirement without documenting the risk and intended use case.
- Document security tradeoffs for changes involving certificate handling, firmware validation, partition layout, or rollback behavior.
