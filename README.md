# Paper OS

A modular-monolith PlatformIO scaffold for M5Stack PaperS3.

## Active source layout

```text
src/main.cpp                     composition root only
src/app/                         app registry and lifecycle manager
src/apps/launcher/               launcher app
src/apps/pomodoro/               Pomodoro app
src/apps/notes/                  SD-backed Notes app
src/apps/chess/                  Chess app and upstream-porting notes
src/apps/settings/               settings app
src/apps/native/                 reusable placeholder/native app presentation
src/ui/Chrome.cpp                shared header/footer navigation
src/platform/{power,network,...} device capabilities
src/services/Services.cpp        composes native capabilities into AppContext
```

Every app draws between the shared `ui::Chrome` header and footer. Header Back,
footer Back, and footer Home always return to the launcher unless an app opts
into the optional Previous/Next controls for paged content.

## First build

1. Copy `include/config/Secrets.h.example` to `include/config/Secrets.h` and enter Wi-Fi/MQTT values.
2. Open this folder in VS Code with the PlatformIO extension.
3. Run `pio run`, then `pio run -t upload`.

## Integration order

1. Dashboard: port as `DashboardApp`; keep LVGL confined to the display service/UI task.
2. Weather: extract Open-Meteo client and renderer into `WeatherApp`.
3. Pomodoro: move timer logic into `onTick()`; no blocking loops.
4. EPub: port last; it must not initialize a second display or SD stack.

## Memory rule

Use `heap_caps_malloc(..., MALLOC_CAP_SPIRAM)` for large image, LVGL, font, and ePub buffers. Free them in `onStop()`. Keep DMA/interrupt-sensitive allocations and task stacks in internal RAM.
