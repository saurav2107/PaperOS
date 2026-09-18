#include "services/Services.h"
#include <Preferences.h>

void ClockFaceService::begin() {
  Preferences preferences;
  preferences.begin("paperos", true);
  const uint8_t stored = preferences.getUChar("clockFace", 0);
  preferences.end();
  face_ = stored < static_cast<uint8_t>(ClockFace::Count)
      ? static_cast<ClockFace>(stored) : ClockFace::Flip;
}

void ClockFaceService::save() {
  Preferences preferences;
  preferences.begin("paperos", false);
  preferences.putUChar("clockFace", static_cast<uint8_t>(face_));
  preferences.end();
}

void ClockFaceService::next() {
  const uint8_t next = (static_cast<uint8_t>(face_) + 1) % static_cast<uint8_t>(ClockFace::Count);
  face_ = static_cast<ClockFace>(next);
  save();
}

const char* ClockFaceService::name() const {
  switch (face_) {
    case ClockFace::DarkFlip: return "Dark flip";
    case ClockFace::Digital: return "Minimal digital";
    case ClockFace::Analog: return "Analog";
    case ClockFace::DateTemperature: return "Date & temperature";
    case ClockFace::Dotted: return "Dotted";
    case ClockFace::Panel: return "Dashboard panel";
    default: return "Flip clock";
  }
}
