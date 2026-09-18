#include "services/Services.h"
#include <Preferences.h>

void PomodoroStyleService::begin() {
  Preferences preferences; preferences.begin("paperos", true);
  const uint8_t value = preferences.getUChar("pomodoroStyle", 0); preferences.end();
  style_ = value <= static_cast<uint8_t>(PomodoroStyle::Focus) ? static_cast<PomodoroStyle>(value) : PomodoroStyle::Ring;
}
void PomodoroStyleService::save() {
  Preferences preferences; preferences.begin("paperos", false);
  preferences.putUChar("pomodoroStyle", static_cast<uint8_t>(style_)); preferences.end();
}
void PomodoroStyleService::next() { style_ = static_cast<PomodoroStyle>((static_cast<uint8_t>(style_) + 1) % 4); save(); }
const char* PomodoroStyleService::name() const {
  switch (style_) {
    case PomodoroStyle::Minimal: return "MINIMAL";
    case PomodoroStyle::Progress: return "PROGRESS";
    case PomodoroStyle::Focus: return "FOCUS";
    default: return "RING";
  }
}
