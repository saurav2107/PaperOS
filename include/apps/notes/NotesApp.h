#pragma once
#include "app/IApp.h"
#include "apps/native/StatusApp.h"

class NotesApp final : public IApp {
 public:
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  AppId id() const override { return AppId::Notes; }
  const char* title() const override { return "Notes"; }
  bool onStart(AppContext&) override;
  void onTick(AppContext&, uint32_t) override;
  void onStop(AppContext&) override {}
 private:
  void draw(AppContext&); void drawTextField(); void drawKeyboard(); void drawActionRow(); void append(char); void save(AppContext&);
  NavigationCallback navigator_{nullptr};
  String text_; String status_; bool shift_{false};
};
