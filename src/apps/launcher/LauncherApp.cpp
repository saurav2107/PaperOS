#include "apps/launcher/LauncherApp.h"
#include <M5Unified.h>
#include "app/AppContext.h"
#include "config/Location.h"
#include "services/Services.h"
#include "ui/Icons.h"
#include "ui/Chrome.h"
#include "ui/Popup.h"
#include "ui/Theme.h"

namespace {
struct Tile { AppId id; const char* label; };
constexpr int kGamesTile = 9;
constexpr int kTileLeft = 14;
constexpr int kTileWidth = 160;
constexpr int kTileHeight = 144;
constexpr int kTileTop = 138;
constexpr int kTileRowStep = 164;
constexpr Tile tiles[] = {
  {AppId::Weather,"Weather"},{AppId::Clock,"Clock"},{AppId::Pomodoro,"Focus"},
  {AppId::Notes,"Notes"},{AppId::Tasks,"To Do"},{AppId::Calendar,"Calendar"},
  {AppId::Reader,"eReader"},{AppId::Images,"Photos"},{AppId::Writing,"Sketch"},
  {AppId::Chess,"Games"},{AppId::Flashcards,"Flashcards"},{AppId::Dashboard,"Utilities"},
};
constexpr size_t kTileCount = sizeof(tiles) / sizeof(tiles[0]);
void icon(AppId id, int x, int y) {
  using ui::Icon;
  Icon material = Icon::Home;
  if (id == AppId::Weather) material = Icon::Weather;
  else if (id == AppId::Clock) material = Icon::Clock;
  else if (id == AppId::Calendar) material = Icon::Calendar;
  else if (id == AppId::Reader) material = Icon::Book;
  else if (id == AppId::Images) material = Icon::Image;
  else if (id == AppId::Pomodoro) material = Icon::Timer;
  else if (id == AppId::Notes) material = Icon::Note;
  else if (id == AppId::Writing) material = Icon::Sketch;
  else if (id == AppId::Tasks) material = Icon::Todo;
  else if (id == AppId::Chess) material = Icon::Games;
  else if (id == AppId::Files) material = Icon::Folder;
  else if (id == AppId::Flashcards) material = Icon::Flashcards;
  else if (id == AppId::Dashboard) material = Icon::System;
  else if (id == AppId::Settings) material = Icon::Settings;
  ui::drawIcon(material, x, y, 56);
}

// The calendar glyph is deliberately completed here rather than baked into
// Icons.cpp: only the launcher needs a live date, while all other uses retain
// the reusable Material-style Calendar icon.
void drawCalendarDay(AppContext& context, int x, int y) {
  tm today{};
  if (!context.time.localTime(today)) return;

  char day[3];
  snprintf(day, sizeof(day), "%d", today.tm_mday);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.setTextDatum(MC_DATUM);
  // This is the white page area of the 56px calendar glyph.
  M5.Display.drawString(day, x, y + 7);
  M5.Display.setTextDatum(TL_DATUM);
  M5.Display.setFont(nullptr);
}

// Filled outer/inner shapes give e-paper a much cleaner 3-pixel frame than
// three overlaid anti-aliased strokes, especially at the lower rounded edges.
void tileFrame(int x, int y) {
  ui::Theme::drawFrame(x, y, kTileWidth, kTileHeight, 14);
}

void popupActionButton(int x, int y, ui::Icon symbol, const char* label) {
  // Same filled 3px outer/inset frame as Pomodoro and the shared chrome.
  ui::Theme::drawButtonFrame(x, y, 140, 60);
  ui::drawIcon(symbol, x + 25, y + 30, 28);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(ML_DATUM);
  M5.Display.drawString(label, x + 48, y + 31);
  M5.Display.setTextDatum(TL_DATUM);
  M5.Display.setFont(nullptr);
}
}

