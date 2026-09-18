#include "apps/weather/WeatherApp.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <math.h>
#include <time.h>
#include <M5Unified.h>
#include <SD.h>
#include "app/AppContext.h"
#include "config/Location.h"
#include "services/Services.h"
#include "ui/Chrome.h"
#include "ui/Icons.h"
#include "ui/Theme.h"

namespace {
constexpr uint32_t kRefreshMs = 10UL * 60UL * 1000UL;
constexpr uint16_t kInk = TFT_BLACK;
constexpr int kRefreshSize = 30;
constexpr const char* kWeatherIconFolder = "/PaperOS/icons/weather/";

const char* weatherIconFile(uint16_t code) {
  if (code == 0) return "wi-day-sunny.png";
  if (code <= 2) return "wi-day-cloudy.png";
  if (code == 3) return "wi-cloudy.png";
  if (code <= 48) return "wi-fog.png";
  if (code <= 67) return "wi-rain.png";
  if (code <= 77 || code == 85 || code == 86) return "wi-snow.png";
  if (code <= 82) return "wi-showers.png";
  return "wi-thunderstorm.png";
}

String countryCode(const String& value) {
  String country(value); country.trim(); country.toUpperCase();
  // Retain a user-entered ISO country code. Common full country names are
  // normalised here so the compact landscape header is always readable.
  if (country.length() == 2 || country.length() == 3) return country;
  if (country == "INDIA") return "IN";
  if (country == "UNITED STATES" || country == "UNITED STATES OF AMERICA") return "US";
  if (country == "UNITED KINGDOM") return "GB";
  if (country == "CANADA") return "CA";
  if (country == "AUSTRALIA") return "AU";
  if (country == "GERMANY") return "DE";
  if (country == "FRANCE") return "FR";
  if (country == "JAPAN") return "JP";
  return country;
}

String weatherLocationLabel(const DeviceLocation& location) {
  const String code = countryCode(location.country);
  if (location.city.isEmpty()) return code;
  return code.isEmpty() ? location.city : location.city + ", " + code;
}

ui::ChromeOptions weatherChrome(uint8_t dayIndex) {
  ui::ChromeOptions options;
  options.showBack = false;
  options.showHome = true;
  options.showPrevious = dayIndex > 0;
  options.showNext = dayIndex < 2;
  return options;
}
int weatherTop(bool fullScreen) { return fullScreen ? 0 : ui::Chrome::headerHeight(); }
int weatherBottom(bool fullScreen) { return fullScreen ? M5.Display.height() : ui::Chrome::footerTop(); }
}

bool WeatherApp::fetch(AppContext& context) {
  // Queue HTTPS work on WeatherFetchService. The UI task immediately returns
  // to touch/navigation handling; onTick consumes its completed plain-data
  // result and redraws only then.
  lastAttemptMs_ = millis();
  if (!context.weatherFetch.request(context.settings.location())) { status_ = context.network.connected() ? "Weather update is already running" : "Wi-Fi is offline. Use Settings to connect."; return false; }
  status_ = "Updating weather…";
  return true;
}

const char* WeatherApp::condition(uint16_t code) const {
  if (code == 0) return "Clear";
  if (code <= 2) return "Partly cloudy";
  if (code == 3) return "Overcast";
  if (code <= 48) return "Fog";
  if (code <= 67) return "Rain";
  if (code <= 77) return "Snow";
  if (code <= 82) return "Showers";
  return "Thunderstorm";
}

bool WeatherApp::onStart(AppContext& context) {
  context.display.setLandscape();
  dayIndex_ = 0; fullScreen_ = pendingFaceChange_ = false; lastTapMs_ = 0;
  fetch(context);
  draw(context);
  return true;
}

void WeatherApp::onStop(AppContext& context) { fullScreen_ = pendingFaceChange_ = false; context.display.setPortrait(); }

