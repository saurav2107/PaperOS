#include "services/Services.h"
#include <math.h>
#include <Preferences.h>

void TemperatureService::begin() {
  Preferences preferences;
  preferences.begin("paperos", true);
  useFahrenheit_ = settings_ ? settings_->useFahrenheit() : preferences.getBool("tempFahrenheit", false);
  lastCelsius_ = preferences.getFloat("lastTempC", NAN);
  lastHumidity_ = preferences.getFloat("lastHumidity", NAN);
  preferences.end();
}

void TemperatureService::setUseFahrenheit(bool value) {
  useFahrenheit_ = value;
  if (settings_) settings_->setUseFahrenheit(useFahrenheit_);
}

void TemperatureService::setLastCelsius(float value) {
  if (isnan(value)) return;
  lastCelsius_ = value;
  Preferences preferences;
  preferences.begin("paperos", false);
  preferences.putFloat("lastTempC", lastCelsius_);
  preferences.end();
}

bool TemperatureService::hasLastTemperature() const { return !isnan(lastCelsius_); }

void TemperatureService::setLastHumidity(float value) {
  if (!isfinite(value) || value < 0 || value > 100 || value == lastHumidity_) return;
  lastHumidity_ = value;
  Preferences preferences;
  preferences.begin("paperos", false);
  preferences.putFloat("lastHumidity", value);
  preferences.end();
}

String TemperatureService::format(float celsius, uint8_t decimals) const {
  if (isnan(celsius)) return String("--");
  const float value = useFahrenheit_ ? celsius * 9.0f / 5.0f + 32.0f : celsius;
  // The bundled FreeSans fonts do not reliably contain Unicode U+00B0.
  // A plain ASCII `o` gives the familiar e-paper/dot-matrix degree marker
  // and is guaranteed to render in every font used across Paper OS.
  return String(value, static_cast<unsigned int>(decimals)) + (useFahrenheit_ ? "oF" : "oC");
}

String TemperatureService::lastFormatted(uint8_t decimals) const {
  return format(lastCelsius_, decimals);
}
