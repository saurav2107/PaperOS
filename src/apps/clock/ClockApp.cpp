#include "apps/clock/ClockApp.h"
#include <math.h>
#include <M5Unified.h>
#include "app/AppContext.h"
#include "services/Services.h"
#include "ui/Chrome.h"
#include "ui/Theme.h"

namespace {
ui::ChromeOptions clockChrome() {
  ui::ChromeOptions options;
  options.showBack = false;
  options.showHome = true;
  return options;
}

int clockTop(bool fullScreen) { return fullScreen ? 0 : ui::Chrome::headerHeight(); }
int clockBottom(bool fullScreen) { return fullScreen ? M5.Display.height() : ui::Chrome::footerTop(); }
void drawClockHeader(AppContext& context, bool fullScreen) { if (!fullScreen) ui::Chrome::drawHeader(context, "Clock"); }
void drawClockFooter(bool fullScreen) { if (!fullScreen) ui::Chrome::drawFooter(clockChrome()); }

// 5 x 7 dot-matrix numerals keep the clock exceptionally legible on e-paper:
// no thin diagonals, no anti-aliasing dependence, and no dense fill area.
constexpr uint8_t kDotDigits[10][7] = {
  {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
  {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}, {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E},
  {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, {0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E},
  {0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E}, {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
  {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, {0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E}
};

void drawDotDigit(uint8_t digit, int x, int y, int cell, int radius) {
  for (int row = 0; row < 7; ++row) for (int col = 0; col < 5; ++col)
    if (kDotDigits[digit][row] & (1 << (4 - col)))
      M5.Display.fillRoundRect(x + col * cell, y + row * cell, radius * 2, radius * 2, radius / 2, TFT_BLACK);
}
}

bool ClockApp::onStart(AppContext& context) {
  // DisplayService composes landscape with the user's 180-degree preference.
  context.display.setLandscape();

  fullScreen_ = false; pendingFaceChange_ = false; lastTapMs_ = 0; lastMinute_ = -1;
  lastWidth_ = M5.Display.width();
  draw(context, true); // Force full redraw on launch
  return true;
}

void ClockApp::onStop(AppContext& context) {
  context.display.setPortrait();
  fullScreen_ = false; pendingFaceChange_ = false; lastWidth_ = 0;
}

void ClockApp::drawDigit(int x, int y, int w, int h, char digit, bool darkTop) const {
  // Draw card background
  M5.Display.fillRoundRect(x, y, w, h, 8, TFT_BLACK);
  
  int margin = 3;
  int halfH = h / 2;

  // Top and bottom card leaves
  M5.Display.fillRect(x + margin, y + margin, w - (margin * 2), halfH - margin, darkTop ? TFT_DARKGREY : TFT_WHITE);
  M5.Display.fillRect(x + margin, y + halfH, w - (margin * 2), halfH - margin, TFT_WHITE);
  M5.Display.drawFastHLine(x + margin, y + halfH, w - (margin * 2), TFT_BLACK);
  
  // Dynamic hinge pins
  int pinRadius = std::max(2, w / 40);
  M5.Display.fillCircle(x + margin + 1, y + halfH, pinRadius, TFT_BLACK);
  M5.Display.fillCircle(x + w - margin - 1, y + halfH, pinRadius, TFT_BLACK);
  
  // Dynamic font sizing according to card height
  M5.Display.setTextDatum(MC_DATUM);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  
  int textSize = std::min(18, std::max(1, h / 35) * 3);
  M5.Display.setTextSize(textSize); 
  M5.Display.drawString(String(digit), x + w / 2, y + halfH);
  M5.Display.setTextDatum(TL_DATUM);
}

void ClockApp::drawColon(bool show, int centerX, int yTop, int cardHeight) {
  uint32_t color = show ? TFT_BLACK : TFT_WHITE;
  int topDotY = yTop + (cardHeight / 3);
  int bottomDotY = yTop + (cardHeight * 2 / 3);
  int dotRadius = std::max(3, cardHeight / 35);
  
  M5.Display.fillCircle(centerX, topDotY, dotRadius, color);
  M5.Display.fillCircle(centerX, bottomDotY, dotRadius, color);
}

void ClockApp::drawDigital(AppContext& context, const tm& local) {
  const int w = M5.Display.width();
  const int top = clockTop(fullScreen_);
  const int bottom = clockBottom(fullScreen_);
  char timeText[16], dateText[36];
  context.time.formatTime(local, timeText, sizeof(timeText));
  strftime(dateText, sizeof(dateText), "%A, %d %B", &local);
  M5.Display.fillScreen(TFT_WHITE); drawClockHeader(context, fullScreen_);
  M5.Display.setTextDatum(MC_DATUM); M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.drawString("MINIMAL DIGITAL", w / 2, top + 30);
  M5.Display.setFont(nullptr); M5.Display.setTextSize(16); M5.Display.drawString(timeText, w / 2, top + (bottom - top) / 2 - 18);
  M5.Display.setTextSize(1);
  M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.drawString(dateText, w / 2, bottom - 66);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.drawString(String("OUTDOOR ") + context.temperature.lastFormatted(), w / 2, bottom - 34);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr); drawClockFooter(fullScreen_);
}

void ClockApp::drawAnalog(AppContext& context, const tm& local) {
  const int w = M5.Display.width();
  const int top = clockTop(fullScreen_);
  const int bottom = clockBottom(fullScreen_);
  const int cx = w / 2, cy = top + (bottom - top) / 2 + 2;
  const int r = std::min((bottom - top) / 2 - 14, w / 3 - 12);
  M5.Display.fillScreen(TFT_WHITE); drawClockHeader(context, fullScreen_);
  // A dedicated, high-contrast dial.  The original face used thin ticks and
  // appeared sparse; this has a double bezel, readable numerals and hands
  // whose weight is proportional to the dial diameter.
  for (int stroke = 0; stroke < 3; ++stroke) M5.Display.drawCircle(cx, cy, r - stroke, TFT_BLACK);
  M5.Display.drawCircle(cx, cy, r - 11, TFT_BLACK);
  for (int i = 0; i < 12; ++i) {
    const float a = i * PI / 6.0f - PI / 2.0f;
    const bool cardinal = i % 3 == 0;
    const int outerX = cx + cosf(a) * (r - 17), outerY = cy + sinf(a) * (r - 17);
    const int innerX = cx + cosf(a) * (r - (cardinal ? 35 : 27)), innerY = cy + sinf(a) * (r - (cardinal ? 35 : 27));
    for (int offset = cardinal ? -2 : -1; offset <= (cardinal ? 2 : 1); ++offset)
      M5.Display.drawLine(innerX + offset, innerY, outerX + offset, outerY, TFT_BLACK);
  }
  M5.Display.setTextDatum(MC_DATUM); M5.Display.setFont(&fonts::FreeSans12pt7b);
  for (int value = 1; value <= 12; ++value) {
    const float a = value * PI / 6.0f - PI / 2.0f;
    const int nx = cx + static_cast<int>(cosf(a) * (r - 58));
    const int ny = cy + static_cast<int>(sinf(a) * (r - 58));
    M5.Display.drawString(String(value), nx, ny);
  }
  const float minute = local.tm_min * PI / 30.0f - PI / 2.0f;
  const float hour = ((local.tm_hour % 12) + local.tm_min / 60.0f) * PI / 6.0f - PI / 2.0f;
  const int hourX = cx + cosf(hour) * (r * 0.43f), hourY = cy + sinf(hour) * (r * 0.43f);
  const int minuteX = cx + cosf(minute) * (r * 0.68f), minuteY = cy + sinf(minute) * (r * 0.68f);
  const auto thickHand = [](int x0, int y0, int x1, int y1, int width) {
    for (int offset = -width / 2; offset <= width / 2; ++offset) {
      M5.Display.drawLine(x0 + offset, y0, x1 + offset, y1, TFT_BLACK);
      M5.Display.drawLine(x0, y0 + offset, x1, y1 + offset, TFT_BLACK);
    }
  };
  thickHand(cx, cy, hourX, hourY, 7);
  thickHand(cx, cy, minuteX, minuteY, 4);
  M5.Display.fillCircle(cx, cy, 9, TFT_BLACK); M5.Display.fillCircle(cx, cy, 3, TFT_WHITE);
  char date[20]; strftime(date, sizeof(date), "%a, %d %b", &local);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.drawString(date, cx, cy - r / 2 + 12);
  M5.Display.fillRoundRect(cx - 74, cy + r / 3, 148, 36, 7, TFT_BLACK);
  M5.Display.fillRoundRect(cx - 71, cy + r / 3 + 3, 142, 30, 5, TFT_WHITE);
  M5.Display.drawString(String("OUTDOOR ") + context.temperature.lastFormatted(), cx, cy + r / 3 + 19);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr); drawClockFooter(fullScreen_);
}

void ClockApp::drawPanel(AppContext& context, const tm& local) {
  const int w = M5.Display.width(), top = clockTop(fullScreen_), bottom = clockBottom(fullScreen_);
  const int margin = 28, panelY = top + 38, panelH = bottom - panelY - 26, splitX = w * 3 / 5;
  char timeText[16], day[16], date[32];
  context.time.formatTime(local, timeText, sizeof(timeText));
  strftime(day, sizeof(day), "%A", &local); strftime(date, sizeof(date), "%d %B %Y", &local);
  M5.Display.fillScreen(TFT_WHITE); drawClockHeader(context, fullScreen_);
  M5.Display.fillRoundRect(margin, panelY, w - margin * 2, panelH, 12, TFT_BLACK);
  M5.Display.fillRoundRect(margin + 3, panelY + 3, w - margin * 2 - 6, panelH - 6, 9, TFT_WHITE);
  M5.Display.fillRect(splitX, panelY + 24, 3, panelH - 48, TFT_BLACK);
  M5.Display.setTextDatum(MC_DATUM); M5.Display.setFont(&fonts::FreeSansBold24pt7b);
  M5.Display.drawString(timeText, margin + (splitX - margin) / 2, panelY + panelH / 2 - 4);
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.drawString(context.time.use24Hour() ? "24-HOUR TIME" : "12-HOUR TIME", margin + (splitX - margin) / 2, panelY + panelH / 2 + 58);
  const int infoX = splitX + (w - margin - splitX) / 2;
  M5.Display.setFont(&fonts::FreeSans18pt7b); M5.Display.drawString(day, infoX, panelY + 78);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.drawString(date, infoX, panelY + 118);
  M5.Display.drawFastHLine(splitX + 32, panelY + 151, w - margin - splitX - 64, TFT_BLACK);
  M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.drawString("OUTDOOR", infoX, panelY + 203);
  M5.Display.setFont(&fonts::FreeSansBold24pt7b); M5.Display.drawString(context.temperature.lastFormatted(), infoX, panelY + 261);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.drawString("TAP TO CHANGE FACE", infoX, panelY + panelH - 42);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr); drawClockFooter(fullScreen_);
}

void ClockApp::drawDateTemperature(AppContext& context, const tm& local) {
  const int w = M5.Display.width();
  const int top = clockTop(fullScreen_);
  const int bottom = clockBottom(fullScreen_);
  char weekday[16], date[28], timeText[16];
  strftime(weekday, sizeof(weekday), "%A", &local); strftime(date, sizeof(date), "%d %B %Y", &local); context.time.formatTime(local, timeText, sizeof(timeText));
  M5.Display.fillScreen(TFT_WHITE); drawClockHeader(context, fullScreen_);
  M5.Display.setTextDatum(MC_DATUM); M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.drawString(weekday, w / 2, top + 48);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.drawString(date, w / 2, top + 82);
  M5.Display.setFont(&fonts::FreeSansBold24pt7b); M5.Display.drawString(timeText, w / 2, top + 165);
  M5.Display.drawRoundRect(w / 2 - 135, bottom - 160, 270, 88, 10, TFT_BLACK);
  M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.drawString("OUTDOOR", w / 2, bottom - 140);
  M5.Display.setFont(&fonts::FreeSansBold24pt7b); M5.Display.drawString(context.temperature.lastFormatted(), w / 2, bottom - 100);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr); drawClockFooter(fullScreen_);
}

