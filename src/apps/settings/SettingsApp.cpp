#include "apps/settings/SettingsApp.h"
#include <cstring>
#include <M5Unified.h>
#include <WiFi.h>
#include "app/AppContext.h"
#include "services/Services.h"
#include "ui/Chrome.h"
#include "ui/Icons.h"
#include "ui/Popup.h"
#include "ui/Theme.h"
#include "ui/Typography.h"

namespace {
constexpr int kRootFirstRow = 112;
constexpr int kRootRowHeight = 82;
// Root Settings is a launcher-level page. Detail pages are children of it, so
// they must expose BACK rather than HOME to preserve the expected hierarchy.
void settingsFooter(bool detailPage = false) {
  ui::ChromeOptions chrome;
  chrome.showBack = detailPage;
  chrome.showHome = !detailPage;
  ui::Chrome::drawFooter(chrome);
}

// A calm list layout reads substantially better than seven competing card
// outlines on reflective e-paper. The 2px divider stays crisp without making
// the page feel boxed in, while the 30px glyph gives each setting a clear
// recognition point before the user reads its label.
void settingsListRow(int index, ui::Icon icon, const char* label, const String& value, bool toggle = false) {
  const int y = kRootFirstRow + index * kRootRowHeight;
  ui::drawIcon(icon, 58, y + 31, ui::Theme::ListIconSize);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.setTextDatum(ML_DATUM);
  M5.Display.drawString(label, 92, y + 27);
  if (toggle) {
    const bool on = value == "ON";
    // Match the wider AP-mode control exactly: a single toggle language for
    // every Settings page rather than a smaller, unrelated radio-style pill.
    M5.Display.fillRoundRect(400, y + 10, 88, 36, 18, TFT_BLACK);
    M5.Display.fillCircle(on ? 468 : 420, y + 28, 14, TFT_WHITE);
    M5.Display.setFont(&fonts::FreeSansBold9pt7b); M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.drawString(on ? "YES" : "NO", on ? 430 : 462, y + 28);
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  } else if (value == ">") {
    // One filled chevron means “opens another page” everywhere in PaperOS.
    ui::drawIcon(ui::Icon::ArrowRight, 484, y + 28, ui::Theme::NextIconSize);
  } else if (!value.isEmpty()) {
    M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(MR_DATUM);
    M5.Display.drawString(value, 494, y + 27);
  }
  // Stronger divider: each setting remains a clearly separated line-button
  // while retaining the cleaner list treatment instead of heavy card boxes.
  M5.Display.fillRect(48, y + 64, 444, ui::Theme::Border, TFT_BLACK);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}
// Shared detail-page row. Every Settings child uses the same visual grammar
// as the landing page: icon, readable title, quiet secondary value, divider.
void settingsDetailRow(int y, ui::Icon icon, const String& label, const String& value) {
  M5.Display.fillRect(28, y, 484, 82, TFT_WHITE);
  ui::drawIcon(icon, 58, y + 31, ui::Theme::ListIconSize);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE); M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.setTextDatum(ML_DATUM);
  M5.Display.drawString(label, 92, y + 25);
  if (value == ">") {
    // Detail pages use the same aligned disclosure chevron as the Settings
    // root, rather than rendering a literal '>' under the row label.
    ui::drawIcon(ui::Icon::ArrowRight, 484, y + 31, ui::Theme::NextIconSize);
  } else {
    M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.drawString(value, 92, y + 53);
  }
  M5.Display.fillRect(48, y + 70, 444, ui::Theme::Border, TFT_BLACK);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}
}
bool SettingsApp::onStart(AppContext& context) { draw(context); return true; }
void SettingsApp::draw(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE); ui::Chrome::drawHeader(context,"SETTINGS"); settingsFooter();
  settingsListRow(0, ui::Icon::Wifi, "Wi-Fi & Network", context.network.connected() ? "CONNECTED" : "SET UP");
  settingsListRow(1, ui::Icon::Home, "Home Address", ">");
  settingsListRow(2, ui::Icon::Clock, "Clock & Time", context.time.timezone());
  settingsListRow(3, ui::Icon::Weather, "Weather Updates", String(context.weatherSettings.offlineRetryMinutes()) + " MIN");
  settingsListRow(4, ui::Icon::Sleep, "Power & Sleep", context.power.inactivityMinutes() ? String(context.power.inactivityMinutes()) + " MIN" : "NEVER");
  settingsListRow(5, ui::Icon::Timer, "Button Sounds", context.uiSound.enabled() ? "ON" : "OFF", true);
  settingsListRow(6, ui::Icon::System, "System & Recovery", ">");
  settingsListRow(7, ui::Icon::Note, "Display & Font", String(static_cast<int>(ui::Typography::scale() * 100)) + "%");
  settingsListRow(8, ui::Icon::Flashcards, "Flashcard AI", context.settings.geminiApiKey().isEmpty() ? "SET UP" : "READY");
}
void SettingsApp::drawWifiSettings(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE); ui::Chrome::drawHeader(context,"Wi-Fi & Network"); settingsFooter(true);
  settingsDetailRow(104,ui::Icon::Wifi,"Connection",context.network.connected()?"Connected":"Not connected");
  const String ssid=context.network.configuredSsid();
  settingsDetailRow(194,ui::Icon::Wifi,"Saved network",ssid.isEmpty()?"No Wi-Fi network saved":ssid);
  settingsDetailRow(284,ui::Icon::Wifi,"Signal",context.network.connected()?String(context.network.signalStrength())+" dBm":"Unavailable while offline");
  settingsDetailRow(374,ui::Icon::Wifi,"Search Wi-Fi",">");
  drawWifiApSection(context);
}
void SettingsApp::drawWifiApSection(AppContext& context) {
  // AP Mode is explicit and reversible. It only runs when the user enables
  // it, avoiding an unnecessary always-on access point and battery drain.
  const int y=464; M5.Display.fillRect(28,y,484,250,TFT_WHITE);
  settingsDetailRow(y,ui::Icon::Wifi,"AP Mode","");
  drawWifiApToggle(context);
  drawWifiApHelp(context.network.provisioning());
}
void SettingsApp::drawWifiApToggle(AppContext& context) {
  constexpr int y=464;
  const bool on=context.network.provisioning();
  // The wider control carries an explicit state rather than an ambiguous
  // Yes/No label, while retaining the single Paper OS toggle language. Its
  // 41px height gives the Enabled/Disabled text comfortable vertical space.
  M5.Display.fillRoundRect(344,y+8,144,41,20,TFT_BLACK);M5.Display.fillCircle(on?466:366,y+28,16,TFT_WHITE);
  M5.Display.setFont(&fonts::FreeSansBold9pt7b);M5.Display.setTextColor(TFT_WHITE,TFT_BLACK);M5.Display.setTextDatum(MC_DATUM);M5.Display.drawString(on?"Enabled":"Disabled",on?406:431,y+28);M5.Display.setTextColor(TFT_BLACK,TFT_WHITE);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}
