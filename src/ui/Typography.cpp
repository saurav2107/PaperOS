#include "ui/Typography.h"

#include <M5Unified.h>
#include <Preferences.h>

namespace ui {
namespace { float gScale = 1.0f; }

void Typography::begin() {
  Preferences preferences; preferences.begin("paperos", true);
  const uint16_t stored = preferences.getUShort("fontScale", 100);
  preferences.end();
  gScale = stored < 80 || stored > 140 ? 1.0f : stored / 100.0f;
}

void Typography::apply(TextRole role, float localScale) {
  switch (role) {
    case TextRole::Caption: M5.Display.setFont(&fonts::FreeSans9pt7b); break;
    case TextRole::Body: M5.Display.setFont(&fonts::FreeSans12pt7b); break;
    case TextRole::BodyBold: M5.Display.setFont(&fonts::FreeSansBold12pt7b); break;
    case TextRole::Heading: M5.Display.setFont(&fonts::FreeSansBold18pt7b); break;
    case TextRole::Title: M5.Display.setFont(&fonts::FreeSansBold24pt7b); break;
    case TextRole::Numeric: M5.Display.setFont(&fonts::Orbitron_Light_32); break;
  }
  M5.Display.setTextSize(gScale * localScale);
}

void Typography::reset() { M5.Display.setTextSize(1); M5.Display.setFont(nullptr); }
float Typography::scale() { return gScale; }
void Typography::setScale(float value) {
  gScale = value < 0.8f ? 0.8f : value > 1.4f ? 1.4f : value;
  Preferences preferences; preferences.begin("paperos", false);
  preferences.putUShort("fontScale", static_cast<uint16_t>(gScale * 100.0f));
  preferences.end();
}

}  // namespace ui
