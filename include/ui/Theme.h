#pragma once

#include <stdint.h>

// PaperOS design tokens. Geometry and colour that must look identical across
// apps live here rather than being copied as one-off literal values.
namespace ui::Theme {
constexpr uint16_t Ink = 0x0000;
constexpr uint16_t Paper = 0xFFFF;
constexpr int Border = 3;
constexpr int CardRadius = 10;
constexpr int ButtonRadius = 7;
constexpr int NextIconSize = 24;
constexpr int ListIconSize = 30;
constexpr int StandardButtonHeight = 56;

// Filled outer/inner frames are deliberately used instead of thin outlines:
// their complete 3px bottom edge survives e-paper refreshes consistently.
void drawFrame(int x, int y, int width, int height, int radius = CardRadius, bool selected = false);
void drawButtonFrame(int x, int y, int width, int height, bool selected = false);
}  // namespace ui::Theme