void SettingsApp::drawWifiApHelp(bool enabled) {
  constexpr int y=464;
  // This is independently cleared so switching AP mode does not redraw the
  // settings rows above it. It stays empty while AP Mode is disabled.
  M5.Display.fillRect(28,y+82,484,ui::Chrome::footerTop()-(y+82),TFT_WHITE);
  if (!enabled) return;
  M5.Display.setFont(&fonts::FreeSansBold12pt7b);M5.Display.setTextDatum(ML_DATUM);M5.Display.drawString("HOW TO USE AP MODE",48,572);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.drawString(String("Wi-Fi: ") + NetworkService::apSsid(),48,605);
  M5.Display.setFont(&fonts::FreeSansBold9pt7b);
  M5.Display.drawString(String("Password: ") + NetworkService::apPassword(),48,635);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.drawString("Open: http://192.168.4.1",48,665);
  M5.Display.drawString("Connect your phone or laptop via 2.4 GHz.",48,695);
  // Encode the same credentials used by softAP. Escape Wi-Fi QR delimiters
  // so future password changes cannot alter the payload's field structure.
  const auto escapeWifi = [](const char* value) {
    String escaped;
    for (const char* p = value; *p; ++p) {
      if (*p == '\\' || *p == ';' || *p == ',' || *p == ':' || *p == '"') escaped += '\\';
      escaped += *p;
    }
    return escaped;
  };
  const String payload = String("WIFI:T:WPA;S:") + escapeWifi(NetworkService::apSsid())
      + ";P:" + escapeWifi(NetworkService::apPassword()) + ";;";
  // Native monochrome modules and a four-module quiet zone keep the code
  // crisp and scannable without an image asset or extra QR dependency.
  M5.Display.qrcode(payload.c_str(),48,718,164,4,true);
  M5.Display.setFont(&fonts::FreeSansBold9pt7b);
  M5.Display.drawString("Scan to join Wi-Fi",230,768);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.drawString("Use your phone camera",230,799);
  M5.Display.drawString("Then open the URL above",230,829);
  M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);
}

void SettingsApp::drawWifiScan(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE); ui::Chrome::drawHeader(context, "Select Wi-Fi"); settingsFooter(true);
  const int count = context.network.scanResultCount();
  lastWifiScanCount_ = context.network.scanInProgress() ? -1 : count;
  if (context.network.scanInProgress()) {
    ui::drawIcon(ui::Icon::Wifi, 270, 230, 64);
    M5.Display.setFont(&fonts::FreeSansBold12pt7b); M5.Display.setTextDatum(MC_DATUM);
    M5.Display.drawString("SCANNING FOR WI-FI NETWORKS", 270, 326);
    M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.drawString("This can take a few seconds.", 270, 366);
    M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
    return;
  }
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString(count ? "Choose a network, then enter its password." : "No networks found. Tap the screen to scan again.", 270, 94);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
  const int visible = count > 6 ? 6 : count;
  for (int i = 0; i < visible; ++i) {
    String ssid = context.network.scanSsid(i); if (ssid.isEmpty()) ssid = "Hidden network";
    String detail = String(context.network.scanRssi(i)) + " dBm";
    settingsDetailRow(112 + i * 88, ui::Icon::Wifi, ssid, detail);
    ui::drawIcon(ui::Icon::ArrowRight, 484, 140 + i * 88, ui::Theme::NextIconSize);
  }
}

