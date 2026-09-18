# Paper OS device validation plan

Run on real PaperS3 hardware with a microSD card, Wi-Fi router, USB cable,
and a serial monitor. Record firmware version, battery percentage, free heap,
free PSRAM, and reset reason at the start and end of every soak test.

## Release gates

| ID | Test | Pass criteria |
|---|---|---|
| R-01 | `pio run -s` | Clean build with no errors. |
| R-02 | Upload then cold boot | Launcher appears within 10 seconds; no reset loop. |
| R-03 | Restart and Factory Reset | Restart preserves settings; Factory Reset clears PaperOS preferences but leaves SD files. |
| R-04 | 30-launch soak | Open/close every app 30 times; no crash, no persistent downward heap/PSRAM trend. |

## Shell, navigation, and display

| ID | Test | Pass criteria |
|---|---|---|
| UI-01 | Header/footer on every app | No clipped borders; title, Wi-Fi, battery, charging mark, and footer labels are legible. |
| UI-02 | Footer permutations | BACK only appears for true child pages; HOME exits to Launcher; PREV/NEXT work where advertised. |
| UI-03 | Touch/swipe | One tap produces one action; reader/photo/weather swipe navigation does not trigger footer actions. |
| UI-04 | Font scale 90–130% | Settings value persists after reboot; migrated apps remain readable and do not overlap. |
| UI-05 | E-paper refresh | Tetris, Snake, Pomodoro timer, reader turns, keyboards, and settings rows refresh only their changing region. |

## Settings, time, power, and networking

| ID | Test | Pass criteria |
|---|---|---|
| SYS-01 | Manual time, 12/24-hour, timezone | Values survive restart; RTC and header/clock agree. |
| SYS-02 | NTP sync online/offline | Online sync updates RTC; offline failure returns promptly without a frozen UI. |
| SYS-03 | Inactivity sleep | Test 1, 5, and disabled settings; sleep screen commits before sleep; touch returns to Launcher. |
| SYS-04 | Power button safety | Verify built-in behavior remains intact; short/double/long custom routing must not bypass hardware safety. |
| NET-01 | Wi-Fi connection | Saved SSID reconnects after reboot; RSSI/header state changes correctly. |
| NET-02 | AP portal | Enable/disable AP; configure a visible and hidden SSID; reload page and verify saved fields. |
| NET-03 | AP responsiveness | Keep portal open while touching device for five minutes; UI remains responsive. |
| NET-04 | Invalid network | No reset loop or repeated full-screen refresh; weather retry respects configured interval. |

## SD card, files, and media

| ID | Test | Pass criteria |
|---|---|---|
| SD-01 | Folder hierarchy | Firmware creates `/PaperOS` subfolders when a blank FAT32 SD card is inserted. |
| SD-02 | Files app | Navigate three levels deep; BACK returns correctly; create, select, move, delete, and cancel-delete work. |
| SD-03 | SD removal | Remove card only while not writing; every SD app reports a recoverable error rather than crashing. |
| SD-04 | Photos | Test portrait/landscape JPG, PNG, BMP and long names; each image is centered, aspect-fit, and navigation works. |
| SD-05 | Notes/Todo/Calendar | Create a note and task; reboot; verify date folders, task persistence, and selected-date Calendar filtering. |
| SD-06 | AP upload safety | Test allowed, oversized, interrupted, duplicate, and no-space uploads after uploader hardening. |

## Reader

| ID | Test | Pass criteria |
|---|---|---|
| READ-01 | TXT/Markdown | Open, page, close, reopen; reading position and opened state persist. |
| READ-02 | EPUB variety | Test small/large chapter, internal CSS, external CSS, headings, lists, images, TOC, chapter selection, and resume. |
| READ-03 | Bad EPUB | Missing container, invalid ZIP, oversized chapter/image produce an error screen without a reboot. |
| READ-04 | Converted PDF | Open page PNG folder; PREV/NEXT persists exact page after close/reopen; title is not clipped. |
| READ-05 | Reader soak | Open/close ten books and clear cache; compare free heap/PSRAM before and after. |

## Apps and games

| ID | Test | Pass criteria |
|---|---|---|
| APP-01 | Clock faces | All faces including Dotted rotate correctly; clock restores portrait on exit. |
| APP-02 | Weather faces | Online request, offline state, manual refresh, day PREV/NEXT, unit switch, and landscape restoration. |
| APP-03 | Pomodoro | Presets/start/pause/stop/style, sound, no inactivity sleep while active, and no static-button flicker. |
| APP-04 | Calculator/converter | Operators, `%`, memory, backspace, error states, and large display fonts. |
| APP-05 | Sudoku/Chess | Touch all controls; Sudoku grid/borders visible; Chess modal blocks board taps and CANCEL/NO preserves game. |
| APP-06 | Snake/Tetris | Buttons and swipes; collision/game-over sound; GAME OVER overlay; high-score persistence after reboot and reset only by Factory Reset. |
| APP-07 | Flashcards/Utilities | SD deck load, pagination, calculator/converter/alarm/backup/system monitor happy and failure paths. |

## Long-duration and fault-injection tests

1. Run each display-heavy app for two hours on battery; compare battery drain
   and ensure no excessive full refresh/flicker.
2. Alternate Weather, Reader, Photos, and Files for 100 cycles; log heap,
   PSRAM, and largest free block after each 10 cycles.
3. Reboot during Wi-Fi reconnect, AP provisioning, Reader cache creation, and
   an SD write. Verify the next boot recovers without corrupting the main data.
4. Test low SD space, read-only/failed SD mount, no PSRAM, no Wi-Fi, captive
   portal, server timeout, and malformed input files.

## Evidence to attach to each release

- PlatformIO build output and firmware SHA/size.
- Serial logs for all failures and the Reader/heap soak.
- Photos of portrait and landscape screens, charging header, sleep screen,
  dialogs, and all game over states.
- Completed checklist with device revision, SD-card model, and router details.