bool LauncherApp::onStart(AppContext& context) { draw(context); return true; }
void LauncherApp::draw(AppContext& context) {
  // A true modal does not redraw the launcher underneath it.  This gives the
  // power choices full focus and prevents background controls looking active.
  if (powerOffConfirmation_) { drawPowerOffPopup(); return; }
  if (gamesOpen_) { drawGamesPopup(context); return; }
  if (utilitiesOpen_) { drawUtilitiesPopup(context); return; }
  M5.Display.fillScreen(TFT_WHITE);
  ui::Chrome::drawHeader(context, "Paper OS", false);
  M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString("APP DRAWER", M5.Display.width() / 2, 112);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
  for (size_t i=0;i<kTileCount;++i) {
    const int row=i/3, col=i%3, x=kTileLeft+col*176, y=kTileTop+row*kTileRowStep;
    tileFrame(x, y);
    icon(tiles[i].id,x+80,y+45);
    if (tiles[i].id == AppId::Calendar) drawCalendarDay(context, x + 80, y + 45);
    M5.Display.setFont(&fonts::FreeSansBold9pt7b); M5.Display.setTextDatum(MC_DATUM);
    M5.Display.drawString(tiles[i].label, x + kTileWidth / 2, y + 107);
    M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
  }
  ui::ChromeOptions footer;
  footer.showBack = false;
  footer.showPowerOff = true;
  footer.showHome = true;
  footer.showSettings = true;
  ui::Chrome::drawFooter(footer);
}

void LauncherApp::drawUtilitiesPopup(AppContext&) {
  M5.Display.fillScreen(TFT_WHITE);
  // This intentionally mirrors the Games modal: same frame, title rail,
  // close control, two-column cards, 3px borders, spacing and typography.
  ui::Popup::drawFrame(24, 148, 492, 700, "UTILITIES");
  // HA Home remains compiled and registered, but is intentionally hidden
  // from the utility drawer until it is ready to be exposed again.
  const char* names[] = {"ALARM", "CALCULATOR", "CONVERTER", "BACKUP", "FILES"};
  const ui::Icon icons[] = {ui::Icon::Clock, ui::Icon::Calculator, ui::Icon::System, ui::Icon::Folder, ui::Icon::Folder};
  for (int i = 0; i < 5; ++i) {
    const int row=i/2, col=i%2, x=52+col*226, y=240+row*132;
    ui::Theme::drawFrame(x, y, 202, 112, 12);
    ui::drawIcon(icons[i],x+101,y+35,38);
    M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString(names[i],x+101,y+86);
  }
  M5.Display.setTextDatum(TL_DATUM);
  M5.Display.setFont(nullptr);
}

void LauncherApp::drawPowerOffPopup() {
  constexpr int x = 30, y = 300, width = 480, height = 340;
  M5.Display.fillScreen(TFT_WHITE);
  ui::Popup::drawFrame(x, y, width, height, "POWER OPTIONS");
  M5.Display.setTextDatum(MC_DATUM);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.drawString("Choose what to do with this device", 270, y + 106);
  M5.Display.drawFastHLine(x + 30, y + 132, width - 60, TFT_BLACK);
  M5.Display.setTextDatum(TL_DATUM);
  popupActionButton(50, y + 156, ui::Icon::Sleep, "SLEEP");
  popupActionButton(200, y + 156, ui::Icon::Power, "OFF");
  popupActionButton(350, y + 156, ui::Icon::Close, "CANCEL");
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString("Sleep preservs your current session", 270, y + 260);
  M5.Display.setTextDatum(TL_DATUM);
  M5.Display.setFont(nullptr);
}