void SettingsApp::drawWifiPassword(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE); ui::Chrome::drawHeader(context, "Wi-Fi Password"); settingsFooter(true);
  settingsDetailRow(92, ui::Icon::Wifi, "Network", selectedWifi_);
  // This local setup screen deliberately shows what the user types. It is
  // clearer on e-paper than a fragile eye-toggle and avoids typo retries.
  M5.Display.fillRoundRect(28, 194, 484, 64, 8, TFT_BLACK); M5.Display.fillRoundRect(31, 197, 478, 58, 5, TFT_WHITE);
  M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.setTextDatum(ML_DATUM);
  String shownPassword = wifiPassword_;
  while (!shownPassword.isEmpty() && M5.Display.textWidth(shownPassword) > 430) shownPassword.remove(0, 1);
  if (shownPassword.length() != wifiPassword_.length()) shownPassword = "..." + shownPassword;
  M5.Display.drawString(shownPassword.isEmpty() ? "Enter password" : shownPassword, 48, 226);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString("Stored locally in this device's settings.", 270, 278);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
  const char* upper[] = {"QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"};
  const char* lower[] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
  const char* symbols[] = {"1234567890", "-_.@#%&*", "()[]{}!?/"};
  const char* const* rows = wifiKeyboardSymbols_ ? symbols : (wifiKeyboardLowercase_ ? lower : upper);
  for (int row = 0; row < 3; ++row) {
    const int length = strlen(rows[row]);
    for (int col = 0; col < length; ++col) {
      const int x = 20 + col * 50, y = 320 + row * 54;
      ui::Theme::drawButtonFrame(x, y, 44, 46);
      M5.Display.setFont(&fonts::FreeSansBold9pt7b); M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString(String(rows[row][col]), x + 22, y + 23);
    }
  }
  ui::Theme::drawButtonFrame(20, 490, 90, 52);
  ui::Theme::drawButtonFrame(118, 490, 90, 52);
  ui::Theme::drawButtonFrame(216, 490, 190, 52);
  ui::Theme::drawButtonFrame(414, 490, 106, 52);
  M5.Display.setFont(&fonts::FreeSansBold9pt7b); M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString(wifiKeyboardLowercase_ ? "ABC" : "abc", 65, 516);
  M5.Display.drawString(wifiKeyboardSymbols_ ? "ABC" : "123", 163, 516);
  M5.Display.drawString("SPACE", 311, 516); M5.Display.drawString("BACK", 467, 516);
  ui::Theme::drawButtonFrame(120, 580, 300, 64);
  M5.Display.setFont(&fonts::FreeSansBold12pt7b); M5.Display.drawString("CONNECT", 270, 612);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}
