#include "ui/Theme.h"

#include <M5Unified.h>

namespace ui::Theme {
void drawFrame(int x, int y, int width, int height, int radius, bool selected) {
  if (width <= Border * 2 || height <= Border * 2) return;
  const int innerRadius = radius > Border ? radius - Border : 1;
  M5.Display.fillRoundRect(x, y, width, height, radius, Ink);
  M5.Display.fillRoundRect(x + Border, y + Border, width - Border * 2, height - Border * 2,
                           innerRadius, selected ? Ink : Paper);
}

void drawButtonFrame(int x, int y, int width, int height, bool selected) {
  drawFrame(x, y, width, height, ButtonRadius, selected);
}
}  // namespace ui::Theme