bool WeatherApp::drawSdWeatherIcon(int x, int y, int size, uint16_t code) const {
  // PNG is deliberately used rather than SVG: M5GFX has a reliable PNG
  // decoder, while it has no SVG decoder. Each supplied source icon was
  // rasterised at 96px and retains its original name on the SD card.
  const String path = String(kWeatherIconFolder) + weatherIconFile(code);
  if (!SD.exists(path)) return false;
  const float scale = static_cast<float>(size) / 96.0f;
  File png = SD.open(path, FILE_READ);
  if (!png) return false;
  const bool drawn = M5.Display.drawPng(&png, x - size / 2, y - size / 2,
                                        0, 0, 0, 0, scale, scale);
  png.close();
  return drawn;
}

void WeatherApp::drawWeatherIcon(int x, int y, int size, uint16_t code) const {
  if (drawSdWeatherIcon(x, y, size, code)) return;
  // Visual mapping comes from the supplied Weather Icons set:
  // wi-day-sunny, wi-day-cloudy, wi-cloudy, wi-fog, wi-rain,
  // wi-snow, wi-showers and wi-thunderstorm.  Those files are SVGs; M5GFX
  // has no SVG renderer, so this is an embedded monochrome rendering of the
  // same recognisable silhouettes.  It avoids a slow SD-card read and keeps
  // the glyphs crisp at the three sizes used by the landscape Weather UI.
  const int r = max(5, size / 4);
  const int stroke = max(1, size / 30);
  const bool clear = code == 0;
  const bool partly = code == 1 || code == 2;
  const bool cloudy = code == 3;
  const bool fog = code >= 45 && code <= 48;
  const bool snow = code >= 71 && code <= 77 || code >= 85 && code <= 86;
  const bool storm = code >= 95;
  const bool rain = code >= 51 && code <= 67 || code >= 80 && code <= 82;

  const auto line = [&](int x0, int y0, int x1, int y1) {
    for (int offset = -(stroke - 1) / 2; offset <= stroke / 2; ++offset) {
      M5.Display.drawLine(x0 + offset, y0, x1 + offset, y1, kInk);
    }
  };
  const auto circle = [&](int cx, int cy, int radius) {
    for (int offset = 0; offset < stroke; ++offset) M5.Display.drawCircle(cx, cy, radius - offset, kInk);
  };
  const auto cloud = [&]() {
    // The three lobes and broad base match the supplied wi-cloudy outline.
    circle(x - r + 4, y + 2, max(3, r / 2));
    circle(x - r / 5, y - r / 3, max(4, r * 2 / 3));
    circle(x + r / 2, y + 1, max(4, r * 3 / 5));
    line(x - r - 1, y + r / 2, x + r + 2, y + r / 2);
  };
  const auto sun = [&](int cx, int cy) {
    circle(cx, cy, max(4, r / 2));
    const int rayStart = max(6, r / 2 + 3), rayEnd = rayStart + max(4, r / 3);
    for (int i = 0; i < 8; ++i) {
      const float a = i * PI / 4.0f;
      line(cx + static_cast<int>(cosf(a) * rayStart), cy + static_cast<int>(sinf(a) * rayStart),
           cx + static_cast<int>(cosf(a) * rayEnd), cy + static_cast<int>(sinf(a) * rayEnd));
    }
  };

  if (clear) { sun(x, y); return; }
  if (partly) sun(x - r / 2, y - r / 2);
  if (!clear) cloud();
  if (cloudy || partly) return;

  if (fog) {
    const int fogWidth = r * 2 + 6;
    for (int i = 0; i < 3; ++i) line(x - fogWidth / 2 + (i & 1 ? 3 : 0), y + r + 6 + i * (stroke + 4),
                                      x + fogWidth / 2 - (i & 1 ? 3 : 0), y + r + 6 + i * (stroke + 4));
    return;
  }
  if (snow) {
    for (int i = -1; i <= 1; ++i) {
      const int sx = x + i * max(7, r * 3 / 4), sy = y + r + 8;
      line(sx - 4, sy, sx + 4, sy);
      line(sx, sy - 4, sx, sy + 4);
      line(sx - 3, sy - 3, sx + 3, sy + 3);
      line(sx - 3, sy + 3, sx + 3, sy - 3);
    }
  } else if (rain) {
    for (int i = -1; i <= 1; ++i) {
      const int rx = x + i * max(7, r * 3 / 4), ry = y + r + 5;
      line(rx + 2, ry, rx - 3, ry + max(7, r / 2 + 2));
    }
  }
  if (storm) {
    // Thick lightning bolt, deliberately separate from the rain strokes so
    // it remains legible even in the 26px hourly cells.
    const int top = y + r - 1;
    line(x + 5, top, x - 4, top + r / 2 + 5);
    line(x - 4, top + r / 2 + 5, x + 2, top + r / 2 + 5);
    line(x + 2, top + r / 2 + 5, x - 5, top + r + 12);
  }
}

