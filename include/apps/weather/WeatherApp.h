#pragma once
#include "app/IApp.h"
#include "apps/native/StatusApp.h"

struct HourlyWeather {
  float temperature{0};
  uint16_t weatherCode{0};
};

struct DailyWeather {
  float minimum{0};
  float maximum{0};
  uint16_t weatherCode{0};
  uint8_t precipitationProbability{0};
  String date;
};

struct WeatherSnapshot {
  float temperature{0};
  float apparentTemperature{0};
  float humidity{0};
  float windSpeed{0};
  float windDirection{0};
  float precipitation{0};
  uint16_t weatherCode{0};
  String sunrise;
  String sunset;
  HourlyWeather hourly[8];
  DailyWeather daily[3];
};

class WeatherApp final : public IApp {
 public:
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  AppId id() const override { return AppId::Weather; }
  const char* title() const override { return "Weather"; }
  // AppManager calls onStart/onStop around navigation. Weather owns landscape
  // while active and returns the display to portrait when it closes.
  bool onStart(AppContext& context) override;
  // nowMs comes from Arduino millis() via AppManager. It drives scheduled
  // fetch attempts; touch coordinates are read from M5.Touch in this method.
  void onTick(AppContext& context, uint32_t nowMs) override;
  void onStop(AppContext& context) override;
 private:
  // Fetches Open-Meteo using NetworkService. The supplied context carries both
  // Wi-Fi state and TemperatureService, where the last outdoor reading is kept.
  bool fetch(AppContext&);
  // Full Weather screen composition. Used only for navigation, manual refresh,
  // day navigation, or configured retry—not continuously every loop.
  void draw(AppContext&);
  // Offline fallback, centred within Chrome's current landscape content bounds.
  void drawOffline(AppContext&);
  // Renders current conditions, eight hourly cells and selected dayIndex_.
  void drawDashboard(AppContext&);
  void drawMinimal(AppContext&);
  void drawHourly(AppContext&);
  void drawDaily(AppContext&);
  void drawTodayCard(AppContext&);
  void drawTimeline(AppContext&);
  void drawThreeDay(AppContext&);
  void drawWeatherClock(AppContext&);
  // Draws an SD-backed Weather Icons PNG for the Open-Meteo code. If the SD
  // card or a matching icon is unavailable, it falls back to the native glyph.
  void drawWeatherIcon(int x, int y, int size, uint16_t code) const;
  bool drawSdWeatherIcon(int x, int y, int size, uint16_t code) const;
  // x/y are M5.Touch coordinates. It resolves shared footer actions, the
  // refresh icon, and PREV/NEXT day actions without changing app ownership.
  void handleTouch(AppContext&, int x, int y);
  // Converts the numeric Open-Meteo weather code to short display text.
  const char* condition(uint16_t code) const;
  NavigationCallback navigator_{nullptr};
  WeatherSnapshot weather_;
  uint32_t lastFetchMs_{0};
  uint32_t lastAttemptMs_{0};
  // Selected 0..2 daily forecast. It is changed only by footer PREV/NEXT.
  uint8_t dayIndex_{0};
  bool loaded_{false};
  bool fullScreen_{false};
  bool pendingFaceChange_{false};
  uint32_t lastTapMs_{0};
  String status_;
};