void ClockApp::drawDotted(AppContext& context, const tm& local) {
  const int width = M5.Display.width();
  const int top = clockTop(fullScreen_);
  const int bottom = clockBottom(fullScreen_);
  const int cell = std::max(17, std::min(25, width / 40));
  const int dotRadius = std::max(5, cell / 3);
  const int digitWidth = cell * 5;
  const int digitGap = cell;
  const int colonWidth = cell * 2;
  const int total = digitWidth * 4 + digitGap * 2 + colonWidth + cell * 2;
  const int startX = (width - total) / 2;
  const int glyphHeight = cell * 7;
  const int y = top + (bottom - top - glyphHeight) / 2 - 14;
  char digits[6];
  strftime(digits, sizeof(digits), context.time.use24Hour() ? "%H%M" : "%I%M", &local);

  M5.Display.fillScreen(TFT_WHITE);
  drawClockHeader(context, fullScreen_);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString("DOTTED CLOCK", width / 2, y - 28);
  int x = startX;
  for (int index = 0; index < 4; ++index) {
    drawDotDigit(static_cast<uint8_t>(digits[index] - '0'), x, y, cell, dotRadius);
    x += digitWidth + digitGap;
    if (index == 1) {
      M5.Display.fillCircle(x + cell / 2, y + cell * 2, dotRadius, TFT_BLACK);
      M5.Display.fillCircle(x + cell / 2, y + cell * 5, dotRadius, TFT_BLACK);
      x += colonWidth + cell;
    }
  }
  char date[32]; strftime(date, sizeof(date), "%a, %d %b %Y", &local);
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.drawString(date, width / 2, bottom - 66);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.drawString(String("OUTDOOR ") + context.temperature.lastFormatted(), width / 2, bottom - 34);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
  drawClockFooter(fullScreen_);
}