void WeatherApp::drawOffline(AppContext& context) {
  const int w = M5.Display.width();
  const int top = weatherTop(fullScreen_);
  const int bottom = weatherBottom(fullScreen_);
  M5.Display.setTextDatum(MC_DATUM);
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.drawString("DEVICE IS OFFLINE", w / 2, top + (bottom - top) / 2 - 26);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.drawString("CONNECT WI-FI IN SETTINGS TO UPDATE WEATHER", w / 2, top + (bottom - top) / 2 + 10);
  M5.Display.drawString(String("NEXT CHECK: UP TO ") + context.weatherSettings.offlineRetryMinutes() + " MINUTES", w / 2, top + (bottom - top) / 2 + 38);
  M5.Display.setTextDatum(TL_DATUM);
  M5.Display.setFont(nullptr);
}

void WeatherApp::drawDashboard(AppContext& context) {
  const int w = M5.Display.width();
  const int top = weatherTop(fullScreen_);
  const int bottom = weatherBottom(fullScreen_);
  const int left = 12;
  const int currentBottom = top + 130;
  const int hourlyLabelY = currentBottom + 15;
  const int hourlyTop = currentBottom + 29;
  const int hourlyH = 94;
  const int dailyLabelY = hourlyTop + hourlyH + 15;
  const int dailyTop = hourlyTop + hourlyH + 29;
  const int dailyH = bottom - dailyTop - 10;

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.drawString(weatherLocationLabel(context.settings.location()), left+8, top + 17);
  M5.Display.drawRoundRect(w - kRefreshSize - left, top + 5, kRefreshSize, kRefreshSize, 5, kInk);
  ui::drawIcon(ui::Icon::Refresh, w - kRefreshSize / 2 - left, top + 20, 20);
  M5.Display.setFont(nullptr); M5.Display.setTextSize(6);
  M5.Display.drawString(context.temperature.format(weather_.temperature), left + 8, top + 58);
  M5.Display.setTextSize(1);
  drawWeatherIcon(left + 192, top + 77, 50, weather_.weatherCode);
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.drawString(condition(weather_.weatherCode), left + 238, top + 58);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.drawString("Feels " + context.temperature.format(weather_.apparentTemperature), left + 238, top + 87);
  M5.Display.drawString("Today " + context.temperature.format(weather_.daily[0].minimum) + " / " + context.temperature.format(weather_.daily[0].maximum), left + 238, top + 111);
  const int statX = w - 330;
  M5.Display.drawString("Humidity  " + String(weather_.humidity, 0) + "%", statX, top + 55);
  M5.Display.drawString("Wind      " + String(weather_.windSpeed, 1) + " km/h", statX, top + 82);
  M5.Display.drawString("Rain      " + String(weather_.precipitation, 1) + " mm", statX, top + 109);
  M5.Display.drawString("Sunrise  " + weather_.sunrise, w - 150, top + 55);
  M5.Display.drawString("Sunset   " + weather_.sunset, w - 150, top + 82);
  M5.Display.drawFastHLine(0, currentBottom, w, kInk);

  M5.Display.drawString("Next 8 hours:", left+8, hourlyLabelY-5);
  const int cellW = w / 8;
  tm now{}; getLocalTime(&now, 10);
  for (int i = 0; i < 8; ++i) {
    const int x = i * cellW;
    M5.Display.drawRect(x, hourlyTop, cellW + 1, hourlyH, kInk);
    char hour[8]; snprintf(hour, sizeof(hour), "%02d:00", (now.tm_hour + i) % 24);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.drawString(hour, x + cellW / 2, hourlyTop + 14);
    drawWeatherIcon(x + cellW / 2, hourlyTop + 45, 26, weather_.hourly[i].weatherCode);
    M5.Display.drawString(context.temperature.format(weather_.hourly[i].temperature), x + cellW / 2, hourlyTop + 78);
    M5.Display.setTextDatum(TL_DATUM);
  }
  const DailyWeather& day = weather_.daily[dayIndex_];
  M5.Display.drawString("Day forecast  " + day.date, left+8, dailyLabelY-5);
  M5.Display.drawRoundRect(left, dailyTop, w - left * 2, dailyH, 8, kInk);
  M5.Display.setTextDatum(MC_DATUM);
  drawWeatherIcon(w / 2 - 180, dailyTop + dailyH / 2, 44, day.weatherCode);
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.drawString(condition(day.weatherCode), w / 2 - 42, dailyTop + 26);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.drawString("Low  " + context.temperature.format(day.minimum) + "     High  " + context.temperature.format(day.maximum), w / 2 + 65, dailyTop + 58);
  M5.Display.drawString("Rain Probability  " + String(day.precipitationProbability) + "%", w / 2 + 65, dailyTop + 86);
  M5.Display.setTextDatum(TL_DATUM);
  M5.Display.setFont(nullptr);
}