void SettingsApp::drawAddressSettings(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE); ui::Chrome::drawHeader(context,"Home Address"); settingsFooter(true);
  settingsDetailRow(104,ui::Icon::Home,"Address",context.network.configuredAddress());
  settingsDetailRow(194,ui::Icon::Home,"City",context.settings.location().city);
  settingsDetailRow(284,ui::Icon::Home,"Country",context.settings.location().country);
  settingsDetailRow(374,ui::Icon::Clock,"Time zone",context.time.timezone());
  M5.Display.fillRoundRect(105,510,330,66,8,TFT_BLACK);M5.Display.fillRoundRect(108,513,324,60,5,TFT_WHITE);M5.Display.setFont(&fonts::FreeSans12pt7b);M5.Display.setTextDatum(MC_DATUM);M5.Display.drawString("EDIT IN SETUP PORTAL",270,543);M5.Display.setFont(&fonts::FreeSans9pt7b);M5.Display.drawString("Changes update Wi-Fi and weather location details.",270,620);M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);
}
void SettingsApp::drawRecoverySettings(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE); ui::Chrome::drawHeader(context,"System & Recovery"); settingsFooter(true);
  settingsDetailRow(130,ui::Icon::System,"System monitor","Memory, battery, Wi-Fi and SD status");
  settingsDetailRow(240,ui::Icon::Refresh,"Restart device","Safely restart Paper OS");
  settingsDetailRow(350,ui::Icon::Power,"Factory reset","Clears OS settings; keeps SD files");
  M5.Display.setFont(&fonts::FreeSans9pt7b);M5.Display.setTextDatum(MC_DATUM);M5.Display.drawString("Use Factory Reset only if setup cannot be recovered.",270,500);M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);
}
void SettingsApp::drawFactoryResetConfirm() {
  M5.Display.fillScreen(TFT_WHITE);
  ui::Popup::drawFrame(30,280,480,360,"FACTORY RESET?");
  M5.Display.setTextDatum(MC_DATUM);
  M5.Display.setFont(&fonts::FreeSans12pt7b);M5.Display.drawString("This clears Paper OS preferences",270,400);M5.Display.drawString("and saved Wi-Fi credentials.",270,434);
  M5.Display.setFont(&fonts::FreeSans9pt7b);M5.Display.drawString("SD card files will NOT be erased.",270,474);
  M5.Display.fillRoundRect(74,530,170,62,8,TFT_BLACK);M5.Display.fillRoundRect(77,533,164,56,5,TFT_WHITE);M5.Display.fillRoundRect(296,530,170,62,8,TFT_BLACK);M5.Display.fillRoundRect(299,533,164,56,5,TFT_WHITE);
  M5.Display.setFont(&fonts::FreeSans12pt7b);M5.Display.drawString("CANCEL",159,561);M5.Display.drawString("RESET",381,561);M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);
}
void SettingsApp::drawPowerSettings(AppContext& context) {
  M5.Display.clear(); ui::Chrome::drawHeader(context, "Power & sleep"); settingsFooter(true);
  for (int i=0;i<3;++i) drawPowerSettingsRow(context, i);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString("Sleep shows time and battery; touch wakes to the launcher.",270,500); M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}
void SettingsApp::drawPowerSettingsRow(AppContext& context, int row) {
  const char* names[] = {"Inactivity sleep", "Sleep now", "Power off device"};
  const String values[] = {
    context.power.inactivityMinutes() ? String(context.power.inactivityMinutes()) + " minutes - tap to change" : "Disabled - tap to change",
    "Light sleep with touch wake",
    powerOffArmed_ ? "Tap again to confirm power off" : "Tap to prepare confirmation"
  };
  if (row < 0 || row > 2) return;
  const ui::Icon icons[]={ui::Icon::Sleep,ui::Icon::Sleep,ui::Icon::Power};
  settingsDetailRow(112+row*112,icons[row],names[row],values[row]);
}
void SettingsApp::drawClockSettings(AppContext& context) {
  M5.Display.clear(); ui::Chrome::drawHeader(context, "Clock & time"); settingsFooter(true);
  for (int i=0;i<6;++i) drawClockSettingsRow(context, i);
  drawClockStatus();
}
void SettingsApp::drawClockSettingsRow(AppContext& context, int row) {
  const char* names[] = {"Set date & time", "Time format", "Time zone", "Sync from internet", "Temperature unit", "Clock face"};
  const String values[] = {context.time.hasHardwareRtc() ? "Manual fallback saved to RTC" : "Manual fallback when NTP is unavailable", context.time.use24Hour() ? "24-hour" : "12-hour with AM/PM", context.time.timezone(), context.network.connected() ? "Sync now" : "Wi-Fi required", context.temperature.useFahrenheit() ? "Fahrenheit (F)" : "Celsius (C)", context.clockFace.name()};
  if (row < 0 || row > 5) return;
  const ui::Icon icons[]={ui::Icon::Clock,ui::Icon::Clock,ui::Icon::Settings,ui::Icon::Wifi,ui::Icon::Weather,ui::Icon::Clock};
  settingsDetailRow(82+row*90,icons[row],names[row],values[row]);
}
void SettingsApp::drawClockStatus() {
  M5.Display.fillRect(36,638,468,44,TFT_WHITE);
  if (!status_.isEmpty()) {
    M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(MC_DATUM);
    M5.Display.drawString(status_, 270, 660);
  }
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}
void SettingsApp::drawManualTimeSettings(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE); ui::Chrome::drawHeader(context, "Set date & time"); settingsFooter(true);
  drawManualTimeHeader();
  for(int i=0;i<5;++i) drawManualTimeRow(i);
  M5.Display.fillRoundRect(135,610,270,66,8,TFT_BLACK);M5.Display.fillRoundRect(138,613,264,60,5,TFT_WHITE);M5.Display.setFont(&fonts::FreeSans12pt7b);M5.Display.setTextDatum(MC_DATUM);M5.Display.drawString("SAVE DATE & TIME",270,643);
  M5.Display.setFont(&fonts::FreeSans9pt7b);M5.Display.drawString(context.time.hasHardwareRtc() ? "Saved time survives reboot through the PaperS3 RTC." : "Use this when NTP is unavailable.",270,710);M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);
}
void SettingsApp::drawManualTimeHeader() const {
  M5.Display.fillRect(24,84,492,66,TFT_WHITE);
  char current[32]; strftime(current,sizeof(current),"%d %b %Y  %H:%M",&manualTime_);
  M5.Display.setFont(&fonts::FreeSans18pt7b); M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString(current,270,116);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}
