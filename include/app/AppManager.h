#pragma once
#include "app/IApp.h"

class AppManager {
 public:
  explicit AppManager(AppContext& context);
  bool registerApp(IApp& app);
  bool activate(AppId id);
  void tick();
  AppId activeId() const;

 private:
  IApp* find(AppId id) const;
  AppContext& context_;
  // Registry capacity is independent of the current 3x4 launcher grid.
  IApp* apps_[32]{};
  uint8_t appCount_{0};
  IApp* active_{nullptr};
};
