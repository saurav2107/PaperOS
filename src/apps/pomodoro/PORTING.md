# Pomodoro port

This app adapts the behavior and UI logic from:

`https://github.com/micokonsep/pomodoro-papers3/blob/main/src/main.cpp`

Adaptations made for Paper OS:

- Removed the upstream standalone `setup()` and `loop()` functions.
- Reused the OS-owned M5 hardware, SD card, top bar, touch processing, and Back navigation.
- Replaced device-wide deep sleep with an in-app lock screen; an individual app must not power down the whole launcher.
- Preserved the 60-dot seconds ring, duration ring, play/pause/stop controls, 25/5/30-minute presets, buzzer feedback, anti-ghosting refresh, and optional `/PaperOS/pomodoro/pomodoro.png` SD lock-screen asset.