void WeatherApp::drawMinimal(AppContext& context) {
  const int w=M5.Display.width(), top=weatherTop(fullScreen_), bottom=weatherBottom(fullScreen_);
  M5.Display.setTextDatum(MC_DATUM); M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.drawString(weatherLocationLabel(context.settings.location()),w/2,top+26); M5.Display.drawString("MINIMAL",w/2,top+52);
  M5.Display.setFont(nullptr); M5.Display.setTextSize(13); M5.Display.drawString(context.temperature.format(weather_.temperature),w/2,top+150); M5.Display.setTextSize(1);
  drawWeatherIcon(w/2,top+235,88,weather_.weatherCode); M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.drawString(condition(weather_.weatherCode),w/2,top+310);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.drawString("FEELS " + context.temperature.format(weather_.apparentTemperature),w/2,top+344);
  M5.Display.drawString("Humidity " + String(weather_.humidity,0) + "%   Wind " + String(weather_.windSpeed,1) + " km/h",w/2,bottom-55);
  M5.Display.drawString("Tap content to change face",w/2,bottom-25); M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}

void WeatherApp::drawHourly(AppContext& context) {
  const int w=M5.Display.width(), top=weatherTop(fullScreen_), bottom=weatherBottom(fullScreen_), cellW=w/4, cellH=(bottom-top-58)/2;
  M5.Display.setTextDatum(MC_DATUM); M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.drawString("Hourly Forecast",w/2,top+24); M5.Display.setFont(nullptr);
  tm now{}; getLocalTime(&now,10);
  for(int i=0;i<8;++i){const int row=i/4,col=i%4,x=col*cellW,y=top+46+row*cellH;M5.Display.drawRect(x,y,cellW+1,cellH+1,TFT_BLACK);char hour[8];snprintf(hour,sizeof(hour),"%02d:00",(now.tm_hour+i)%24);M5.Display.setTextDatum(MC_DATUM);M5.Display.setFont(&fonts::FreeSans9pt7b);M5.Display.drawString(hour,x+cellW/2,y+25);drawWeatherIcon(x+cellW/2,y+cellH/2,52,weather_.hourly[i].weatherCode);M5.Display.setFont(nullptr);M5.Display.setTextSize(5);M5.Display.drawString(context.temperature.format(weather_.hourly[i].temperature),x+cellW/2,y+cellH-28);M5.Display.setTextSize(1);}
  M5.Display.setTextDatum(TL_DATUM);
}

