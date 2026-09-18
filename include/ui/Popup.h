#pragma once

#include <Arduino.h>

// Shared modal construction.  Keeping the rail here means a popup is
// recognisable everywhere: dark rounded title band, reversed title, and an
// inverted close affordance.  The caller still owns its content and actions.
namespace ui::Popup {
constexpr int kHeaderHeight = 72;
void drawFrame(int x, int y, int width, int height, const char* title, bool closeButton = true);
bool hitClose(int x, int y, int popupX, int popupY, int popupWidth);
}  // namespace ui::Popup