void LauncherApp::drawGameIcon(int x, int y, uint8_t game) const {
  if (game == 0) {  // Chess board
    M5.Display.fillRect(x-26,y-26,52,52,TFT_BLACK);
    for (int row=0; row<4; ++row) for (int col=0; col<4; ++col)
      if ((row + col) % 2 == 0) M5.Display.fillRect(x-26+col*13,y-26+row*13,13,13,TFT_WHITE);
    // A bold white knight silhouette keeps the board readable at icon size.
    M5.Display.fillCircle(x+4,y-11,7,TFT_WHITE);
    M5.Display.fillTriangle(x-5,y-9,x+10,y-9,x+13,y+12,TFT_WHITE);
    M5.Display.fillRect(x-13,y+11,28,8,TFT_WHITE);
    M5.Display.fillRect(x+6,y-13,3,3,TFT_BLACK);
  } else if (game == 1) {  // Sudoku grid
    M5.Display.fillRect(x-26,y-26,52,52,TFT_BLACK);
    for (int p=-9; p<=9; p+=18) {
      M5.Display.drawFastVLine(x+p,y-23,46,TFT_WHITE); M5.Display.drawFastVLine(x+p+1,y-23,46,TFT_WHITE);
      M5.Display.drawFastHLine(x-23,y+p,46,TFT_WHITE); M5.Display.drawFastHLine(x-23,y+p+1,46,TFT_WHITE);
    }
    M5.Display.setTextColor(TFT_WHITE,TFT_BLACK); M5.Display.setTextSize(1);
    M5.Display.drawString("1",x-18,y-20); M5.Display.drawString("5",x-2,y-2); M5.Display.drawString("9",x+14,y+14);
    M5.Display.setTextColor(TFT_BLACK,TFT_WHITE);
  } else if (game == 2) {  // Snake
    M5.Display.fillRect(x-22,y+10,18,8,TFT_BLACK); M5.Display.fillRect(x-12,y-4,8,18,TFT_BLACK);
    M5.Display.fillRect(x-8,y-4,20,8,TFT_BLACK); M5.Display.fillRect(x+4,y-19,8,19,TFT_BLACK);
    M5.Display.fillCircle(x+8,y-18,4,TFT_BLACK);
  } else if (game == 3) {  // Tetris blocks
    M5.Display.fillRect(x-22,y-18,14,14,TFT_BLACK); M5.Display.fillRect(x-7,y-18,14,14,TFT_BLACK);
    M5.Display.fillRect(x+8,y-18,14,14,TFT_BLACK); M5.Display.fillRect(x+8,y-3,14,14,TFT_BLACK);
  } else {  // Minesweeper mine and flag
    M5.Display.fillCircle(x, y, 18, TFT_BLACK);
    for (int angle = 0; angle < 8; ++angle) {
      const float radians = angle * 0.785398f;
      const int dx = static_cast<int>(cosf(radians) * 25);
      const int dy = static_cast<int>(sinf(radians) * 25);
      M5.Display.drawLine(x, y, x + dx, y + dy, TFT_BLACK);
      M5.Display.drawLine(x + 1, y, x + dx + 1, y + dy, TFT_BLACK);
    }
    M5.Display.fillCircle(x - 6, y - 5, 3, TFT_WHITE);
    M5.Display.fillRect(x + 19, y - 23, 3, 43, TFT_BLACK);
    M5.Display.fillTriangle(x + 22, y - 21, x + 38, y - 14, x + 22, y - 7, TFT_BLACK);
  }
}

void LauncherApp::drawGamesPopup(AppContext& context) {
  // Shared 3px outer/inset frame: identical construction to launcher tiles
  // and action buttons, so the popup does not look like a separate UI system.
  M5.Display.fillScreen(TFT_WHITE);
  ui::Popup::drawFrame(24, 142, 492, 674, "GAMES");
  const char* names[] = {"CHESS", "SUDOKU", "SNAKE", "TETRIS", "MINESWEEPER"};
  for (int i = 0; i < 5; ++i) {
    // Theme::drawFrame draws a filled 3px outer/inner frame. Unlike a thin
    // outline, its entire bottom edge survives e-paper update waveforms.
    const int row = i / 2, col = i % 2;
    const int x = (i == 4) ? 169 : 52 + col * 226;
    const int y = 242 + row * 172;
    ui::Theme::drawFrame(x, y, 202, 140, 12);
    drawGameIcon(x + 101, y + 45, i);
    M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString(names[i], x + 101, y + 106);
    M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
  }
  (void)context;
}

void LauncherApp::showComingSoon(AppContext& context, const char* game) {
  M5.Display.fillRoundRect(82, 756, 376, 74, 10, TFT_WHITE);
  M5.Display.drawRoundRect(82, 756, 376, 74, 10, TFT_BLACK);
  M5.Display.setTextSize(1); M5.Display.drawString(String(game) + " IS PLANNED FOR A FUTURE UPDATE.", 92, 785);
  (void)context;
}

