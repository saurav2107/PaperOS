#include "ui/Chrome.h"
#include <M5Unified.h>
#include "app/AppContext.h"
#include "services/Services.h"
#include "ui/Icons.h"

namespace {
struct ChromeMetrics {
  int width;
  int height;
  int header;
  int footer;
  int footerTop;
  int margin;
  int gap;
  int buttonWidth;
  int buttonHeight;
  int buttonTop;
};

int clampInt(int value, int minimum, int maximum) { return value < minimum ? minimum : value > maximum ? maximum : value; }

ChromeMetrics metrics() {
  const int width = M5.Display.width(), height = M5.Display.height();
  const bool portrait = height >= width;
  const int header = portrait ? clampInt(width / 9, 54, 72) : clampInt(height / 9, 48, 60);
  // Header and footer are the shared visual frame of every app. Keeping them
  // identical in height makes portrait and landscape screens feel balanced.
  const int footer = header;
  const int margin = clampInt(width / 45, 8, 18);
  const int gap = clampInt(width / 45, 8, 18);
  const int buttonWidth = (width - margin * 2 - gap * 4) / 5;
  const int buttonHeight = clampInt(footer - 28, 38, 48);
  return {width, height, header, footer, height - footer, margin, gap, buttonWidth, buttonHeight, height - footer + (footer - buttonHeight) / 2};
}

void thickLine(int x0, int y0, int x1, int y1, uint16_t colour) {
  M5.Display.drawLine(x0, y0, x1, y1, colour);
  M5.Display.drawLine(x0 + 1, y0, x1 + 1, y1, colour);
  M5.Display.drawLine(x0 - 1, y0, x1 - 1, y1, colour);
}
void drawWifiIcon(int x, int y, bool connected, int32_t rssi, uint16_t foreground, uint16_t background) {
  int strength = 0;
  if (connected) strength = rssi >= -55 ? 4 : rssi >= -67 ? 3 : rssi >= -75 ? 2 : 1;
  // Solid concentric arc bands form a recognisable Wi-Fi icon at PaperS3's
  // native resolution. The icon is intentionally monochrome for e-paper.
  if (strength >= 4) M5.Display.drawArc(x, y + 5, 14, 16, 225, 315, foreground);
  if (strength >= 3) M5.Display.drawArc(x, y + 5, 10, 12, 225, 315, foreground);
  if (strength >= 2) M5.Display.drawArc(x, y + 5, 6, 8, 225, 315, foreground);
  if (strength >= 1) M5.Display.fillCircle(x, y + 9, 3, foreground);
  if (!connected) {
    // Draw a conventional Wi-Fi glyph underneath the X, then knock out and
    // redraw the cross so it remains visible over solid black arc bands.
    M5.Display.drawArc(x, y + 5, 14, 16, 225, 315, foreground);
    M5.Display.drawArc(x, y + 5, 8, 10, 225, 315, foreground);
    M5.Display.fillCircle(x, y + 9, 3, foreground);
    // A single compact slash matches the conventional Wi-Fi-off symbol and
    // does not overwhelm the small status area.
    thickLine(x - 10, y + 13, x + 11, y - 8, background);
    thickLine(x - 10, y + 13, x + 11, y - 8, foreground);
  }
}
void drawBattery(int x, int y, uint8_t level, bool charging, uint16_t foreground, uint16_t background) {
  // Solid battery silhouette with a white inset keeps the small status icon
  // aligned with the filled icon language used by launcher and dashboard.
  M5.Display.fillRoundRect(x, y, 29, 20, 4, foreground);
  M5.Display.fillRect(x + 29, y + 6, 4, 8, foreground);
  M5.Display.fillRect(x + 3, y + 3, 23, 14, background);
  const int fill = 23 * level / 100;
  if (fill) M5.Display.fillRect(x + 3, y + 3, fill, 14, foreground);
  if (charging) {
    // Knock a small lightning bolt out of the charge fill. Unlike a thin
    // divider, this remains recognisable at the header's 20px icon height.
    M5.Display.fillTriangle(x + 16, y + 3, x + 10, y + 11, x + 15, y + 11, background);
    M5.Display.fillTriangle(x + 13, y + 17, x + 20, y + 8, x + 15, y + 8, background);
  }
}
void button(int x, int y, int w, int h, const char* label) {
  // A filled outer frame survives e-paper partial updates more cleanly than
  // a single-pixel outline, especially along the lower rounded corners.
  M5.Display.fillRoundRect(x, y, w, h, 5, TFT_BLACK);
  M5.Display.fillRoundRect(x + 3, y + 3, w - 6, h - 6, 3, TFT_WHITE);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString(label, x + w / 2, y + h / 2 + 1);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}
void homeButton(int x, int y, int w, int h) {
  button(x, y, w+5, h, "");
  const int centerY = y + h / 2;
  ui::drawIcon(ui::Icon::Home, x + w / 4, centerY, clampInt(h / 2 - 3, 18, 20));
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextSize(1);
  M5.Display.setTextDatum(ML_DATUM); M5.Display.drawString("HOME", x + w / 2 - 4, centerY);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}
void settingsButton(int x, int y, int w, int h) {
  button(x, y, w, h, "");
  ui::drawIcon(ui::Icon::Settings, x + w / 2, y + h / 2, clampInt(h / 2 - 3, 18, 20));
}
void powerButton(int x, int y, int w, int h) {
  button(x, y, w, h, "");
  const int centerY = y + h / 2;
  ui::drawIcon(ui::Icon::Power, x + w / 4, centerY, 18);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextSize(1);
  M5.Display.setTextDatum(ML_DATUM); M5.Display.drawString("OFF", x + w / 2 - 4, centerY);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}

struct FooterItem { ui::FooterAction action; const char* label; };
uint8_t footerItems(const ui::ChromeOptions& options, FooterItem (&items)[5]) {
  uint8_t count = 0;
  const auto add = [&](ui::FooterAction action, const char* label) { if (count < 5) items[count++] = {action, label}; };
  if (options.showPrevious) add(ui::FooterAction::Previous, "PREV");
  // When all reader-style controls are enabled, navigation must read in the
  // natural left-to-right order: PREV / BACK (to the list) / NEXT.
  if (options.showBack) add(ui::FooterAction::Back, "BACK");
  if (options.showUp) add(ui::FooterAction::Up, "UP");
  if (options.showPowerOff) add(ui::FooterAction::PowerOff, "OFF");
  else if (options.showClose) add(ui::FooterAction::Close, "CLOSE");
  else if (options.showConfirm) add(ui::FooterAction::Confirm, "CONFIRM");
  else if (options.showHome) add(ui::FooterAction::Home, "HOME");
  if (options.showDown) add(ui::FooterAction::Down, "DOWN");
  if (options.showNext) add(ui::FooterAction::Next, "NEXT");
  if (options.showSettings) add(ui::FooterAction::Settings, "SETTINGS");
  return count;
}

void footerSegment(const ChromeMetrics& m, int index, int count, const FooterItem& item) {
  const int left = index * m.width / count;
  const int right = (index + 1) * m.width / count;
  if (index) M5.Display.fillRect(left, m.footerTop, 3, m.footer, TFT_BLACK);
  const int centerX = (left + right) / 2;
  const int centerY = m.footerTop + m.footer / 2;
  // Footer navigation is deliberately text-only. One centred, bold label per
  // segment is clearer on e-paper than mixing differently sized icons with
  // text, and remains consistent across every app.
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE); M5.Display.setFont(&fonts::FreeSansBold9pt7b); M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString(item.label, centerX, centerY);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}
}