void WeatherApp::drawDaily(AppContext& context) {
  const int w=M5.Display.width(), top=weatherTop(fullScreen_), bottom=weatherBottom(fullScreen_); const DailyWeather& day=weather_.daily[dayIndex_];
  M5.Display.setTextDatum(MC_DATUM); M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.drawString("Daily Forecast  " + day.date,w/2,top+30);
  M5.Display.drawRoundRect(70,top+58,w-140,bottom-top-100,14,TFT_BLACK); drawWeatherIcon(w/2,top+90,100,day.weatherCode);
  M5.Display.setFont(&fonts::FreeSans12pt7b);M5.Display.drawString(condition(day.weatherCode),w/2,top+185);
  M5.Display.setFont(nullptr);M5.Display.setTextSize(7);M5.Display.drawString(context.temperature.format(day.minimum)+"  /  "+context.temperature.format(day.maximum),w/2,top+245);M5.Display.setTextSize(1);
  M5.Display.setFont(&fonts::FreeSans9pt7b);M5.Display.drawString("Rain Probability " + String(day.precipitationProbability) + "%",w/2,top+330);M5.Display.drawString("Use PREV / NEXT for another day",w/2,top+360);M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);
}

void WeatherApp::drawTodayCard(AppContext& context) {
  const int w = M5.Display.width(), top = weatherTop(fullScreen_), bottom = weatherBottom(fullScreen_);
  const int cardX = 42, cardY = top + 36, cardW = w - 84, cardH = bottom - cardY - 24;
  const int leftColumnX = cardX + cardW / 4, rightColumnX = cardX + cardW * 3 / 4;
  // Reserve the lower third for the three weather facts.  This guarantees
  // their values remain inside the card, above the persistent footer.
  const int factsRuleY = cardY + cardH - 104;
  const int factsLabelY = factsRuleY + 36, factsValueY = factsRuleY + 72;
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString(weatherLocationLabel(context.settings.location()), w / 2, top + 20);
  M5.Display.drawRoundRect(cardX, cardY, cardW, cardH, 12, kInk);
  // Balanced two-column summary: icon on the left; temperature and condition
  // are centred as a group in the right half. This also centres “Overcast”.
  drawWeatherIcon(leftColumnX, cardY + 122, 104, weather_.weatherCode);
  M5.Display.setFont(nullptr); M5.Display.setTextSize(10);
  M5.Display.drawString(context.temperature.format(weather_.temperature), rightColumnX, cardY + 92);
  M5.Display.setTextSize(1); M5.Display.setFont(&fonts::FreeSans18pt7b);
  M5.Display.drawString(condition(weather_.weatherCode), rightColumnX, cardY + 156);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.drawString("Feels " + context.temperature.format(weather_.apparentTemperature), rightColumnX, cardY + 201);
  M5.Display.drawString("High " + context.temperature.format(weather_.daily[0].maximum) + "   Low " + context.temperature.format(weather_.daily[0].minimum), rightColumnX, cardY + 231);
  M5.Display.drawFastHLine(cardX + 36, factsRuleY, cardW - 72, kInk);
  M5.Display.drawString("Humidity", cardX + cardW / 6, factsLabelY);
  M5.Display.drawString(String(weather_.humidity, 0) + "%", cardX + cardW / 6, factsValueY);
  M5.Display.drawString("Wind", w / 2, factsLabelY);
  M5.Display.drawString(String(weather_.windSpeed, 1) + " km/h", w / 2, factsValueY);
  M5.Display.drawString("Rain", cardX + cardW * 5 / 6, factsLabelY);
  M5.Display.drawString(String(weather_.precipitation, 1) + " mm", cardX + cardW * 5 / 6, factsValueY);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}