void LauncherApp::onTick(AppContext& context, uint32_t now) {
  (void)now;
  const auto& touch=M5.Touch.getDetail();
  if (touch.wasPressed() && powerOffConfirmation_) {
    if (ui::Popup::hitClose(touch.x, touch.y, 30, 300, 480)) { powerOffConfirmation_ = false; draw(context); return; }
    if (touch.y >= 456 && touch.y <= 516) {
      if (touch.x >= 50 && touch.x <= 190) {
        powerOffConfirmation_ = false;
        context.power.sleepNow(context.time.formattedLocalTime());
        return;
      }
      if (touch.x >= 200 && touch.x <= 340) { context.power.powerOff(); }
      if (touch.x >= 350 && touch.x <= 490) { powerOffConfirmation_ = false; draw(context); return; }
    }
    return;
  }
  if (touch.wasPressed() && gamesOpen_) {
    if (ui::Popup::hitClose(touch.x, touch.y, 24, 142, 492)) { gamesOpen_ = false; draw(context); return; }
    int game = -1;
    if (touch.y >= 242 && touch.y < 726) {
      const int row = (touch.y - 242) / 172;
      if (row < 2 && touch.x >= 52 && touch.x <= 480) game = row * 2 + (touch.x >= 278);
      else if (row == 2 && touch.x >= 169 && touch.x <= 371) game = 4;
    }
    if (game >= 0) {
      if (game == 0 && navigator_) { gamesOpen_ = false; navigator_(AppId::Chess); return; }
      if (game == 1 && navigator_) { gamesOpen_ = false; navigator_(AppId::Sudoku); return; }
      if (game == 2 && navigator_) { gamesOpen_ = false; navigator_(AppId::Snake); return; }
      if (game == 3 && navigator_) { gamesOpen_ = false; navigator_(AppId::Tetris); return; }
      if (game == 4 && navigator_) { gamesOpen_ = false; navigator_(AppId::Minesweeper); return; }
      static const char* const names[] = {"CHESS", "SUDOKU", "SNAKE", "TETRIS", "MINESWEEPER"}; showComingSoon(context, names[game]); return;
    }
    return;
  }
  if (touch.wasPressed() && utilitiesOpen_) {
    if (ui::Popup::hitClose(touch.x, touch.y, 24, 148, 492)) {
      utilitiesOpen_ = false; draw(context); return;
    }
    if (touch.x >= 52 && touch.x <= 480 && touch.y >= 240 && touch.y < 616 && navigator_) {
      const int col = touch.x >= 278, row = (touch.y - 240) / 132, item = row * 2 + col;
      constexpr AppId destinations[] = {AppId::Alarm, AppId::Calculator, AppId::Converter, AppId::BackupRestore, AppId::Files};
      if (item >= 0 && item < 5) { utilitiesOpen_ = false; navigator_(destinations[item]); }
    }
    return;
  }
  if (touch.wasPressed()) {
    ui::ChromeOptions footer; footer.showBack = false; footer.showPowerOff = true; footer.showHome = true; footer.showSettings = true;
    const auto action = ui::Chrome::hitTestFooter(touch.x, touch.y, footer);
    if (action == ui::FooterAction::Home) { draw(context); return; }
    if (action == ui::FooterAction::Settings && navigator_) { navigator_(AppId::Settings); return; }
    if (action == ui::FooterAction::PowerOff) { powerOffConfirmation_ = true; draw(context); return; }
  }
  if (touch.wasPressed() && navigator_) for(size_t i=0;i<kTileCount;++i) {
    const int row=i/3,col=i%3,x=kTileLeft+col*176,y=kTileTop+row*kTileRowStep;
    if(touch.x>=x&&touch.x<=x+kTileWidth&&touch.y>=y&&touch.y<=y+kTileHeight){
      if (static_cast<int>(i) == kGamesTile) { gamesOpen_ = true; drawGamesPopup(context); return; }
      if (tiles[i].id == AppId::Dashboard) { utilitiesOpen_ = true; drawUtilitiesPopup(context); return; }
      navigator_(tiles[i].id); return;
    }
  }
}
