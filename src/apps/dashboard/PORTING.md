# Dashboard port checklist

- Retain the PaperS3 LVGL pages, MQTT topic dispatcher, OTA UI, and HA data models.
- Delete/rename its standalone `setup()` and `loop()`.
- Use shared `NetworkService`, `StorageService`, `TimeService`, and `DisplayService`.
- Allocate LVGL draw buffers in PSRAM and free app-owned LVGL objects in `onStop()`.
- Do not permit another app to call LVGL while DashboardApp is inactive.
