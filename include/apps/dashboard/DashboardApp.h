#pragma once

#include "app/IApp.h"
#include "apps/native/StatusApp.h"

// The dashboard owns a dirty flag. MQTT/Home Assistant integration can call
// markDirty() after a meaningful state update; no timer-driven display redraws.
class DashboardApp final : public IApp {
 public:
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  AppId id() const override { return AppId::Dashboard; }
  const char* title() const override { return "Home Dashboard"; }
  bool onStart(AppContext&) override;
  void onTick(AppContext&, uint32_t) override;
  void onStop(AppContext&) override {}
  void markDirty() { dirty_ = true; }

 private:
  void draw(AppContext&);
  NavigationCallback navigator_{nullptr};
  bool dirty_{true};
  bool lastOnline_{false};
  uint8_t lastBattery_{255};
  uint32_t nextStateCheckMs_{0};
};
