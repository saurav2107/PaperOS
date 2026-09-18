#pragma once
#include <Arduino.h>
#include "config/Location.h"

class WeatherClient {
 public:
  String forecastUrl() const {
    return String("https://api.open-meteo.com/v1/forecast?latitude=") +
        String(kHomeLocation.latitude, 5) + "&longitude=" +
        String(kHomeLocation.longitude, 5) +
        "&timezone=Asia%2FKolkata&current=temperature_2m,relative_humidity_2m,weather_code";
  }
};