// Values come from the shared RTC/local-time and cached outdoor services.
// A forced draw lays out chrome and labels; minute ticks repaint changed
// tiles only. No network request or animation runs in the display loop.
void ClockApp::drawDarkFlip(AppContext& context, const tm& local, bool force) {
  const int width = M5.Display.width(), top = clockTop(fullScreen_);
  const int height = clockBottom(fullScreen_) - top;
  const int margin = 24, gap = 14, labelH = 32;
  const int availableW = width - 2 * margin;
  const int pairW = (availableW - gap) / 2;
  const int tripleW = (availableW - 2 * gap) / 3;
  const int firstY = top + 22 + labelH;
  const int largeH = height * 42 / 100;
  const int secondY = firstY + largeH + 20 + labelH;
  const int dateH = height * 23 / 100;
  const int thirdY = secondY + dateH + 20 + labelH;
  const int weatherH = clockBottom(fullScreen_) - thirdY - 42;
  char hours[3], minutes[3], month[3], date[3], day[12];
  strftime(hours, sizeof(hours), context.time.use24Hour() ? "%H" : "%I", &local);
  strftime(minutes, sizeof(minutes), "%M", &local);
  strftime(month, sizeof(month), "%m", &local);
  strftime(date, sizeof(date), "%d", &local);
  strftime(day, sizeof(day), "%a", &local);
  const float humidity = context.temperature.lastHumidity();
  String temperature = context.temperature.lastFormatted(1);
  // Draw the degree mark geometrically; bundled fonts lack this glyph.
  temperature.replace("oC", " C"); temperature.replace("oF", " F");
  const String values[] = {hours, minutes, month, date, day, temperature,
    isfinite(humidity) ? String(humidity, 0) + "%" : String("--")};
  const char* labels[] = {context.time.use24Hour() ? "HOUR" : (local.tm_hour < 12 ? "HOUR AM" : "HOUR PM"),
    "MINUTES", "MONTH", "DATE", "DAY", "TEMP", "HUM"};
  if (force) {
    M5.Display.fillRect(0, top, width, height, TFT_BLACK);
    drawClockHeader(context, fullScreen_); drawClockFooter(fullScreen_);
    M5.Display.setTextSize(1); M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.setTextDatum(MC_DATUM); M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.drawString("Cached outdoor weather", width / 2, clockBottom(fullScreen_) - 19);
  }
  for (int i = 0; i < 7; ++i) {
    // Refresh the hour label at noon/midnight even with identical 12h digits.
    if (!force && values[i] == darkTileValues_[i] && (i != 0 || darkHour_ == local.tm_hour)) continue;
    const int w = i < 2 || i >= 5 ? pairW : tripleW;
    const int x = margin + (i < 2 ? i : i < 5 ? i - 2 : i - 5) * (w + gap);
    const int y = i < 2 ? firstY : i < 5 ? secondY : thirdY;
    const int h = i < 2 ? largeH : i < 5 ? dateH : weatherH;
    M5.Display.fillRect(x, y - labelH, w, labelH, TFT_BLACK);
    M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK); M5.Display.setTextDatum(ML_DATUM);
    M5.Display.drawString(labels[i], x, y - labelH / 2);
    const uint16_t tileColor = TFT_DARKGREY;
    M5.Display.fillRoundRect(x, y, w, h, 12, tileColor);
    M5.Display.setFont(&fonts::FreeSansBold24pt7b); M5.Display.setTextSize(1);
    const float scale = std::min((w - 22.0f) / std::max(1, M5.Display.textWidth(values[i])),
                                (h * (i < 2 ? 0.58f : 0.52f)) / M5.Display.fontHeight());
    M5.Display.setTextSize(scale); M5.Display.setTextColor(TFT_WHITE, tileColor);
    M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString(values[i], x + w / 2, y + h / 2);
    if (i == 5 && context.temperature.hasLastTemperature()) {
      const int totalW = M5.Display.textWidth(values[i]);
      const int suffixW = M5.Display.textWidth(values[i].substring(values[i].length() - 2));
      M5.Display.drawCircle(x + w / 2 + totalW / 2 - suffixW + 3, y + h / 2 - M5.Display.fontHeight() / 4,
                            std::max(2, static_cast<int>(scale * 3)), TFT_WHITE);
    }
    if (i < 2) M5.Display.fillRect(x, y + h / 2, w, ui::Theme::Border, TFT_BLACK);
    darkTileValues_[i] = values[i];
    if (!force) M5.Display.display(x, y - labelH, w, h + labelH);
  }
  darkHour_ = local.tm_hour;
  M5.Display.setTextSize(1); M5.Display.setFont(nullptr);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
}