void WeatherApp::drawTimeline(AppContext& context) {
  const int w = M5.Display.width(), top = weatherTop(fullScreen_), bottom = weatherBottom(fullScreen_);
  const int left = 34, right = w - 34, chartTop = top + 116, chartBottom = bottom - 72;
  tm now{}; getLocalTime(&now, 10);
  float low = weather_.hourly[0].temperature, high = low;
  for (const auto& hour : weather_.hourly) { low = min(low, hour.temperature); high = max(high, hour.temperature); }
  if (high - low < 2.0f) { low -= 1.0f; high += 1.0f; }
  M5.Display.setTextDatum(MC_DATUM); M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.drawString("Next 8 Hours", w / 2, top + 28);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.drawString("Temperature Timeline", w / 2, top + 61);
  M5.Display.drawRect(left, chartTop, right - left, chartBottom - chartTop, kInk);
  const int step = (right - left - 28) / 7;
  int previousX = 0, previousY = 0;
  for (int i = 0; i < 8; ++i) {
    const int x = left + 14 + step * i;
    const int y = chartBottom - 72 - static_cast<int>((weather_.hourly[i].temperature - low) * (chartBottom - chartTop - 150) / (high - low));
    if (i) M5.Display.drawLine(previousX, previousY, x, y, kInk);
    M5.Display.fillCircle(x, y, 4, kInk);
    char label[8]; snprintf(label, sizeof(label), "%02d:00", (now.tm_hour + i) % 24);
    M5.Display.drawString(label, x, chartBottom - 42);
    M5.Display.drawString(context.temperature.format(weather_.hourly[i].temperature), x, y - 22);
    drawWeatherIcon(x, chartBottom - 92, 30, weather_.hourly[i].weatherCode);
    previousX = x; previousY = y;
  }
  M5.Display.drawString("Low " + context.temperature.format(low) + "    High " + context.temperature.format(high), w / 2, bottom - 28);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}

void WeatherApp::drawThreeDay(AppContext& context) {
  const int w = M5.Display.width(), top = weatherTop(fullScreen_), bottom = weatherBottom(fullScreen_);
  const int gap = 16, cardW = (w - 56 - gap * 2) / 3, cardH = bottom - top - 90, startX = 28, y = top + 48;
  M5.Display.setTextDatum(MC_DATUM); M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.drawString("3-Day Forecast", w / 2, top + 25);
  for (int i = 0; i < 3; ++i) {
    const DailyWeather& day = weather_.daily[i]; const int x = startX + i * (cardW + gap);
    // The shared filled frame keeps all four sides intact during e-paper
    // updates; single-pixel drawRoundRect borders were losing their bottom
    // and right edges on this view.
    ui::Theme::drawFrame(x, y, cardW, cardH);
    M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.drawString(day.date, x + cardW / 2, y + 28);
    drawWeatherIcon(x + cardW / 2, y + 112, 68, day.weatherCode);
    M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.drawString(condition(day.weatherCode), x + cardW / 2, y + 176);
    M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.drawString("High " + context.temperature.format(day.maximum), x + cardW / 2, y + 224);
    M5.Display.drawString("Low  " + context.temperature.format(day.minimum), x + cardW / 2, y + 258);
    M5.Display.drawFastHLine(x + 20, y + cardH - 62, cardW - 40, kInk);
    // The rain value is anchored to the card bottom with an inset rather
    // than a fixed screen coordinate, so it never lands on the frame edge.
    M5.Display.drawString("Rain " + String(day.precipitationProbability) + "%", x + cardW / 2, y + cardH - 30);
  }
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}

