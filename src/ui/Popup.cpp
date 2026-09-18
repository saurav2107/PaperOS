#include "ui/Popup.h"
#include "ui/Theme.h"

#include <M5Unified.h>

namespace ui::Popup {
namespace {
constexpr int kCloseWidth = 112;
constexpr int kCloseHeight = 42;
int closeX(int popupX, int popupWidth) { return popupX + popupWidth - kCloseWidth - 18; }
}

void drawFrame(int x, int y, int width, int height, const char* title, bool closeButton) {
  Theme::drawFrame(x, y, width, height, 14);
  // Make only the top corners rounded; the lower edge of the title rail is
  // intentionally square so content aligns cleanly beneath it.
  M5.Display.fillRoundRect(x + Theme::Border, y + Theme::Border, width - Theme::Border * 2, kHeaderHeight, 11, Theme::Ink);
  M5.Display.fillRect(x + Theme::Border, y + 35, width - Theme::Border * 2, kHeaderHeight - 32, Theme::Ink);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setFont(&fonts::FreeSansBold12pt7b); M5.Display.setTextDatum(ML_DATUM);
  M5.Display.drawString(title, x + 28, y + kHeaderHeight / 2 + 2);
  if (closeButton) {
    const int bx = closeX(x, width), by = y + 15;
    // Reversed control: white outer button and black inner face inside the
    // dark rail, with a white label and X for reliable e-paper contrast.
    M5.Display.fillRoundRect(bx, by, kCloseWidth, kCloseHeight, Theme::ButtonRadius, Theme::Paper);
    M5.Display.fillRoundRect(bx + Theme::Border, by + Theme::Border, kCloseWidth - Theme::Border * 2, kCloseHeight - Theme::Border * 2, 4, Theme::Ink);
    M5.Display.drawLine(bx + 15, by + 13, bx + 27, by + 27, TFT_WHITE);
    M5.Display.drawLine(bx + 27, by + 13, bx + 15, by + 27, TFT_WHITE);
    M5.Display.setFont(&fonts::FreeSansBold9pt7b); M5.Display.setTextDatum(ML_DATUM);
    M5.Display.drawString("CLOSE", bx + 38, by + 22);
  }
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setTextColor(TFT_BLACK, TFT_WHITE); M5.Display.setFont(nullptr);
}

bool hitClose(int x, int y, int popupX, int popupY, int popupWidth) {
  const int bx = closeX(popupX, popupWidth), by = popupY + 15;
  return x >= bx && x <= bx + kCloseWidth && y >= by && y <= by + kCloseHeight;
}
}  // namespace ui::Popup
