# PaperOS

PaperOS is custom firmware for the **M5Stack PaperS3**, with a touch launcher, clocks, weather, books, notes, drawing, games, and utilities. It is a modular C++ application on Arduino-ESP32/FreeRTOS, not a desktop OS with isolated application processes.

This guide describes the checked-in implementation, reviewed September 2026. Some historical planning documents describe features not yet complete; the limitations below take precedence.

> Screenshot placeholder: `docs/images/launcher.png` — add a home-screen image here.

## Contents

- [Development setup](#development-setup)
- [Navigation and first boot](#navigation-and-first-boot)
- [Settings and AP Mode](#settings-and-ap-mode)
- [SD-card structure](#sd-card-structure)
- [Apps](#apps)
- [Uploading books](#uploading-books)
- [PDF books](#pdf-books)
- [Architecture and customization](#architecture-and-customization)
- [Testing and troubleshooting](#testing-and-troubleshooting)
- [Limitations and credits](#limitations-and-credits)

## Development setup

### Hardware and software

Use a PaperS3, a USB **data** cable, and a microSD card for content. A card reader is the easiest way to copy books/images. This firmware targets PaperS3, not the original M5Paper.

| Tool/library | Purpose |
| --- | --- |
| VS Code + PlatformIO IDE | Editing, dependencies, compiler, build, upload, serial monitor |
| C++ / Arduino-ESP32 / FreeRTOS | Application code, hardware APIs, task scheduling |
| M5Unified / M5GFX | Display, touch, RTC, power, buzzer, fonts and image decoding |
| SD, SPI, WiFi, WebServer, Preferences | Storage, network, setup portal and persistent settings |
| ArduinoJson | JSON parsing for weather and flashcards |
| LVGL, PubSubClient | Declared integration dependencies; most current UI uses M5GFX directly |
| Python 3, Pillow, Poppler | Optional desktop PDF conversion |
| Git | Source history and collaboration |

`platformio.ini` is authoritative: environment `papers3`, platform `espressif32@7.0.1`, Arduino framework, board definition `esp32-s3-devkitm-1`, 16 MB flash configuration, and `qio_opi` memory configuration for PSRAM. M5Unified detects the actual PaperS3. Do not change the generic board name simply because it differs from the product name.

### Build and upload

1. Install VS Code and the **PlatformIO IDE** extension.
2. Open the project root containing `platformio.ini`, `include`, and `src`.
3. Let PlatformIO download its dependencies. The first build needs internet access.
4. Open a **PlatformIO terminal** in this folder; use it if `pio` is not available in your normal terminal.
5. Connect the device with a data cable and run:

```sh
pio run
pio device list
pio run -t upload
pio device monitor -b 115200
```

If necessary, select the port explicitly:

```sh
# Substitute the port listed on your Mac
pio run -t upload --upload-port /dev/cu.usbmodem101
pio device monitor --port /dev/cu.usbmodem101 -b 115200
```

On Windows use the listed port, such as `COM5`. Exit the monitor with **Ctrl+C**, including on macOS, before uploading. To clean and rebuild, run `pio run -t clean` followed by `pio run`. Firmware upload does not copy SD assets.

Wi-Fi can be configured on-device; a secrets header is optional. `include/config/Secrets.h.example` provides a developer fallback where needed. Never commit credentials/API keys. The ESP32 platform is deliberately pinned after earlier Network/WiFi framework compatibility issues.

### VS Code include errors

If the compiler succeeds but IntelliSense cannot find `sdkconfig.h`, FreeRTOS, or M5Unified headers, run **PlatformIO: Rebuild C/C++ Project Index**, then reload VS Code. Let PlatformIO manage include paths. After moving the project, reopen the new root and rebuild the index; do not copy framework headers into the project.

## Navigation and first boot

| Launcher row | Left | Middle | Right |
| --- | --- | --- | --- |
| 1 | Weather | Clock | Focus |
| 2 | Notes | To Do | Calendar |
| 3 | eReader | Photos | Sketch |
| 4 | Games | Flashcards | Utilities |

Tap a tile to open it. Games and Utilities use blocking selection popups; Close dismisses them. Launcher footer controls provide power options, manual Home refresh, and Settings. The header contains date, title, Wi-Fi and battery/charging indicators.

Footer sections are full touch targets. **Home** returns to the launcher; **Back** returns to a parent page; **Close** leaves the current reader/modal; **Prev/Next** change items/pages. Reader navigation is Prev/Close/Next; flashcard category navigation is Prev/Back/Next. Swipe support is app-specific—use visible controls where swipes are unavailable.

E-paper retains its last image without continuous drawing. Brief flash/ghosting can occur during updates; static controls should stay still during value changes, although some older screens still redraw larger regions.

## Settings and AP Mode

Open Settings from the launcher footer. Tap rows/arrows to enter subpages and Back to return. Device preferences generally survive restart; SD files are stored separately.

| Page | Controls |
| --- | --- |
| Wi-Fi & Network | Connection, saved SSID, signal, asynchronous scan, password entry, AP Mode |
| Home Address | Address, city, country, timezone; edit via portal |
| Clock & Time | Manual date/time, 12/24-hour, timezone, NTP sync, temperature unit, face |
| Weather Updates | Offline retry interval and weather face |
| Power & Sleep | Inactivity timeout, Sleep now, power-off confirmation |
| Button Sounds | Direct Yes/No toggle for shared touch feedback |
| System & Recovery | Monitor, restart, confirmed factory reset |
| Display & Font | Typography scale and 180-degree display flip |
| Flashcard AI | Key status, model names, link to AP configuration |

### Connect to your router

1. Go to **Wi-Fi & Network → Search Wi-Fi**.
2. Wait for results, select a 2.4 GHz network, and type its password.
3. Use `ABC/abc` for case, `123` for symbols, Space and Back for editing. The native input currently displays entered characters.
4. Tap Connect and check status. For a hidden/unlisted SSID, use the portal's manual network field.

### AP setup, password and QR

1. Enable AP Mode (disabled by default).
2. Scan its QR with your phone camera, or join **PaperOS-Setup**, password **paperossetup**.
3. Stay connected even if the phone reports no internet.
4. Open **http://192.168.4.1** manually; automatic captive-portal opening is not required.
5. Save changes. The page redirects and reads stored settings. Saved secret values are not displayed back as plaintext.
6. Disable AP Mode when finished; the help and QR disappear.

The QR joins Wi-Fi; it does not submit settings or open the portal automatically. Its credentials share the same source as the access point.

The portal supports Wi-Fi/manual SSID, address/city/country, coordinates, POSIX timezone, 24-hour format, sleep/retry intervals, sounds, Gemini configuration, and uploads. It does **not** expose every device setting. Address edits do not automatically geocode: update latitude/longitude for weather too. The timezone expects a POSIX expression such as the project's India setting `IST-5:30`.

General uploads go to `/PaperOS/uploads` with an 8 MB limit. Font uploads go to `/PaperOS/fonts` with a 2 MB limit. Uploads use temporary `.part` files. Storing a font does not automatically make it selectable. Move books from `uploads` to `books` using Files or a card reader.

> Screenshot placeholders: `docs/images/settings.png`, `docs/images/wifi-ap-qr.png`, `docs/images/setup-portal.png`.

### Time, sleep and reset

Use Clock & Time to adjust day/month/year/hour/minute and save when NTP is unavailable. The time service writes the hardware RTC when available. Shared settings control 12/24-hour format and Celsius/Fahrenheit.

An inactivity timeout of zero disables automatic sleep. Clock, Weather and Focus inhibit global inactivity sleep while active; Focus also has its own idle view. Sleep displays a static page, and touch wake returns to the launcher. Use power options for Sleep, Off or Cancel; preserve the device's hardware power-button safety behavior. Turn the device on using its hardware power control after shutdown.

Factory reset clears preferences/credentials but preserves SD files. An SD backup does not include preferences stored in device flash.

## SD-card structure

Use a card compatible with Arduino's SD library; FAT32 is the recommended starting format. Back up files before formatting. These paths are on the card:

```text
/PaperOS/
  books/              EPUB, TXT, MD, converted PDF folders
  photos/             JPG/JPEG, PNG, BMP
  notes/YYYY-MM-DD/   note_001.txt, note_002.txt, ...
  todo/tasks.txt      Task data shared with Calendar
  writing/           Dated saved sketches
  flashcards/        Category JSON decks
    images/          Card illustrations
  chess/             Twelve chess-piece PNGs
  icons/weather/     Eight weather PNGs
  fonts/             Uploaded fonts
  uploads/           General portal uploads
  cache/             EPUB chapter/image cache
  backup/            Diagnostic snapshot
  pomodoro/          Optional pomodoro.png idle image
```

Power off before removing the card. Safely eject it from your computer and reopen the relevant app after reinsertion. USB SD Mass Storage mode is **not implemented**.

## Apps

### Weather

Configure router connectivity and location coordinates first. Weather uses Open-Meteo with a background request service. Use the refresh icon for new data; offline retries use Settings → Weather Updates. Forecasts are outdoor estimates, not onboard sensor readings. Normally landscape, with double-tap fullscreen and context-dependent Prev/Next forecast navigation. Select a face through settings or the app's face-change interaction.

| Face | Display |
| --- | --- |
| Dashboard | Current conditions, hourly strip and selected-day detail |
| Minimal | Large current temperature and essential conditions |
| Hourly | Next-hours forecast |
| Daily | Selected-day forecast |
| Today Card | Compact current weather card |
| Timeline | Hourly temperature timeline |
| 3-Day | Three forecast-day cards |
| Weather Clock | Weather and time |

Copy eight PNGs from `assets/weather` to `/PaperOS/icons/weather`, preserving names. Missing files use built-in fallback drawings. See [asset mapping](assets/weather/README.md). Labels use configured city/country.

> Screenshot placeholder: `docs/images/weather-faces.png`.

### Clock: all seven faces

Tap content once to cycle faces, or choose Settings → Clock & Time → Clock face. Double-tap enters/exits fullscreen. Single taps do not cycle while fullscreen. The face selection persists.

| Face | Layout |
| --- | --- |
| Flip clock | Landscape split cards for individual time digits |
| Minimal digital | Landscape large time, date, outdoor temperature |
| Analog | Landscape bold dial/hands, date and temperature |
| Date & temperature | Landscape date-focused layout with time/weather |
| Dotted | Landscape dot-matrix time and date/temperature |
| Dashboard panel | Landscape split time/information panel |
| Dark flip | Portrait dark tiles: hour/minute, month/date/day, temperature/humidity |

Faces use shared time/unit preferences and display flip. Dark flip selects portrait automatically; other faces use landscape. Leaving Clock restores portrait. Dark flip updates changed tiles on minute ticks; older faces vary in redraw scope.

Weather values are cached. Open Weather and fetch successfully to populate them; Clock does not perform a separate live weather request. `--` means unavailable. Values can be stale; Dark flip labels them as cached outdoor readings and draws its degree symbol geometrically.

> Screenshot placeholders: `docs/images/clock-faces.png`, `docs/images/clock-dark-flip.png`.

### Focus / Pomodoro

Choose **25**, **5**, or **30 minutes**, then Start, Pause or Stop. Tap the timer to cycle **Ring**, **Minimal**, **Progress**, and **Focus** styles. The upper refresh icon redraws it. Timer changes repaint the timer region instead of the six static controls. The buzzer signals actions/completion. Reopening the app starts a new session; it is not a persistent background timer. Its idle view optionally uses `/PaperOS/pomodoro/pomodoro.png`.

### Notes

Use the QWERTY keyboard, Shift, punctuation, Space and Back. Save creates `/PaperOS/notes/YYYY-MM-DD/note_001.txt` and subsequent numbers, starting again each date. Set the correct date first. Clear empties the draft. Retrieve files via Files or a computer. This is plain-text entry, not a rich-text editor.

### To Do and Calendar

To Do provides task entry and task-management/completion controls with a Notes-style keyboard. Data persists at `/PaperOS/todo/tasks.txt`. Calendar displays the month grid and date-filtered tasks; Prev/Next browse months. Check task dates if an item does not appear for today. These apps do not synchronize with Microsoft To Do or Google Calendar.

### eReader

Tap a Library entry, then use **Prev / Close / Next**. The list shows format, size and reading status. Supported reading progress is saved; retain a book's path when possible so its saved position remains associated.

Tap the book title for Reader Settings: **Serif/Sans**, **Normal/Large text**, **Compact/Normal/Relaxed line spacing**, **Chapters**, and **Jump Forward (+10%)**. Chapters opens a blocking selector; Close returns to settings. Text formatting changes apply to supported text, not text embedded in PDF page images.

The local EPUB adapter handles container/manifest/spine, semantic blocks, supported JPEG/PNG illustrations and SD cache. It is bounded and does not provide full browser-grade CSS/layout or the complete upstream CrossPoint engine. See upload instructions below.

> Screenshot placeholders: `docs/images/reader-library.png`, `docs/images/reader-page.png`, `docs/images/reader-settings.png`.

### Photos

Copy JPG/JPEG, PNG or BMP files directly to `/PaperOS/photos`. Use Prev/Next or horizontal swipes. Double-tap toggles fullscreen. Images are fitted/centered with aspect ratio preserved, so margins are normal. Loading feedback appears before decoding; resize large photos on a computer for better performance.

### Sketch

Draw inside the canvas. Toolbar: **Pen, Erase, Undo, Clear, New, View, Save**. Empty/cleared drawings are not saved. Save overwrites the current drawing until New starts another. View browses sketches under `/PaperOS/writing`, grouped by date when available. Capacitive touch drawing does not provide pressure sensitivity, palm rejection or handwriting recognition.

### Flashcards

Select **Dinosaurs, Space, Animals, Science, Geography or History** on the landing page. Inside a category use Prev/Back/Next. Generate requests another item when Wi-Fi and Gemini configuration are available. Cards are saved to category JSON for offline use and duplicate names are checked. Facts/metadata and optional images are displayed; historical reveal-button descriptions may not match the current screen.

Deck files are `/PaperOS/flashcards/<category>.json`. Firmware seeds the dinosaur starter deck; do not assume every category already contains 200 items. Example:

```json
{"deck":"DINOSAURS","cards":[{
  "name":"Tyrannosaurus rex",
  "fact":"A large theropod from Late Cretaceous North America.",
  "era":"Late Cretaceous","diet":"Carnivore","length":"About 12 m",
  "region":"North America","image":"images/tyrannosaurus-rex.png"
}]}
```

Images are relative JPEG/PNG paths; empty `image` is valid. Prefer modest illustrations around 180×130 pixels. Legacy metadata keys also carry category-specific information. Back up decks before editing.

Configure Gemini key/models via Settings → Flashcard AI → Enable AP Setup and the portal. Use models available to your Google project. Free/paid quotas and image eligibility depend on that project. Text can save even if image generation fails; review AI facts for accuracy.

`tools/generate_flashcard_decks.py` accepts `GEMINI_API_KEY`, `--model`, `--count`, and `--output` to generate bulk decks. It makes real API requests and overwrites output category files; use a staging folder and an available model rather than assuming its hard-coded default works. It creates image placeholders, not finished illustrations.

### Games

| Game | Controls and scope |
| --- | --- |
| Chess | Play White: tap piece then destination; Black replies. New Game requires Yes/No. Captures, king safety, queen promotion and checkmate/stalemate for both sides are implemented. Castling, en passant, repetition/fifty-move draws are not. |
| Sudoku | Select cell then number; Clear Cell and New Puzzle controls. Fixed clues cannot be edited. |
| Snake | Direction buttons or swipes; collect food, avoid walls/body. Game-over sound and persistent high score. |
| Tetris | Buttons or left/right/down swipes; up rotates. Next-piece preview, score/high score and reset. E-paper limits motion smoothness. |
| Minesweeper | 9×9, ten mines. Reveal opens cells; Flag marks suspects. First reveal and neighbours are safe; Reset confirms. Safe opening does not guarantee a no-guess board. |

For chess images, copy the twelve PNGs from `assets/chess` to `/PaperOS/chess`; otherwise letters are used. Chess now updates only affected squares and changed status text during play. These are lightweight game implementations; device playtesting remains important.

> Screenshot placeholder: `docs/images/games.png`.

### Utilities and System Monitor

| Utility | Usage and status |
| --- | --- |
| Calculator | Arithmetic, sign, percent, decimal, AC, backspace, M+ and MR. Previous entry is shown above result; calculations refresh the display area. |
| Converter | Change Unit cycles Celsius→Fahrenheit, km→miles, kg→lb, litres→US gallons, cm→inches, kPa→PSI. Basic directional conversion with numeric keypad. |
| Files | Browse SD folders, select items, create folders, delete with confirmation, move selected files. Back returns to parent; Prev/Next paginate. |
| Alarm | Schedule-entry UI only: hour/minute and enable controls. Reliable background ringing is not implemented. |
| Backup | Create Snapshot writes diagnostic `/PaperOS/backup/system.txt`. Not a full settings/content backup or restore system. |

System Monitor lives in **Settings → System & Recovery**. HA Home remains hidden; its module is registered for future integration. Shopping List is not exposed. Compass was removed because the onboard IMU does not provide magnetic heading.

For a real backup, copy the entire `/PaperOS` SD directory to your computer and retain firmware source separately. NVS device preferences are not included in that copy.

## Uploading books

### Direct SD copy (recommended)

1. Power off, remove the card, and mount it in your computer.
2. Copy DRM-free `.epub`, `.txt` or `.md` files directly to `/PaperOS/books`.
3. Prefer UTF-8 text and ordinary reflowable EPUBs. Markdown is treated as text; full Markdown rendering is not promised.
4. Safely eject/reinsert the card, start PaperOS, open eReader and tap the book.

Do not unpack EPUBs. Raw PDFs require conversion. Arbitrary nested book directories are not equivalent to recognized PDF folders.

### AP upload

Join AP Mode and upload through SD Card Updates. Files up to 8 MB go to `/PaperOS/uploads`. Move them into `/PaperOS/books` through Utilities → Files and reopen eReader. Use a card reader for larger books or PDF directories; the portal does not upload/unpack complete directory trees or ZIP books.

## PDF books

PDF pages are rendered **on your computer** into PNGs; the device does not parse raw PDFs. This preserves layout but provides fixed page images, without PDF text reflow, selection or search.

### macOS

With Homebrew already available, install Poppler. Use a Python environment for Pillow:

```sh
brew install poppler
python3 -m venv /tmp/paperos-pdf-venv
source /tmp/paperos-pdf-venv/bin/activate
python -m pip install Pillow
python tools/pdf_to_papers3.py "$HOME/Downloads/book.pdf" \
  --output "/Volumes/SDCARD/PaperOS/books" --title "My Book"
```

Replace `SDCARD` with your mounted volume. The temporary Python environment may need recreating after cleanup; a persistent location outside the repository is also fine.

### Windows

Install Python 3 and Poppler and ensure `pdftoppm` is on PATH. The included wrapper provides this setup:

```powershell
winget install oschwartz10612.Poppler
py -m pip install Pillow
.\tools\Convert-PdfToPaperS3.ps1 -Pdf "$HOME\Downloads\book.pdf" `
  -Output "E:\PaperOS\books" -Title "My Book"
```

Replace `E:` with your card. If script execution is restricted, invoke Python directly:

```powershell
py tools\pdf_to_papers3.py "$HOME\Downloads\book.pdf" --output "E:\PaperOS\books" --title "My Book"
```

The converter supports `--dpi` (default 150); the wrapper uses `-Dpi`. Output pages are 540×780 grayscale PNGs, centered and quantized to four levels:

```text
/PaperOS/books/book/
  manifest.txt
  page_001.png
  page_002.png
  ...
```

The manifest contains `PAPERS3_PDF=1`, `TITLE=...`, and `PAGES=...`. Keep names unchanged and copy the **whole folder** when converting to local storage first. Use a fresh destination when reconverting a different book with the same name. Large PDFs require temporary space for rendered pages.

After reinserting the card, choose the converted book and use Prev/Close/Next. Progress associates with its path. Very small PDF print may still be hard to read on a 540-pixel-wide display; crop/simplify the source on a computer if necessary.

## Architecture and customization

```text
platformio.ini                 Build/dependency configuration
partitions.csv                 Flash layout
src/main.cpp                   Register services/apps/navigation
include/app/ + src/app/        IApp, AppContext, AppManager
include/apps/ + src/apps/      Individual app models/views
src/platform/                  Device capabilities and workers
include/services/Services.h   Shared service interfaces/state
src/services/Services.cpp     Service composition/ticking
include/ui/ + src/ui/          Chrome, Theme, Typography, Icons, Popup
assets/                        Optional SD assets/licenses
tools/                         PDF/deck desktop helpers
test/                          Regression checks
docs/                          Detailed guides, audit, test plan
```

For .NET developers: `IApp` is a lifecycle interface (`onStart`, `onTick`, `onStop`); `AppContext` supplies dependencies; `AppManager` coordinates navigation. All apps share one address space and one hardware initialization. A crash can restart the whole device.

Add an app with its own header/source, implement `IApp`, add an `AppId`, register/instantiate in `main.cpp`, and add a launcher/modal destination. Do not import standalone `setup()`/`loop()` or reinitialize display/SD. Stop owned work and release resources when leaving.

| Customization | Source |
| --- | --- |
| Header/footer labels, geometry, hit areas | `src/ui/Chrome.cpp`, `include/ui/Chrome.h` |
| Borders, radii, shared sizes/colors | `include/ui/Theme.h`, `src/ui/Theme.cpp` |
| Font roles/scaling | `include/ui/Typography.h`, `src/ui/Typography.cpp` |
| Icons | `src/ui/Icons.cpp` and associated assets |
| Modal headers/Close | `src/ui/Popup.cpp` |
| Launcher layout and folders | `src/apps/launcher/LauncherApp.cpp` |
| Clock rendering and saved face names | `src/apps/clock/ClockApp.cpp`, `src/platform/time/ClockFaceService.cpp` |
| Settings UI/persistence | `src/apps/settings/SettingsApp.cpp`, `src/platform/settings/DeviceSettingsService.cpp` |
| AP HTML/upload routes | `src/platform/network/NetworkService.cpp` |

Use runtime Chrome bounds, Theme's three-pixel frames and semantic Typography roles. Draw static buttons/borders on entry and refresh changed areas afterward. Match hitboxes to geometry. Some older apps still have direct fonts/fixed coordinates, so global changes do not automatically update every element.

Large EPUB buffers use bounded PSRAM allocations; inflater state uses internal memory to avoid loop-task stack overflow. Images are decoded as needed. Avoid blocking work in ticks and close files/free buffers. Network workers transfer results to the UI. E-paper updates, HTTPS, AP transfers, image decoding and games consume power; partial updates reduce work but do not guarantee zero flicker.

There is no fixed resource-usage percentage: `pio run` reports static RAM/flash, and System Monitor reports runtime resources. Free heap does not measure CPU utilization. Content size, active app and Wi-Fi activity affect headroom/battery life.

## Testing and troubleshooting

Run `pio run` and `python3 test/chess_rules_check.py`. See [TEST_PLAN.md](docs/TEST_PLAN.md) for device testing. Build success does not prove touch alignment, QR scanning, image appearance or battery behavior.

| Symptom | Check |
| --- | --- |
| Cannot upload | Exit monitor, check data cable and `pio device list` |
| Editor-only header errors | Rebuild PlatformIO C/C++ index |
| Network.h missing after upgrades | Restore pinned project configuration and inspect framework packages |
| Partition overlap | Check `partitions.csv`; do not independently move offsets |
| Missing SD files | Mount status, exact folder/extension, reopen app |
| PDF missing | Convert first; keep manifest and numbered pages together |
| EPUB section too large | Try a smaller reflowable EPUB; bounded parser limits apply |
| Stale EPUB after replacement | Back up/remove its files in `/PaperOS/cache`, reopen |
| AP portal absent | Stay joined; manually open `http://192.168.4.1` |
| Weather offline | Router/internet, coordinates, retry setting and serial logs |
| Clock temperature/humidity stale | Open Weather and fetch successfully |
| Gemini failure | Key, model access, quota, internet, `[GEMINI]` serial output |
| Chess letter pieces | Copy twelve asset PNGs to `/PaperOS/chess` |

Check logs for secrets before sharing them. Factory reset clears preferences, not SD content; use it as recovery rather than the first troubleshooting step.

## Limitations and credits

- No USB SD Mass Storage, application package installation or implemented OTA update workflow.
- Alarm ringing and full backup/restore are incomplete.
- Reader lacks full EPUB CSS fidelity and user-facing margins/orientation/cache-clear settings.
- Uploaded fonts are stored; arbitrary font activation is incomplete.
- HA Home is hidden, and Chess special moves/draw rules remain incomplete.
- Centralized typography and minimum-area refresh have not yet reached every older screen.
- Local preferences are not an encrypted credential vault.

### Third-party repositories used as references

PaperOS is a unified implementation, but its apps were informed by these projects:

| Repository | PaperOS usage |
| --- | --- |
| [Boisti13/papers3-dashboard](https://github.com/Boisti13/papers3-dashboard) | Dashboard, Home Assistant and power-management reference |
| [squirmen/PaperS3Weather](https://github.com/squirmen/PaperS3Weather) | Weather data and e-paper presentation reference |
| [juicecultus/EPub-M5Stack-Paper-S3](https://github.com/juicecultus/EPub-M5Stack-Paper-S3/tree/experimental) | Earlier EPUB parsing/pagination reference |
| [juicecultus/crosspoint-reader-papers3](https://github.com/juicecultus/crosspoint-reader-papers3) | EPUB architecture, spine, cache and reader-experience reference |
| [omeriko9/M5Paper_PaperS3_eBookReader](https://github.com/omeriko9/M5Paper_PaperS3_eBookReader) | Reader behavior and format research |
| [arunmathaisk/PaperS3-chess](https://github.com/arunmathaisk/PaperS3-chess) | Chess layout, initial turn flow and optional PNG pieces |


PaperOS is released under the root [MIT License](LICENSE), copyright © 2026 Saurav Singh.