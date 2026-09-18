# Native platform capabilities

This folder owns device-level features rather than applications:

- `power/` — battery telemetry and sleep policy.
- `network/` — Wi-Fi connection and provisioning portal.
- `storage/` — microSD and future USB Mass Storage ownership.
- `time/` — RTC/NTP synchronization.
- `input/` — touch/button polling.
- `display/` — PaperS3 hardware initialization.

The existing service interfaces are composed by `Services`; implementations are
being moved here incrementally so apps never reach into hardware directly.
