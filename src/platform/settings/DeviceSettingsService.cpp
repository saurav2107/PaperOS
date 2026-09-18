#include "services/Services.h"
#include <Preferences.h>
#include "config/Location.h"

namespace {
constexpr const char* kNamespace = "paperos";
uint8_t bounded(uint8_t value, uint8_t fallback) { return value <= 60 ? value : fallback; }
}

void DeviceSettingsService::begin() {
  Preferences p; p.begin(kNamespace, true);
  ssid_ = p.getString("ssid", ""); password_ = p.getString("password", "");
  location_.address = p.getString("address", kHomeLocation.label);
  location_.city = p.getString("city", "Noida"); location_.country = p.getString("country", "India");
  location_.latitude = p.getDouble("latitude", kHomeLocation.latitude);
  location_.longitude = p.getDouble("longitude", kHomeLocation.longitude);
  timezone_ = p.getString("timezone", kHomeLocation.timezone);
  use24Hour_ = p.getBool("clock24h", true); buttonSound_ = p.getBool("buttonSound", true);
  useFahrenheit_ = p.getBool("fahrenheit", false);
  displayFlipped_ = p.getBool("displayFlip", false);
  inactivityMinutes_ = bounded(p.getUChar("idleMinutes", 5), 5);
  weatherRetryMinutes_ = bounded(p.getUChar("weatherRetry", 5), 5);
  geminiApiKey_ = p.getString("geminiKey", "");
  geminiTextModel_ = p.getString("geminiText", "gemini-2.5-flash");
  geminiImageModel_ = p.getString("geminiImage", "gemini-3.1-flash-image");
  p.end();
  if (timezone_.isEmpty()) timezone_ = kHomeLocation.timezone;
  if (location_.address.isEmpty()) location_.address = kHomeLocation.label;
}

void DeviceSettingsService::save() {
  Preferences p; p.begin(kNamespace, false);
  p.putString("ssid", ssid_); p.putString("password", password_);
  p.putString("address", location_.address); p.putString("city", location_.city); p.putString("country", location_.country);
  p.putDouble("latitude", location_.latitude); p.putDouble("longitude", location_.longitude);
  p.putString("timezone", timezone_); p.putBool("clock24h", use24Hour_); p.putBool("buttonSound", buttonSound_);
  p.putBool("fahrenheit", useFahrenheit_); p.putBool("displayFlip", displayFlipped_); p.putUChar("idleMinutes", inactivityMinutes_); p.putUChar("weatherRetry", weatherRetryMinutes_);
  p.putString("geminiKey", geminiApiKey_); p.putString("geminiText", geminiTextModel_); p.putString("geminiImage", geminiImageModel_);
  p.end();
}
void DeviceSettingsService::setWifi(const String& ssid, const String& password) { ssid_ = ssid; password_ = password; save(); }
void DeviceSettingsService::setLocation(const DeviceLocation& value) {
  if (value.latitude < -90 || value.latitude > 90 || value.longitude < -180 || value.longitude > 180) return;
  location_ = value; save();
}
void DeviceSettingsService::setTimezone(const String& value) { if (!value.isEmpty()) { timezone_ = value; save(); } }
void DeviceSettingsService::setUse24Hour(bool value) { use24Hour_ = value; save(); }
void DeviceSettingsService::setButtonSound(bool value) { buttonSound_ = value; save(); }
void DeviceSettingsService::setUseFahrenheit(bool value) { useFahrenheit_ = value; save(); }
void DeviceSettingsService::setDisplayFlipped(bool value) { displayFlipped_ = value; save(); }
void DeviceSettingsService::setInactivityMinutes(uint8_t value) { inactivityMinutes_ = bounded(value, 5); save(); }
void DeviceSettingsService::setWeatherRetryMinutes(uint8_t value) { weatherRetryMinutes_ = bounded(value, 5); save(); }
void DeviceSettingsService::setGemini(const String& apiKey, const String& textModel, const String& imageModel) {
  if (!apiKey.isEmpty()) geminiApiKey_ = apiKey;
  if (!textModel.isEmpty()) geminiTextModel_ = textModel;
  if (!imageModel.isEmpty()) geminiImageModel_ = imageModel;
  save();
}
void DeviceSettingsService::reset() { Preferences p; p.begin(kNamespace, false); p.clear(); p.end(); begin(); }
