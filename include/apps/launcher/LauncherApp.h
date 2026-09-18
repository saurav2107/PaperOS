#pragma once
#include "app/IApp.h"
#include "apps/native/StatusApp.h"

class LauncherApp final : public IApp {
 public:
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  AppId id() const override { return AppId::Launcher; }
  const char* title() const override { return "Launcher"; }
  bool onStart(AppContext&) override;
  void onTick(AppContext&, uint32_t) override;
  void onStop(AppContext&) override {}
 private:
  void draw(AppContext&);
  void drawGamesPopup(AppContext&);
  void drawUtilitiesPopup(AppContext&);
  void drawPowerOffPopup();
  void drawGameIcon(int x, int y, uint8_t game) const;
  void showComingSoon(AppContext&, const char* game);
  NavigationCallback navigator_{nullptr};
  bool gamesOpen_{false};
  bool utilitiesOpen_{false};
  bool powerOffConfirmation_{false};
};