void ClockApp::draw(AppContext& context, bool force) {
  // The tall reference face owns portrait orientation; all other faces keep
  // their existing landscape layout, including the user's 180-degree flip.
  const bool portrait = context.clockFace.face() == ClockFace::DarkFlip;
  if (portrait != (M5.Display.height() > M5.Display.width())) {
    if (portrait) context.display.setPortrait(); else context.display.setLandscape();
    force = true;
  }
  tm local{};
  if (!context.time.localTime(local)) {
    if (force) {
        M5.Display.fillScreen(TFT_WHITE); 
        drawClockHeader(context, fullScreen_);
        M5.Display.setTextSize(2); 
        M5.Display.drawString("Waiting for time sync...", 86, M5.Display.height() / 2); 
        drawClockFooter(fullScreen_);
    }
    return;
  }

  int screenW = M5.Display.width();
  int screenH = M5.Display.height();
  
  // Chrome Header & Footer reserved heights
  const int headerH = clockTop(fullScreen_);
  const int footerTop = clockBottom(fullScreen_);
  int availableH = footerTop - headerH;

  if (context.clockFace.face() == ClockFace::DarkFlip) { drawDarkFlip(context, local, force); lastMinute_ = local.tm_min; lastWidth_ = screenW; return; }

  if (context.clockFace.face() == ClockFace::Digital) { drawDigital(context, local); lastMinute_ = local.tm_min; lastWidth_ = screenW; return; }
  if (context.clockFace.face() == ClockFace::Analog) { drawAnalog(context, local); lastMinute_ = local.tm_min; lastWidth_ = screenW; return; }
  if (context.clockFace.face() == ClockFace::DateTemperature) { drawDateTemperature(context, local); lastMinute_ = local.tm_min; lastWidth_ = screenW; return; }
  if (context.clockFace.face() == ClockFace::Dotted) { drawDotted(context, local); lastMinute_ = local.tm_min; lastWidth_ = screenW; return; }
  if (context.clockFace.face() == ClockFace::Panel) { drawPanel(context, local); lastMinute_ = local.tm_min; lastWidth_ = screenW; return; }

  // Dynamically scale card dimensions and spacing relative to screen width
  int sideMargin = 20;
  int availableW = screenW - (sideMargin * 2);

  // Total horizontal units: 4 cards + 2 small gaps + 1 center gap = ~4.55 units
  int cardW = availableW / 4.55; 
  int gap = cardW * 0.10;         // 10% of card width
  int centerGap = cardW * 0.35;   // 35% of card width for the colon

  int totalWidth = (cardW * 4) + (gap * 2) + centerGap;
  int startX = (screenW - totalWidth) / 2;

  // Maximize height based on standard flip card aspect ratio (~1.4 - 1.5)
  int cardH = availableH * 0.58; 
  if (cardH > cardW * 1.5) {
      cardH = cardW * 1.5; 
  }

  int cardTop = headerH + (availableH - cardH) / 2 + 5; 

  char clockText[6]; 
  strftime(clockText, sizeof(clockText), context.time.use24Hour() ? "%H%M" : "%I%M", &local);

  // 1. FULL REDRAW
  if (force) {
      char date[32]; 
      strftime(date, sizeof(date), "%A, %d %B %Y", &local);

      M5.Display.fillScreen(TFT_WHITE);
      drawClockHeader(context, fullScreen_);
      
      M5.Display.setTextDatum(MC_DATUM); 
      M5.Display.setTextSize(2);
      M5.Display.drawString("FLIP CLOCK", screenW / 2, cardTop - 35);
      M5.Display.setTextSize(1); 
      M5.Display.drawString(date, screenW / 2, cardTop - 18);
      
      int colonX = startX + (cardW * 2) + gap + (centerGap / 2);
      drawColon(true, colonX, cardTop, cardH);

      M5.Display.drawString(String("OUTDOOR ") + context.temperature.lastFormatted(), screenW / 2, cardTop + cardH + 20);
      M5.Display.setTextDatum(TL_DATUM);
      
      drawClockFooter(fullScreen_);
  }

  // 2. PARTIAL REDRAW (The Digits)
  for (int i = 0; i < 4; ++i) {
    int xOffset = startX + (i * cardW);
    if (i >= 1) xOffset += gap;
    if (i >= 2) xOffset += centerGap;
    if (i >= 3) xOffset += gap;

    bool darkTop = (local.tm_min + i) % 2 == 0;
    drawDigit(xOffset, cardTop, cardW, cardH, clockText[i], darkTop);
  }

  lastMinute_ = local.tm_min;
  lastWidth_ = screenW;
}

