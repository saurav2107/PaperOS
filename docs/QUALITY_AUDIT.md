# Paper OS quality and optimization audit

Audit date: 2026-08-04

## Baseline

- Release build succeeds with PlatformIO.
- `firmware.bin`: approximately 1.4 MiB.
- OTA application slot: 5.5 MiB; current binary is comfortably below it.
- Static internal DRAM use reported by the ELF is approximately 24 KiB data +
  51 KiB BSS. EPUB and image work uses PSRAM deliberately.
- No FreeRTOS worker is currently created. `TaskRegistry` is an empty future
  extension; every app runs from Arduino's `loopTask`.

## What is already sound

- Apps are long-lived static instances registered by `AppManager`; no app is
  allocated with `new`, and navigation invokes `onStop` before activation.
- Reader ZIP/EPUB allocations are bounded and the inspected `ps_malloc`,
  `heap_caps_malloc`, and `malloc` paths release buffers on success and error.
- EPUB content is size limited (48 KiB sections, 180 KiB images) before
  decompression, which is essential for malformed books.
- SD content now has a single top-level `/PaperOS` hierarchy.
- Weather uses a bounded 10-second HTTP timeout and avoids repeated offline
  redraws. Wi-Fi scans are asynchronous.
- Tetris uses changed-cell rendering rather than repainting its whole board.
- Sleep uses `waitDisplay()` before light sleep, preventing the previous
  sleep-screen timing defect.

## Priority remediation backlog

### P0 — correctness and data safety

1. **Use saved location everywhere.** `NetworkService` saves address/city/
   country, but `WeatherApp` still requests the compiled `kHomeLocation`
   latitude/longitude and renders its compiled label. Create a single
   `DeviceSettingsService`/`LocationService` and have Weather, Settings, and
   the AP portal read it. Add a geocoding/coordinate editing workflow.
2. **Make the SD AP uploader safe.** Upload size is not capped, free space is
   not checked, and incomplete uploads are not cleaned up. Stream to a
   temporary file, enforce extension/size allowlists, verify write success,
   then rename atomically. Do not allow uploads while USB mass storage owns SD.
3. **Reader cache lifecycle.** EPUB image/chapter cache files in
   `/PaperOS/cache` are never evicted. Add cache size accounting, an LRU/age
   limit, and make Reader Settings' cache-clear action actually delete only
   cache-owned files.

### P1 — responsiveness, power, and recovery

4. **Move network I/O off the UI loop.** Weather performs a synchronous HTTPS
   request and may hold `loopTask` for up to 10 seconds. Use one owned FreeRTOS
   network worker, a result queue, cancellation on app exit, and an explicit
   task registry. The same worker can handle NTP and AP-related work.
5. **Harden `AppManager::activate`.** If `onStart()` returns false, `active_`
   already points at the failed app. Retain the previous safe app or fall back
   to Launcher and render a recoverable error message.
6. **Add runtime memory telemetry.** System Monitor should record minimum
   free heap/PSRAM, largest free block, task stack watermark, SD free space,
   and reset reason. Log before/after Reader, Photo, Weather, and AP tests.
7. **Audit blocking delays.** Game-over audio deliberately blocks briefly, but
   NTP can block for five seconds and Weather for ten. A central async job
   service makes this policy explicit.

### P2 — configuration and UI consistency

8. **Create one settings repository.** `Preferences` reads/writes are spread
   across Time, Power, Weather, Sound, fonts, scores, Reader, and Network.
   Replace the duplicated `"paperos"` access with a typed
   `DeviceSettingsService`, validation, defaults, a schema version, and one
   save path. This is analogous to .NET `IOptions<T>` plus a configuration
   provider.
9. **Finish typography adoption.** `ui::Typography` now persists a global
   scale and Sudoku/Chess/Snake/Tetris adopt semantic roles. Most other apps
   still call `setFont`/`setTextSize` directly, so the setting is not yet truly
   global. Migrate shared Chrome first, then Settings, Launcher, Weather,
   Calendar, Reader, Notes, Todo, Files, and Flashcards.
10. **Centralize UI components.** Border thickness, rounded frames, action
    buttons, list rows, and game buttons are repeated. Extract
    `ui::Components` (`frame`, `primaryButton`, `listRow`, `modal`) and shared
    layout metrics. This prevents a visual fix in one app from being missed in
    another.
11. **Centralize SD paths.** `/PaperOS/...` is broadly used but still repeated
    string-by-string. A `StoragePaths` module should own directory constants,
    initialise them, and provide safe joins/normalisation.

### P3 — security and product completeness

12. **Secure provisioning.** AP credentials are fixed and the local portal has
    no session protection. Generate/display a per-device AP password, time
    out AP mode, require confirmation before enabling uploads, and avoid TLS
    `setInsecure()` where certificate validation is feasible.
13. **Make font upload honest or complete.** Fonts are accepted into
    `/PaperOS/fonts` but are not dynamically loaded or applied. Either finish
    VLW font validation/loading, or hide the upload control until it works.
14. **USB mass storage remains deferred.** The partition/storage abstraction
    exists, but TinyUSB MSC ownership, host attach detection, transfer-idle
    checks, and safe mount/unmount behavior are not implemented.
15. **OTA slots are unused.** The partition table reserves two OTA slots, but
    there is no signed update flow. Do not expose a software-update UI until
    that feature has an authenticated implementation.

## Memory and leak conclusion

No direct allocation leak was found in the inspected EPUB decompression paths:
their PSRAM/internal allocations are freed on every visible success/error
path. This is not a substitute for runtime proof. The most credible memory
risks are Arduino `String` fragmentation in long-lived AP HTML/EPUB parsing,
unbounded cache growth on SD, and decoder/library transient allocations.

Use the soak tests in `docs/TEST_PLAN.md` before treating the firmware as a
daily-driver OS. Record heap and PSRAM before/after each loop; a downward trend
over repeated launches is a release blocker.
