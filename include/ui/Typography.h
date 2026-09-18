#pragma once

namespace ui {

// A small global type scale for PaperOS. Apps select semantic roles instead
// of hard-coding a particular bundled font; changing the scale affects every
// app that adopts these roles while preserving its intended hierarchy.
enum class TextRole : unsigned char { Caption, Body, BodyBold, Heading, Title, Numeric };

class Typography {
 public:
  static void begin();
  static void apply(TextRole role, float localScale = 1.0f);
  static void reset();
  static float scale();
  static void setScale(float value);
};

}  // namespace ui
