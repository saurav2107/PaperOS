#pragma once
#include <Arduino.h>
#include <time.h>
#include "app/AppContext.h"
#include "platform/weather/WeatherFetchService.h"
#include "platform/ai/GeminiFlashcardService.h"

class TaskRegistry {
 public:
  void stopOwnedBy(uint8_t owner);
};

class DisplayService {
 public:
  void begin();
  void setFlipped(bool flipped);
  void setPortrait();
  void setLandscape();
  bool flipped() const { return flipped_; }
  void clear();
  void header(const char* title);
  void message(const char* title, const char* detail);
  void resetToLauncherState();
 private:
  bool flipped_{false};
};

class InputService {
 public:
  void begin();
  void update();
};

// The single owner of device-wide, persistent configuration.  Apps receive
// this through AppContext but do not open Preferences directly for system
// policies.  This is the embedded equivalent of a typed .NET options store.
struct DeviceLocation {
  String address;
  String city;
  String country;
  double latitude{28.61418};
  double longitude{77.38564};
};

class DeviceSettingsService {
 public:
  void begin();
  const String& ssid() const { return ssid_; }
  const String& password() const { return password_; }
  void setWifi(const String& ssid, const String& password);
  const DeviceLocation& location() const { return location_; }
  void setLocation(const DeviceLocation& value);
  const String& timezone() const { return timezone_; }
  void setTimezone(const String& value);
  bool use24Hour() const { return use24Hour_; }
  void setUse24Hour(bool value);
  bool buttonSound() const { return buttonSound_; }
  void setButtonSound(bool value);
  bool useFahrenheit() const { return useFahrenheit_; }
  void setUseFahrenheit(bool value);
  bool displayFlipped() const { return displayFlipped_; }
  void setDisplayFlipped(bool value);
  uint8_t inactivityMinutes() const { return inactivityMinutes_; }
  void setInactivityMinutes(uint8_t value);
  uint8_t weatherRetryMinutes() const { return weatherRetryMinutes_; }
  void setWeatherRetryMinutes(uint8_t value);
  const String& geminiApiKey() const { return geminiApiKey_; }
  const String& geminiTextModel() const { return geminiTextModel_; }
  const String& geminiImageModel() const { return geminiImageModel_; }
  void setGemini(const String& apiKey, const String& textModel, const String& imageModel);
  void reset();
 private:
  void save();
  String ssid_, password_, timezone_, geminiApiKey_;
  String geminiTextModel_{"gemini-2.5-flash"};
  String geminiImageModel_{"gemini-3.1-flash-image"};
  DeviceLocation location_;
  bool use24Hour_{true}, buttonSound_{true}, useFahrenheit_{false}, displayFlipped_{false};
  uint8_t inactivityMinutes_{5}, weatherRetryMinutes_{5};
};

// Global UI feedback policy. Services owns the preference so every app gets
// identical button feedback without duplicating speaker calls in app code.
class UiSoundService {
 public:
  void begin();
  bool enabled() const { return enabled_; }
  void setEnabled(bool value);
  void setSettings(DeviceSettingsService* settings) { settings_ = settings; }
  void click();
 private:
  bool enabled_{true};
  DeviceSettingsService* settings_{nullptr};
};

class StorageService {
 public:
  bool begin();
  bool mounted() const { return mounted_; }
  bool unmountForUsb();
  bool mountForFirmware();
  // Removes stale reader cache files until a bounded write can safely proceed.
  // Returns false rather than risking a partial/cache-corrupting write.
  bool prepareCacheWrite(size_t bytesNeeded);
 private:
  bool mounted_{false};
};

class NetworkService {
 public:
  void begin();
  void tick();
  bool connected() const;
  int32_t signalStrength() const;
  String configuredSsid() const;
  String configuredAddress() const;
  // Asynchronous station scan used by the on-device Settings UI. It never
  // blocks loopTask, so touch and power handling remain responsive.
  void startScan();
  bool scanInProgress() const;
  int scanResultCount() const;
  String scanSsid(uint8_t index) const;
  int32_t scanRssi(uint8_t index) const;
  void connect(const String& ssid, const String& password);
  bool provisioning() const { return provisioning_; }
  static const char* apSsid() { return "PaperOS-Setup"; }
  static const char* apPassword() { return "paperossetup"; }
  void startProvisioning();
  void stopProvisioning();
  void setSettings(DeviceSettingsService* settings) { settings_ = settings; }
 private:
  void beginPortal();
  bool provisioning_{false};
  bool hasStationCredentials_{false};
  uint32_t nextReconnectMs_{0};
  uint32_t nextPortalServiceMs_{0};
  DeviceSettingsService* settings_{nullptr};
};

