#include "platform/weather/WeatherFetchService.h"
#include "services/Services.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>

namespace {
struct Request { WeatherFetchService* service; DeviceLocation location; };
void copyTime(char (&target)[6], const char* value) {
  if (!value) return;
  const size_t length = strlen(value);
  if (length >= 16) { memcpy(target, value + 11, 5); target[5] = 0; }
}
void fail(WeatherFetchResult& result, const char* message) { strlcpy(result.error, message, sizeof(result.error)); }
}

bool WeatherFetchService::request(const DeviceLocation& location) {
  if (inProgress_ || WiFi.status() != WL_CONNECTED) return false;
  inProgress_ = true;
  Request* request = new Request{this, location};
  if (!request || xTaskCreatePinnedToCore(worker, "WeatherFetch", 10240, request, 1, &task_, 0) != pdPASS) {
    delete request;
    inProgress_ = false; return false;
  }
  return true;
}

void WeatherFetchService::worker(void* argument) {
  Request* request = static_cast<Request*>(argument);
  request->service->run(request->location);
  delete request;
  vTaskDelete(nullptr);
}

void WeatherFetchService::run(DeviceLocation location) {
  WeatherFetchResult completed{};
  if (WiFi.status() != WL_CONNECTED) fail(completed, "Wi-Fi is offline");
  else {
    WiFiClientSecure client; client.setInsecure();
    HTTPClient http;
    const String url = String("https://api.open-meteo.com/v1/forecast?latitude=") + String(location.latitude, 5) + "&longitude=" + String(location.longitude, 5) +
      "&current=temperature_2m,apparent_temperature,relative_humidity_2m,precipitation,wind_speed_10m,wind_direction_10m,weather_code" +
      "&hourly=temperature_2m,weather_code&daily=sunrise,sunset,weather_code,temperature_2m_min,temperature_2m_max,precipitation_probability_max&timezone=auto&forecast_days=3";
    http.useHTTP10(true); http.setReuse(false); http.addHeader("Accept-Encoding", "identity"); http.setTimeout(10000);
    if (!http.begin(client, url)) fail(completed, "Could not open weather connection");
    else {
      const int code = http.GET();
      if (code != HTTP_CODE_OK) { snprintf(completed.error, sizeof(completed.error), "Weather request failed: HTTP %d", code); http.end(); }
      else {
        JsonDocument doc; const DeserializationError jsonError = deserializeJson(doc, http.getStream()); http.end();
        JsonObject current=doc["current"]; JsonArray temperatures=doc["hourly"]["temperature_2m"], codes=doc["hourly"]["weather_code"];
        JsonArray sunrises=doc["daily"]["sunrise"], sunsets=doc["daily"]["sunset"], dailyCodes=doc["daily"]["weather_code"], dailyMinimums=doc["daily"]["temperature_2m_min"], dailyMaximums=doc["daily"]["temperature_2m_max"], dailyRain=doc["daily"]["precipitation_probability_max"], dailyDates=doc["daily"]["time"];
        if (jsonError || current.isNull() || temperatures.isNull() || codes.isNull() || sunrises.isNull() || sunsets.isNull() || dailyCodes.isNull() || dailyMinimums.isNull() || dailyMaximums.isNull() || dailyRain.isNull() || dailyDates.isNull()) fail(completed, "Weather response is incomplete");
        else {
          completed.temperature=current["temperature_2m"]|0.0f; completed.apparentTemperature=current["apparent_temperature"]|0.0f; completed.humidity=current["relative_humidity_2m"]|0.0f; completed.precipitation=current["precipitation"]|0.0f; completed.windSpeed=current["wind_speed_10m"]|0.0f; completed.windDirection=current["wind_direction_10m"]|0.0f; completed.weatherCode=current["weather_code"]|0;
          copyTime(completed.sunrise,sunrises[0]|""); copyTime(completed.sunset,sunsets[0]|"");
          time_t now=time(nullptr); tm local{}; localtime_r(&now,&local); const int offset=local.tm_hour;
          if (temperatures.size() < static_cast<size_t>(offset+8) || codes.size() < static_cast<size_t>(offset+8)) fail(completed,"Weather response has no hourly forecast");
          else { for(int i=0;i<8;++i){completed.hourlyTemperature[i]=temperatures[offset+i]|0.0f;completed.hourlyCode[i]=codes[offset+i]|0;} for(int i=0;i<3;++i){completed.dailyMinimum[i]=dailyMinimums[i]|0.0f;completed.dailyMaximum[i]=dailyMaximums[i]|0.0f;completed.dailyCode[i]=dailyCodes[i]|0;completed.dailyRain[i]=dailyRain[i]|0;const char* date=dailyDates[i]|"";if(strlen(date)>=10){memcpy(completed.dailyDate[i],date+5,5);completed.dailyDate[i][5]=0;}} completed.success=true; }
        }
      }
    }
  }
  portENTER_CRITICAL(&lock_); result_=completed; ready_=true; inProgress_=false; task_=nullptr; portEXIT_CRITICAL(&lock_);
}

bool WeatherFetchService::consume(WeatherFetchResult& result) {
  bool available=false; portENTER_CRITICAL(&lock_); if(ready_){result=result_;ready_=false;available=true;} portEXIT_CRITICAL(&lock_); return available;
}
