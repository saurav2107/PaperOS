# Third-party source policy

Add repositories here as Git submodules or pinned source snapshots. Do not add their `setup()`/`loop()` files to the unified build.

| Source | Target adapter | First extraction |
|---|---|---|
| `papers3-dashboard` | `DashboardApp` | LVGL pages, MQTT client, HA models |
| `PaperS3Weather` | `WeatherApp` | Open-Meteo client, settings model, renderer |
| `pomodoro-papers3` | `PomodoroApp` | timer state machine and touch behavior |
| `EPub-M5Stack-Paper-S3` (`experimental`) | `EpubApp` | parser/pagination first, renderer last |
| `crosspoint-reader-papers3` | `CrossPointReaderApp` | Historical design reference only; its local clone was removed because PaperOS uses its own adapter |

Example commands, run from this folder after reviewing each license:

```bash
git submodule add https://github.com/Boisti13/papers3-dashboard.git papers3-dashboard
git submodule add https://github.com/squirmen/PaperS3Weather.git PaperS3Weather
git submodule add https://github.com/micokonsep/pomodoro-papers3.git pomodoro-papers3
git submodule add -b experimental https://github.com/juicecultus/EPub-M5Stack-Paper-S3.git EPub-M5Stack-Paper-S3
```
# PaperS3-chess

`papers3-chess` is retained as the upstream reference and asset source for the
first-party `ChessApp`. Copy `assets/chess_pieces/*.png` to `/chess` on the SD
card; see `src/apps/chess/PORTING.md`.