class TimeService {
 public:
  void begin();
  void tick();
  const char* formattedLocalTime();
  const char* formattedDate();
  bool localTime(tm& value) const;
  const char* timezone() const { return timezone_; }
  void setTimezone(const char* timezone);
  bool syncFromInternet();
  // Starts NTP synchronisation without blocking the UI task. Poll result via
  // syncInProgress()/lastSyncSucceeded().
  bool requestInternetSync();
  bool syncInProgress() const { return syncInProgress_; }
  bool lastSyncSucceeded() const { return lastSyncSucceeded_; }
  void setSettings(DeviceSettingsService* settings) { settings_ = settings; }
  bool use24Hour() const { return use24Hour_; }
  bool hasHardwareRtc() const { return hardwareRtc_; }
  void setUse24Hour(bool value);
  void formatTime(const tm& value, char* target, size_t targetSize, bool seconds = false) const;
  // Converts the selected local wall-clock date/time through the configured
  // timezone and writes it to the ESP32 RTC-backed system clock.
  bool setManualLocalTime(const tm& local);
 private:
  void applyTimezone();
  void writeHardwareRtc(const tm& local);
  void refreshCache();
  char buffer_[32]{};
  char dateBuffer_[24]{};
  char timezone_[48]{};
  bool use24Hour_{true};
  bool hardwareRtc_{false};
  uint32_t nextClockRefreshMs_{0};
  DeviceSettingsService* settings_{nullptr};
  bool syncInProgress_{false};
  bool lastSyncSucceeded_{false};
  uint32_t syncDeadlineMs_{0};
};

class PowerService {
 public:
  void begin();
  void tick(const char* localTime);
  void setSleepInhibited(bool inhibited);
  uint8_t batteryPercent() const;
  uint8_t inactivityMinutes() const { return inactivityMinutes_; }
  void setInactivityMinutes(uint8_t minutes);
  void noteActivity();
  bool consumeWakeEvent();
  void sleepNow(const char* localTime);
  [[noreturn]] void powerOff();
  void setSettings(DeviceSettingsService* settings) { settings_ = settings; }
 private:
  void drawSleepScreen(const char* localTime);
  uint8_t inactivityMinutes_{5};
  uint32_t lastActivityMs_{0};
  bool wakeEvent_{false};
  bool sleepInhibited_{false};
  DeviceSettingsService* settings_{nullptr};
};

// Persistent policy used by Weather when a network request cannot succeed.
class WeatherSettingsService {
 public:
  void begin();
  uint8_t offlineRetryMinutes() const { return offlineRetryMinutes_; }
  void setOfflineRetryMinutes(uint8_t minutes);
  void setSettings(DeviceSettingsService* settings) { settings_ = settings; }
 private:
  uint8_t offlineRetryMinutes_{5};
  DeviceSettingsService* settings_{nullptr};
};

// Shared temperature policy and last known outdoor reading.  Apps provide
// Celsius data; this service owns conversion and the user's unit preference.
class TemperatureService {
 public:
  void begin();
  bool useFahrenheit() const { return useFahrenheit_; }
  void setUseFahrenheit(bool value);
  void setLastCelsius(float value);
  void setLastHumidity(float value);
  float lastHumidity() const { return lastHumidity_; }
  bool hasLastTemperature() const;
  String format(float celsius, uint8_t decimals = 0) const;
  String lastFormatted(uint8_t decimals = 0) const;
  void setSettings(DeviceSettingsService* settings) { settings_ = settings; }
 private:
  bool useFahrenheit_{false};
  float lastCelsius_{NAN};
  float lastHumidity_{NAN};
  DeviceSettingsService* settings_{nullptr};
};

enum class ClockFace : uint8_t { Flip, Digital, Analog, DateTemperature, Dotted, Panel, DarkFlip, Count };

// Presentation preference kept outside ClockApp so a new app instance keeps
// the user's chosen face after navigation or a device restart.
class ClockFaceService {
 public:
  void begin();
  ClockFace face() const { return face_; }
  void next();
  const char* name() const;
 private:
  void save();
  ClockFace face_{ClockFace::Flip};
};

enum class PomodoroStyle : uint8_t { Ring, Minimal, Progress, Focus };
class PomodoroStyleService {
 public:
  void begin();
  PomodoroStyle style() const { return style_; }
  void next();
  const char* name() const;
 private:
  void save();
  PomodoroStyle style_{PomodoroStyle::Ring};
};

enum class WeatherFace : uint8_t {
  Dashboard,
  Minimal,
  Hourly,
  Daily,
  TodayCard,
  Timeline,
  ThreeDay,
  WeatherClock,
};
class WeatherFaceService {
 public:
  void begin();
  WeatherFace face() const { return face_; }
  void next();
  const char* name() const;
 private:
  void save();
  WeatherFace face_{WeatherFace::Dashboard};
};

class Services {
 public:
  Services();
  void begin();
  void tick();
  void setSleepInhibited(bool inhibited) { power_.setSleepInhibited(inhibited); }
  bool consumeWakeEvent() { return power_.consumeWakeEvent(); }
  AppContext& context() { return context_; }

 private:
  TaskRegistry tasks_;
  DisplayService display_;
  DeviceSettingsService settings_;
  InputService input_;
  UiSoundService uiSound_;
  StorageService storage_;
  NetworkService network_;
  TimeService time_;
  PowerService power_;
  WeatherSettingsService weatherSettings_;
  TemperatureService temperature_;
  ClockFaceService clockFace_;
  PomodoroStyleService pomodoroStyle_;
  WeatherFaceService weatherFace_;
  WeatherFetchService weatherFetch_;
  GeminiFlashcardService geminiFlashcards_;
  AppContext context_;
};