void SettingsApp::drawManualTimeRow(int row) const {
  const char* labels[]={"DAY","MONTH","YEAR","HOUR","MINUTE"};
  const int values[]={manualTime_.tm_mday,manualTime_.tm_mon+1,manualTime_.tm_year+1900,manualTime_.tm_hour,manualTime_.tm_min};
  if (row < 0 || row > 4) return;
  const int y=170+row*82;M5.Display.fillRect(36,y,468,66,TFT_WHITE);M5.Display.setFont(&fonts::FreeSansBold12pt7b);M5.Display.setTextDatum(MC_DATUM);M5.Display.drawString("-",92,y+30);M5.Display.drawString(labels[row],210,y+30);M5.Display.drawString(String(values[row]),320,y+30);M5.Display.drawString("+",448,y+30);M5.Display.fillRect(48,y+62,444,3,TFT_BLACK);M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);
}
void SettingsApp::drawWeatherSettings(AppContext& context) {
  M5.Display.clear(); ui::Chrome::drawHeader(context, "Weather updates"); settingsFooter(true);
  for(int i=0;i<2;++i)drawWeatherSettingsRow(context,i);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString("Tap a row to change the saved setting.",270,390); M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}
void SettingsApp::drawWeatherSettingsRow(AppContext& context, int row) {
  const char* names[]={"Offline retry","Weather face"}; const String values[]={String(context.weatherSettings.offlineRetryMinutes()) + " minutes - tap to change",String(context.weatherFace.name()) + " - tap to change"};
  if(row<0||row>1)return;const ui::Icon icons[]={ui::Icon::Refresh,ui::Icon::Weather};settingsDetailRow(130+row*110,icons[row],names[row],values[row]);
}
void SettingsApp::drawSoundSettings(AppContext& context) {
  M5.Display.clear(); ui::Chrome::drawHeader(context, "Button sounds"); settingsFooter(true);
  M5.Display.setTextDatum(MC_DATUM); M5.Display.setTextSize(2); M5.Display.drawString("BUTTON CLICK SOUND", 270, 174);
  const bool enabled = context.uiSound.enabled();
  M5.Display.fillRoundRect(105, 245, 330, 118, 14, TFT_BLACK);
  M5.Display.fillRoundRect(109, 249, 322, 110, 10, TFT_WHITE);
  M5.Display.setTextSize(4); M5.Display.drawString(enabled ? "ON" : "OFF", 270, 290);
  M5.Display.setTextSize(1); M5.Display.drawString("Tap to toggle. This applies to all app buttons.", 270, 410);
  M5.Display.setTextDatum(TL_DATUM);
}
void SettingsApp::drawFontSettings(AppContext& context) {
  M5.Display.clear(); ui::Chrome::drawHeader(context, "Display & Font"); settingsFooter(true);
  settingsDetailRow(130, ui::Icon::Note, "System text size", String(static_cast<int>(ui::Typography::scale() * 100)) + "% - tap to change");
  settingsDetailRow(220, ui::Icon::Refresh, "Flip display", context.settings.displayFlipped() ? "180 degrees" : "Normal orientation");
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString("Text size and display rotation apply across PaperOS.", 270, 350);
  M5.Display.drawString("Flip is retained after restart and works in landscape apps.", 270, 386);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}