namespace ui {
int Chrome::headerHeight() { return metrics().header; }
int Chrome::footerHeight() { return metrics().footer; }
int Chrome::footerTop() { return metrics().footerTop; }
FooterAction Chrome::swipeAction(int startX,int startY,int endX,int endY,const ChromeOptions& options) {
  const int dx=endX-startX, dy=endY-startY;
  if (abs(dx)<70 || abs(dy)>55 || startY<headerHeight() || startY>footerTop()) return FooterAction::None;
  if (dx<0 && options.showNext) return FooterAction::Next;
  if (dx>0 && options.showPrevious) return FooterAction::Previous;
  return FooterAction::None;
}

void Chrome::drawHeader(AppContext& context, const char* title, bool legacyShowBack, bool uppercaseTitle) {
  const auto m = metrics();
  M5.Display.fillRect(0, 0, m.width, m.header, TFT_BLACK);
  M5.Display.drawFastHLine(0, m.header - 1, m.width, TFT_WHITE);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  (void)legacyShowBack;
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextSize(1);
  M5.Display.setTextDatum(ML_DATUM); M5.Display.drawString(context.time.formattedDate(), m.margin + 8, m.header / 2 + 4);
  String headerTitle(title); if (uppercaseTitle) headerTitle.toUpperCase();
  M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString(headerTitle, m.width / 2, m.header / 2 + 4);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
  drawWifiIcon(m.width - 81, m.header / 2 - 6, context.network.connected(), context.network.signalStrength(), TFT_WHITE, TFT_BLACK);
  drawBattery(m.width - 46, m.header / 2 - 10, context.power.batteryPercent(), M5.Power.isCharging() == m5::Power_Class::is_charging, TFT_WHITE, TFT_BLACK);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
}

void Chrome::drawFooter(const ChromeOptions& options) {
  const auto m = metrics();
  M5.Display.fillRect(0, m.footerTop, m.width, m.footer, TFT_WHITE);
  M5.Display.fillRect(0, m.footerTop, m.width, 3, TFT_BLACK);
  M5.Display.fillRect(0, m.footerTop, 3, m.footer, TFT_BLACK);
  M5.Display.fillRect(m.width - 3, m.footerTop, 3, m.footer, TFT_BLACK);
  M5.Display.fillRect(0, m.height - 3, m.width, 3, TFT_BLACK);
  FooterItem items[5]; const uint8_t count = footerItems(options, items);
  for (uint8_t i = 0; i < count; ++i) footerSegment(m, i, count, items[i]);
}

void Chrome::drawFooter() { drawFooter(ChromeOptions{}); }

FooterAction Chrome::hitTestFooter(int x, int y, const ChromeOptions& options) {
  const auto m = metrics();
  if (y < m.footerTop || y >= m.height) return FooterAction::None;
  FooterItem items[5]; const uint8_t count = footerItems(options, items);
  if (!count) return FooterAction::None;
  const uint8_t index = static_cast<uint8_t>((static_cast<int32_t>(x) * count) / m.width);
  return index < count ? items[index].action : FooterAction::None;
}

FooterAction Chrome::hitTestFooter(int x, int y) { return hitTestFooter(x, y, ChromeOptions{}); }
} // namespace ui