void WeatherApp::drawWeatherClock(AppContext& context) {
  const int w = M5.Display.width(), top = weatherTop(fullScreen_), bottom = weatherBottom(fullScreen_);
  const int cx = w / 2 - 115, cy = (top + bottom) / 2 + 10, r = min(170, (bottom - top) / 2 - 26);
  tm now{}; getLocalTime(&now, 10);
  M5.Display.setTextDatum(MC_DATUM); M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.drawString("Weather Clock", w / 2, top + 25);
  for (int line = 0; line < 3; ++line) M5.Display.drawCircle(cx, cy, r - line, kInk);
  for (int i = 0; i < 12; ++i) {
    const float angle = i * PI / 6.0f - PI / 2.0f;
    const int x0 = cx + static_cast<int>(cosf(angle) * (r - 16)); const int y0 = cy + static_cast<int>(sinf(angle) * (r - 16));
    const int x1 = cx + static_cast<int>(cosf(angle) * (r - 5)); const int y1 = cy + static_cast<int>(sinf(angle) * (r - 5));
    M5.Display.drawLine(x0, y0, x1, y1, kInk);
  }
  const float minute = now.tm_min * PI / 30.0f - PI / 2.0f;
  const float hour = (now.tm_hour % 12 + now.tm_min / 60.0f) * PI / 6.0f - PI / 2.0f;
  M5.Display.drawLine(cx, cy, cx + static_cast<int>(cosf(hour) * (r * .50f)), cy + static_cast<int>(sinf(hour) * (r * .50f)), kInk);
  M5.Display.drawLine(cx + 1, cy, cx + 1 + static_cast<int>(cosf(hour) * (r * .50f)), cy + static_cast<int>(sinf(hour) * (r * .50f)), kInk);
  M5.Display.drawLine(cx, cy, cx + static_cast<int>(cosf(minute) * (r * .74f)), cy + static_cast<int>(sinf(minute) * (r * .74f)), kInk);
  M5.Display.fillCircle(cx, cy, 6, kInk);
  const int panelX = w - 225;
  M5.Display.drawRoundRect(panelX, cy - 146, 190, 292, 12, kInk);
  drawWeatherIcon(panelX + 95, cy - 78, 76, weather_.weatherCode);
  M5.Display.setFont(nullptr); M5.Display.setTextSize(7); M5.Display.drawString(context.temperature.format(weather_.temperature), panelX + 95, cy + 10); M5.Display.setTextSize(1);
  M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.drawString(condition(weather_.weatherCode), panelX + 95, cy + 70);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.drawString("High " + context.temperature.format(weather_.daily[0].maximum) + "  Low " + context.temperature.format(weather_.daily[0].minimum), panelX + 95, cy + 108);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}

void WeatherApp::draw(AppContext& context) {
  M5.Display.clear();
  const ui::ChromeOptions options = weatherChrome(dayIndex_);
  if (!fullScreen_) {
    const String title = String("Weather  ") + weatherLocationLabel(context.settings.location());
    ui::Chrome::drawHeader(context, title.c_str());
  }
  if (!context.network.connected()) drawOffline(context);
  else if (loaded_) {
    if (context.weatherFace.face() != WeatherFace::Dashboard) {
      const int x=M5.Display.width()-kRefreshSize-12,y=weatherTop(fullScreen_)+5;
      M5.Display.drawRoundRect(x,y,kRefreshSize,kRefreshSize,5,kInk);
      ui::drawIcon(ui::Icon::Refresh,x+kRefreshSize/2,y+kRefreshSize/2,20);
    }
    switch (context.weatherFace.face()) {
      case WeatherFace::Minimal: drawMinimal(context); break;
      case WeatherFace::Hourly: drawHourly(context); break;
      case WeatherFace::Daily: drawDaily(context); break;
      case WeatherFace::TodayCard: drawTodayCard(context); break;
      case WeatherFace::Timeline: drawTimeline(context); break;
      case WeatherFace::ThreeDay: drawThreeDay(context); break;
      case WeatherFace::WeatherClock: drawWeatherClock(context); break;
      default: drawDashboard(context); break;
    }
  }
  else { M5.Display.setTextSize(2); M5.Display.setCursor(25, weatherTop(fullScreen_) + 40); M5.Display.print(status_); }
  if (!fullScreen_) ui::Chrome::drawFooter(options);
}

