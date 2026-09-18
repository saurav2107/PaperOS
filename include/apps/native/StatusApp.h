#pragma once
#include "app/IApp.h"

using NavigationCallback = void (*)(AppId);

class StatusApp : public IApp {
 public:
  StatusApp(AppId id, const char* title, const char* description)
      : id_(id), title_(title), description_(description) {}
  AppId id() const override { return id_; }
  const char* title() const override { return title_; }
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  bool onStart(AppContext&) override;
  void onTick(AppContext&, uint32_t) override;
  void onStop(AppContext&) override {}
 protected:
  AppId id_;
  const char* title_;
  const char* description_;
  NavigationCallback navigator_{nullptr};
};
