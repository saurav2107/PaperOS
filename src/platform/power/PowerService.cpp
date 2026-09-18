#include "services/Services.h"
#include <M5Unified.h>

// Native power capability. Apps only receive read-only battery state through
// AppContext; no app is allowed to power down the whole OS directly.
void PowerService::begin() {
  inactivityMinutes_ = settings_ ? settings_->inactivityMinutes() : 5;
  lastActivityMs_ = millis();
}

uint8_t PowerService::batteryPercent() const {
  return static_cast<uint8_t>(M5.Power.getBatteryLevel());
}

void PowerService::setInactivityMinutes(uint8_t minutes) {
  inactivityMinutes_ = minutes;
  if (settings_) settings_->setInactivityMinutes(inactivityMinutes_);
  noteActivity();
}

void PowerService::noteActivity() { lastActivityMs_ = millis(); }

void PowerService::setSleepInhibited(bool inhibited) {
  if (sleepInhibited_ == inhibited) return;
  sleepInhibited_ = inhibited;
  // A protected app should never immediately sleep after the user returns to
  // a normal app. Start a fresh inactivity interval at that hand-off.
  if (!inhibited) noteActivity();
}

void PowerService::drawSleepScreen(const char* localTime) {
  (void)localTime;  // A displayed clock would freeze while the CPU is asleep.
  M5.Display.fillScreen(TFT_WHITE);
  // Use an entirely static screen: light sleep pauses normal app rendering,
  // so keeping a clock here would present stale information to the user.
  M5.Display.fillRoundRect(62, 224, 416, 398, 16, TFT_BLACK);
  M5.Display.fillRoundRect(65, 227, 410, 392, 13, TFT_WHITE);
  M5.Display.setTextDatum(MC_DATUM);
  // Bold crescent, visible without relying on a bitmap asset or font glyph.
  M5.Display.fillCircle(270, 316, 42, TFT_BLACK);
  M5.Display.fillCircle(290, 298, 40, TFT_WHITE);
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.drawString("Sleep mode", 270, 394);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.drawString("Device is using low power", 270, 432);
  M5.Display.drawFastHLine(122, 470, 296, TFT_BLACK);
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.drawString(String("BATTERY  ") + batteryPercent() + "%", 270, 514);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.drawString("App state is preserved", 270, 552);
  M5.Display.drawString("Touch the screen to wake", 270, 580);
  M5.Display.setTextDatum(TL_DATUM);
  M5.Display.setFont(nullptr);
}

void PowerService::sleepNow(const char* localTime) {
  drawSleepScreen(localTime ? localTime : "");
  // PaperS3 drawing is asynchronous. Do not enter light sleep until the
  // e-paper controller has committed the sleep screen, otherwise it can first
  // become visible only after a wake touch.
  M5.Display.waitDisplay();
  // PaperS3's M5Unified implementation configures GPIO48 (touch interrupt)
  // as the light-sleep wake source.  RAM and the app registry survive.
  M5.Power.lightSleep(0, true);
  M5.Display.wakeup();
  delay(180);  // do not treat the wake press as a launcher tap.
  noteActivity(); wakeEvent_ = true;
}

void PowerService::tick(const char* localTime) {
  if (sleepInhibited_) return;
  if (inactivityMinutes_ == 0) return;
  const uint32_t timeout = static_cast<uint32_t>(inactivityMinutes_) * 60UL * 1000UL;
  if (millis() - lastActivityMs_ >= timeout) sleepNow(localTime);
}

bool PowerService::consumeWakeEvent() { const bool result = wakeEvent_; wakeEvent_ = false; return result; }

[[noreturn]] void PowerService::powerOff() {
  M5.Display.fillScreen(TFT_WHITE);
  M5.Display.setTextDatum(MC_DATUM); M5.Display.setTextSize(2);
  M5.Display.drawString("Powering off", 270, 470); M5.Display.setTextDatum(TL_DATUM);
  delay(250); M5.Power.powerOff();
  while (true) delay(1000);
}