void WeatherApp::handleTouch(AppContext& context, int x, int y) {
  const ui::ChromeOptions options = weatherChrome(dayIndex_);
  const auto footer = fullScreen_ ? ui::FooterAction::None : ui::Chrome::hitTestFooter(x, y, options);
  if (footer == ui::FooterAction::Home) { if (navigator_) navigator_(AppId::Launcher); }
  else if (footer == ui::FooterAction::Previous && dayIndex_ > 0) { --dayIndex_; draw(context); }
  else if (footer == ui::FooterAction::Next && dayIndex_ < 2) { ++dayIndex_; draw(context); }
  else if (x >= M5.Display.width() - kRefreshSize - 18 && x <= M5.Display.width() - 8 &&
           y >= weatherTop(fullScreen_) + 3 && y <= weatherTop(fullScreen_) + kRefreshSize + 7) { fetch(context); draw(context); }
}

void WeatherApp::onTick(AppContext& context, uint32_t nowMs) {
  const auto& touch = M5.Touch.getDetail();
  if (touch.wasPressed()) {
    const bool footerTap = !fullScreen_ && touch.y >= ui::Chrome::footerTop();
    const bool refreshTap = touch.x >= M5.Display.width() - kRefreshSize - 18 && touch.x <= M5.Display.width() - 8 &&
                            touch.y >= weatherTop(fullScreen_) + 3 && touch.y <= weatherTop(fullScreen_) + kRefreshSize + 7;
    // Navigation and the explicit refresh affordance stay immediate. Content
    // taps participate in the Photos-style double-tap fullscreen gesture.
    if (footerTap || refreshTap) { handleTouch(context, touch.x, touch.y); return; }
    if (lastTapMs_ && nowMs - lastTapMs_ < 380UL) {
      fullScreen_ = !fullScreen_;
      pendingFaceChange_ = false;
      lastTapMs_ = 0;
      draw(context);
      return;
    }
    lastTapMs_ = nowMs;
    pendingFaceChange_ = !fullScreen_;
  }
  else if (touch.wasReleased()) {
    const auto action=ui::Chrome::swipeAction(touch.base_x,touch.base_y,touch.x,touch.y,weatherChrome(dayIndex_));
    if(action==ui::FooterAction::Previous&&dayIndex_>0){pendingFaceChange_=false;lastTapMs_=0;--dayIndex_;draw(context);}
    else if(action==ui::FooterAction::Next&&dayIndex_<2){pendingFaceChange_=false;lastTapMs_=0;++dayIndex_;draw(context);}
  }
  if (pendingFaceChange_ && nowMs - lastTapMs_ >= 380UL) {
    pendingFaceChange_ = false;
    context.weatherFace.next();
    draw(context);
  }
  WeatherFetchResult result;
  if (context.weatherFetch.consume(result)) {
    if (result.success) {
      weather_.temperature=result.temperature; weather_.apparentTemperature=result.apparentTemperature; weather_.humidity=result.humidity; weather_.windSpeed=result.windSpeed; weather_.windDirection=result.windDirection; weather_.precipitation=result.precipitation; weather_.weatherCode=result.weatherCode;
      weather_.sunrise=result.sunrise; weather_.sunset=result.sunset;
      for(int i=0;i<8;++i){weather_.hourly[i].temperature=result.hourlyTemperature[i];weather_.hourly[i].weatherCode=result.hourlyCode[i];}
      for(int i=0;i<3;++i){weather_.daily[i].minimum=result.dailyMinimum[i];weather_.daily[i].maximum=result.dailyMaximum[i];weather_.daily[i].weatherCode=result.dailyCode[i];weather_.daily[i].precipitationProbability=result.dailyRain[i];weather_.daily[i].date=result.dailyDate[i];}
      context.temperature.setLastCelsius(weather_.temperature); context.temperature.setLastHumidity(weather_.humidity); loaded_=true; lastFetchMs_=nowMs; status_="Updated from Open-Meteo";
    } else status_=result.error[0]?result.error:"Weather update failed";
    draw(context);
  }
  const uint32_t offlineRetryMs = static_cast<uint32_t>(context.weatherSettings.offlineRetryMinutes()) * 60UL * 1000UL;
  const uint32_t retryDelay = (!context.network.connected() || !loaded_) ? offlineRetryMs : kRefreshMs;
  if (!context.weatherFetch.inProgress() && nowMs - lastAttemptMs_ >= retryDelay) { fetch(context); draw(context); }
}