void SettingsApp::drawFlashcardAiSettings(AppContext& context) {
  M5.Display.clear(); ui::Chrome::drawHeader(context, "Flashcard AI"); settingsFooter(true);
  settingsDetailRow(118, ui::Icon::Flashcards, "Gemini API key", context.settings.geminiApiKey().isEmpty() ? "Not configured" : "Saved on this device");
  settingsDetailRow(208, ui::Icon::Note, "Text model", context.settings.geminiTextModel());
  settingsDetailRow(298, ui::Icon::Image, "Image model", context.settings.geminiImageModel());
  M5.Display.setTextDatum(MC_DATUM); M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.drawString("Tap below to enable local AP setup, then open",270,470); M5.Display.drawString("http://192.168.4.1 and save Flashcard AI settings.",270,498);
  M5.Display.fillRoundRect(105,540,330,62,8,TFT_BLACK);M5.Display.fillRoundRect(108,543,324,56,5,TFT_WHITE);M5.Display.setFont(&fonts::FreeSans12pt7b);M5.Display.drawString("ENABLE AP SETUP",270,571);M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);
}
void SettingsApp::onTick(AppContext& context,uint32_t) {
  const auto& t=M5.Touch.getDetail();
  // A scan completes asynchronously. Redraw only this child page once its
  // result count changes; no network operation ever blocks touch input.
  if (wifiScanOpen_ && !t.wasPressed()) {
    const int scanState = context.network.scanInProgress() ? -1 : context.network.scanResultCount();
    if (scanState != lastWifiScanCount_) drawWifiScan(context);
    return;
  }
  if(!t.wasPressed())return;
  if (factoryResetConfirm_) {
    if (ui::Popup::hitClose(t.x,t.y,30,280,480)) { factoryResetConfirm_=false;drawRecoverySettings(context);return; }
    if (t.y >= 530 && t.y <= 592) {
      if (t.x >= 74 && t.x <= 244) { factoryResetConfirm_ = false; drawRecoverySettings(context); return; }
      if (t.x >= 296 && t.x <= 466) {
        context.settings.reset();
        WiFi.disconnect(true, true); // removes SDK-managed credentials too
        M5.Display.fillScreen(TFT_WHITE); M5.Display.setTextDatum(MC_DATUM); M5.Display.setFont(&fonts::FreeSans18pt7b); M5.Display.drawString("RESETTING DEVICE",270,470); M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr); M5.Display.waitDisplay(); delay(250); ESP.restart();
      }
    }
    return;
  }
  const bool detailPage = wifiSettingsOpen_ || wifiScanOpen_ || wifiPasswordOpen_ || addressSettingsOpen_ || recoverySettingsOpen_ || clockSettingsOpen_ || manualTimeOpen_ || powerSettingsOpen_ || weatherSettingsOpen_ || soundSettingsOpen_ || fontSettingsOpen_ || flashcardAiOpen_;
  ui::ChromeOptions footer; footer.showBack = detailPage; footer.showHome = !detailPage;
  if (ui::Chrome::hitTestFooter(t.x, t.y, footer) != ui::FooterAction::None) {
    if (manualTimeOpen_) { manualTimeOpen_ = false; drawClockSettings(context); return; }
    if (wifiPasswordOpen_) { wifiPasswordOpen_ = false; wifiScanOpen_ = true; drawWifiScan(context); return; }
    if (wifiScanOpen_) { wifiScanOpen_ = false; wifiSettingsOpen_ = true; drawWifiSettings(context); return; }
    if (wifiSettingsOpen_ || addressSettingsOpen_) { wifiSettingsOpen_ = addressSettingsOpen_ = false; draw(context); return; }
    if (recoverySettingsOpen_) { recoverySettingsOpen_ = false; draw(context); return; }
    if (detailPage) {
      clockSettingsOpen_ = powerSettingsOpen_ = weatherSettingsOpen_ = soundSettingsOpen_ = fontSettingsOpen_ = flashcardAiOpen_ = powerOffArmed_ = false;
      status_ = ""; draw(context);
    } else if (navigator_) navigator_(AppId::Launcher);
    return;
  }
  if (wifiScanOpen_) {
    const int count = context.network.scanResultCount();
    if (count <= 0) { context.network.startScan(); drawWifiScan(context); return; }
    if (t.x >= 28 && t.x <= 512 && t.y >= 112 && t.y < 112 + (count > 6 ? 6 : count) * 88) {
      const int item = (t.y - 112) / 88;
      selectedWifi_ = context.network.scanSsid(item);
      if (selectedWifi_.isEmpty()) return; // Hidden SSIDs remain available through AP setup.
      wifiPassword_ = ""; wifiKeyboardSymbols_ = false; wifiKeyboardLowercase_ = false; wifiScanOpen_ = false; wifiPasswordOpen_ = true; drawWifiPassword(context);
    }
    return;
  }
  if (wifiPasswordOpen_) {
    if (t.y >= 320 && t.y < 482 && t.x >= 20 && t.x < 520) {
      const int row = (t.y - 320) / 54, col = (t.x - 20) / 50;
      const char* upper[] = {"QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"};
      const char* lower[] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
      const char* symbols[] = {"1234567890", "-_.@#%&*", "()[]{}!?/"};
      const char* rowText = (wifiKeyboardSymbols_ ? symbols : (wifiKeyboardLowercase_ ? lower : upper))[row];
      if (col >= 0 && col < static_cast<int>(strlen(rowText)) && wifiPassword_.length() < 63) wifiPassword_ += rowText[col];
      drawWifiPassword(context); return;
    }
    if (t.y >= 490 && t.y <= 542) {
      if (t.x < 110) wifiKeyboardLowercase_ = !wifiKeyboardLowercase_;
      else if (t.x < 208) wifiKeyboardSymbols_ = !wifiKeyboardSymbols_;
      else if (t.x < 406 && wifiPassword_.length() < 63) wifiPassword_ += ' ';
      else if (t.x >= 414 && wifiPassword_.length()) wifiPassword_.remove(wifiPassword_.length() - 1);
      drawWifiPassword(context); return;
    }
    if (t.x >= 120 && t.x <= 420 && t.y >= 580 && t.y <= 644) {
      context.network.connect(selectedWifi_, wifiPassword_);
      status_ = "Connecting to " + selectedWifi_;
      wifiPasswordOpen_ = false; wifiSettingsOpen_ = true; drawWifiSettings(context); return;
    }
    return;
  }
  if (wifiSettingsOpen_) {
    if (t.x >= 28 && t.x <= 512 && t.y >= 374 && t.y <= 456) {
      context.network.startScan(); lastWifiScanCount_ = -2; wifiSettingsOpen_ = false; wifiScanOpen_ = true; drawWifiScan(context); return;
    }
    if (t.x >= 28 && t.x <= 512 && t.y >= 464 && t.y <= 546) {
      if (context.network.provisioning()) context.network.stopProvisioning();
      else context.network.startProvisioning();
      // The row itself is static. Update only the changed toggle plus the
      // conditional help strip that must appear/disappear with its state.
      drawWifiApToggle(context);
      M5.Display.display(344,472,144,41);
      drawWifiApHelp(context.network.provisioning());
      M5.Display.display(28,546,484,ui::Chrome::footerTop()-546);
    }
    return;
  }
  if (addressSettingsOpen_) {
    if (t.x >= 105 && t.x <= 435 && t.y >= 510 && t.y <= 576) {
      context.network.startProvisioning();
      M5.Display.fillRect(28,374,484,82);
      settingsDetailRow(374,ui::Icon::Clock,"Time zone",context.time.timezone());
      M5.Display.display(28,374,484,82);
    }
    return;
  }
  if (recoverySettingsOpen_) {
    if (t.y >= 130 && t.y < 432 && t.x >= 28 && t.x <= 512) {
      const int row=(t.y-130)/110;
      if(row==0&&navigator_){navigator_(AppId::SystemMonitor);return;}
      if(row==1){M5.Display.fillScreen(TFT_WHITE);M5.Display.setTextDatum(MC_DATUM);M5.Display.setFont(&fonts::FreeSans18pt7b);M5.Display.drawString("RESTARTING",270,470);M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);M5.Display.waitDisplay();delay(250);ESP.restart();}
      if(row==2){factoryResetConfirm_=true;drawFactoryResetConfirm();return;}
    }
    return;
  }
  if (manualTimeOpen_) {
    if (t.y >= 170 && t.y < 580 && (t.x >= 36 && t.x <= 504)) {
      const int row = (t.y - 170) / 82;
      if (row < 0 || row > 4) return;
      const int direction = t.x < 270 ? -1 : 1;
      if (row == 0) manualTime_.tm_mday += direction;
      else if (row == 1) manualTime_.tm_mon += direction;
      else if (row == 2) manualTime_.tm_year += direction;
      else if (row == 3) manualTime_.tm_hour += direction;
      else manualTime_.tm_min += direction * 5;
      // mktime/localtime normalise month ends, leap years and DST safely.
      manualTime_.tm_isdst = -1; const time_t normalized = mktime(&manualTime_); localtime_r(&normalized, &manualTime_);
      drawManualTimeHeader();
      drawManualTimeRow(row);
      // Two small, non-overlapping partial updates: current date/time and
      // the edited field. Static controls and chrome are never redrawn.
      M5.Display.display(24, 84, 492, 66);
      M5.Display.display(36, 170 + row * 82, 468, 66);
      return;
    }
    if (t.x >= 135 && t.x <= 405 && t.y >= 610 && t.y <= 676) {
      status_ = context.time.setManualLocalTime(manualTime_) ? (context.time.hasHardwareRtc() ? "Date/time saved to the PaperS3 RTC." : "Date and time saved.") : "Could not save date and time.";
      manualTimeOpen_ = false; drawClockSettings(context); return;
    }
    return;
  }
  if (clockSettingsOpen_) {
    if (t.x < 22 || t.x > 518) return;
    const int row=(t.y-82)/90; if (row < 0 || row > 5) return;
    if (row == 0) {
      if (!context.time.localTime(manualTime_)) {
        manualTime_ = {}; manualTime_.tm_year = 126; manualTime_.tm_mon = 0; manualTime_.tm_mday = 1;
      }
      manualTimeOpen_ = true; drawManualTimeSettings(context); return;
    } else if (row == 1) {
      context.time.setUse24Hour(!context.time.use24Hour()); status_ = "Time format saved.";
    } else if (row == 2) {
      static const char* const zones[] = {"IST-5:30", "UTC0", "GMT0BST,M3.5.0/1,M10.5.0", "EST5EDT,M3.2.0,M11.1.0", "PST8PDT,M3.2.0,M11.1.0"};
      int selected=0; for(int i=0;i<5;++i)if(String(context.time.timezone())==zones[i])selected=i;
      context.time.setTimezone(zones[(selected+1)%5]); status_="Saved timezone. Time will update from NTP when online.";
    } else if (row == 3) status_=context.time.requestInternetSync()?"NTP sync started; the clock stays usable while it completes.":"Cannot sync: connect Wi-Fi first, then try again.";
    else if (row == 4) { context.temperature.setUseFahrenheit(!context.temperature.useFahrenheit()); status_="Temperature unit saved for Clock and Weather."; }
    else { context.clockFace.next(); status_="Clock face saved."; }
    drawClockSettingsRow(context, row);
    drawClockStatus();
    M5.Display.display(28, 82 + row * 90, 484, 82);
    M5.Display.display(36, 638, 468, 44);
    return;
  }
  if (powerSettingsOpen_) {
    if (t.x < 22 || t.x > 518) return;
    const int row=(t.y-112)/112; if (row < 0 || row > 2) return;
    if (row == 0) {
      static const uint8_t choices[] = {0, 1, 5, 10, 15, 30};
      int current=0; for(int i=0;i<6;++i)if(context.power.inactivityMinutes()==choices[i])current=i;
      context.power.setInactivityMinutes(choices[(current+1)%6]);
    } else if (row == 1) { context.power.sleepNow(context.time.formattedLocalTime()); return; }
    else if (!powerOffArmed_) { powerOffArmed_=true; }
    else { context.power.powerOff(); }
    drawPowerSettingsRow(context, row);
    M5.Display.display(28, 112 + row * 112, 484, 82);
    return;
  }
  if (weatherSettingsOpen_) {
    if (t.x < 22 || t.x > 518 || t.y < 130 || t.y > 350) return;
    const int row=(t.y-130)/110;
    if(row==1){context.weatherFace.next();drawWeatherSettingsRow(context, row);M5.Display.display(28,130+row*110,484,82);return;}
    static const uint8_t choices[] = {1, 5, 10, 15, 30};
    int current = 0; for (int i=0; i<5; ++i) if (context.weatherSettings.offlineRetryMinutes() == choices[i]) current = i;
    context.weatherSettings.setOfflineRetryMinutes(choices[(current + 1) % 5]);
    drawWeatherSettingsRow(context, row);
    M5.Display.display(28,130+row*110,484,82);
    return;
  }
  if (soundSettingsOpen_) {
    if (t.x < 105 || t.x > 435 || t.y < 245 || t.y > 363) return;
    const bool wasEnabled = context.uiSound.enabled(); context.uiSound.setEnabled(!wasEnabled);
    if (!wasEnabled) context.uiSound.click();
    drawSoundSettings(context); return;
  }
  if (fontSettingsOpen_) {
    if (t.x < 28 || t.x > 512 || t.y < 130 || t.y > 302) return;
    if (t.y >= 220) {
      const bool flipped = !context.settings.displayFlipped();
      context.settings.setDisplayFlipped(flipped);
      context.display.setFlipped(flipped);
      drawFontSettings(context); return;
    }
    static const float scales[] = {0.90f, 1.00f, 1.10f, 1.20f, 1.30f};
    int current = 0; for (int index = 0; index < 5; ++index) if (fabsf(ui::Typography::scale() - scales[index]) < 0.02f) current = index;
    ui::Typography::setScale(scales[(current + 1) % 5]);
    drawFontSettings(context); return;
  }
  if (flashcardAiOpen_) {
    if(t.x>=105&&t.x<=435&&t.y>=540&&t.y<=602){context.network.startProvisioning(); drawFlashcardAiSettings(context);}
    return;
  }
  const int row=(t.y-kRootFirstRow)/kRootRowHeight;
  if(t.x<32||t.x>508||t.y<kRootFirstRow||row<0||row>8)return;
  if(row==0){wifiSettingsOpen_=true;drawWifiSettings(context);return;}
  if(row==1){addressSettingsOpen_=true;drawAddressSettings(context);return;}
  if(row==2){clockSettingsOpen_=true;status_="";drawClockSettings(context);return;}
  if(row==3){weatherSettingsOpen_=true;drawWeatherSettings(context);return;}
  if(row==4){powerSettingsOpen_=true;powerOffArmed_=false;drawPowerSettings(context);return;}
  if (row == 5) {
    // Sound is a direct preference, not a nested page. Refresh only its row
    // and leave the rest of Settings (including shared chrome) untouched.
    const bool wasEnabled = context.uiSound.enabled();
    context.uiSound.setEnabled(!wasEnabled);
    if (!wasEnabled) context.uiSound.click();
    const int y = kRootFirstRow + 5 * kRootRowHeight;
    M5.Display.fillRect(32, y, 476, kRootRowHeight, TFT_WHITE);
    settingsListRow(5, ui::Icon::Timer, "Button Sounds", context.uiSound.enabled() ? "ON" : "OFF", true);
    M5.Display.display(32, y, 476, kRootRowHeight);
    return;
  }
  if(row==6){recoverySettingsOpen_=true;drawRecoverySettings(context);return;}
  if(row==7){fontSettingsOpen_=true;drawFontSettings(context);return;}
  if(row==8){flashcardAiOpen_=true;drawFlashcardAiSettings(context);return;}
  draw(context);
}
