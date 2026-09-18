#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
struct DeviceLocation;

// Plain-data result transfer keeps the HTTPS worker isolated from the UI task.
// No String or display object crosses the FreeRTOS task boundary.
struct WeatherFetchResult {
  bool success{false};
  float temperature{0}, apparentTemperature{0}, humidity{0}, windSpeed{0}, windDirection{0}, precipitation{0};
  uint16_t weatherCode{0};
  float hourlyTemperature[8]{}; uint16_t hourlyCode[8]{};
  float dailyMinimum[3]{}, dailyMaximum[3]{}; uint16_t dailyCode[3]{}; uint8_t dailyRain[3]{};
  char sunrise[6]{}, sunset[6]{}, dailyDate[3][6]{}, error[96]{};
};

class WeatherFetchService {
 public:
  bool request(const DeviceLocation& location);
  bool inProgress() const { return inProgress_; }
  bool consume(WeatherFetchResult& result);
 private:
  static void worker(void* argument);
  void run(DeviceLocation location);
  portMUX_TYPE lock_ = portMUX_INITIALIZER_UNLOCKED;
  TaskHandle_t task_{nullptr};
  volatile bool inProgress_{false};
  bool ready_{false};
  WeatherFetchResult result_{};
};