void ClockApp::onTick(AppContext& context, uint32_t now) {
  const auto& touch = M5.Touch.getDetail();
  if (touch.wasPressed()) {
    if (!fullScreen_ && ui::Chrome::hitTestFooter(touch.x, touch.y, clockChrome()) != ui::FooterAction::None) {
      if (navigator_) navigator_(AppId::Launcher);
      return;
    }
    // The Photos-style double tap takes priority over a face change. In
    // immersive mode it is also the explicit, discoverable way back to the
    // shared header and footer.
    if (lastTapMs_ && now - lastTapMs_ < 380UL) {
      fullScreen_ = !fullScreen_;
      pendingFaceChange_ = false;
      lastTapMs_ = 0;
      draw(context, true);
      return;
    }
    lastTapMs_ = now;
    pendingFaceChange_ = !fullScreen_;
    return;
  }

  // Defer the ordinary content tap just long enough to distinguish it from a
  // double tap. This keeps the original tap-to-cycle clock face interaction.
  if (pendingFaceChange_ && now - lastTapMs_ >= 380UL) {
    pendingFaceChange_ = false;
    context.clockFace.next();
    draw(context, true);
  }

  tm local{};
  if (context.time.localTime(local)) {
    int currentWidth = M5.Display.width();
    
    if (currentWidth != lastWidth_) {
      lastWidth_ = currentWidth;
      draw(context, true); 
    } else if (local.tm_min != lastMinute_) {
      lastMinute_ = local.tm_min;
      draw(context, context.clockFace.face() != ClockFace::Flip && context.clockFace.face() != ClockFace::DarkFlip); 
    }
  }
}
