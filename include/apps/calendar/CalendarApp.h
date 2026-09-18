#pragma once
#include "app/IApp.h"
#include "apps/native/StatusApp.h"

// Calendar's local Today section reads the same SD file written by TodoApp.
class CalendarApp final : public IApp {
 public:
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  AppId id() const override { return AppId::Calendar; }
  const char* title() const override { return "Calendar"; }
  bool onStart(AppContext& context) override;
  void onTick(AppContext& context, uint32_t nowMs) override;
 void onStop(AppContext&) override {}
 private:
  void draw(AppContext& context);
  void changeMonth(int delta);
  NavigationCallback navigator_{nullptr};
  int year_{0};
  int month_{0}; // tm convention: January = 0
  int selectedDay_{0};
};
