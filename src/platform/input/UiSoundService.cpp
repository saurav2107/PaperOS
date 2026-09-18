#include "services/Services.h"
#include <M5Unified.h>

void UiSoundService::begin() {
  enabled_ = settings_ ? settings_->buttonSound() : true;
  // main.cpp has already configured and started PaperS3's GPIO21 buzzer.
  M5.Speaker.setVolume(120);
}

void UiSoundService::setEnabled(bool value) {
  enabled_ = value;
  if (settings_) settings_->setButtonSound(enabled_);
}

void UiSoundService::click() {
  if (enabled_) M5.Speaker.tone(1500, 18);
}
