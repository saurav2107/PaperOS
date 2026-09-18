#include "services/Services.h"

void WeatherSettingsService::begin() {
  offlineRetryMinutes_ = settings_ ? settings_->weatherRetryMinutes() : 5;
}

void WeatherSettingsService::setOfflineRetryMinutes(uint8_t minutes) {
  offlineRetryMinutes_ = minutes;
  if (settings_) settings_->setWeatherRetryMinutes(offlineRetryMinutes_);
}
