#pragma once
#include "app/IApp.h"
#include "apps/native/StatusApp.h"

class SettingsApp final : public IApp {
 public:
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  AppId id() const override { return AppId::Settings; }
  const char* title() const override { return "Settings"; }
  bool onStart(AppContext&) override;
  void onTick(AppContext&, uint32_t) override;
  void onStop(AppContext&) override {}
 private:
  void draw(AppContext&);
  void drawWifiSettings(AppContext&);
  void drawWifiApSection(AppContext&);
  void drawWifiApToggle(AppContext&);
  void drawWifiApHelp(bool enabled);
  void drawWifiScan(AppContext&);
  void drawWifiPassword(AppContext&);
  void drawAddressSettings(AppContext&);
  void drawRecoverySettings(AppContext&);
  void drawFactoryResetConfirm();
  void drawClockSettings(AppContext&);
  void drawClockSettingsRow(AppContext&, int row);
  void drawClockStatus();
  void drawManualTimeSettings(AppContext&);
  void drawManualTimeHeader() const;
  void drawManualTimeRow(int row) const;
  void drawPowerSettings(AppContext&);
  void drawPowerSettingsRow(AppContext&, int row);
  void drawWeatherSettings(AppContext&);
  void drawWeatherSettingsRow(AppContext&, int row);
  void drawSoundSettings(AppContext&);
  void drawFontSettings(AppContext&);
  void drawFlashcardAiSettings(AppContext&);
  NavigationCallback navigator_{nullptr};
  uint8_t fontScale_{1}; bool lowGhosting_{true};
  bool wifiSettingsOpen_{false}; bool wifiScanOpen_{false}; bool wifiPasswordOpen_{false}; bool wifiKeyboardSymbols_{false}; bool wifiKeyboardLowercase_{false}; bool addressSettingsOpen_{false}; bool recoverySettingsOpen_{false}; bool factoryResetConfirm_{false}; bool clockSettingsOpen_{false}; bool manualTimeOpen_{false}; bool powerSettingsOpen_{false}; bool weatherSettingsOpen_{false}; bool soundSettingsOpen_{false}; bool fontSettingsOpen_{false}; bool flashcardAiOpen_{false}; bool powerOffArmed_{false};
  int lastWifiScanCount_{-2};
  String selectedWifi_, wifiPassword_;
  tm manualTime_{};
  String status_;
};
