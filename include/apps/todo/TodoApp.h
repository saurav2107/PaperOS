#pragma once
#include "app/IApp.h"
#include "apps/native/StatusApp.h"

// SD-backed task editor. Tasks are stored as `0|YYYY-MM-DD|Task text` or
// `1|YYYY-MM-DD|Task text` in /PaperOS/todo/tasks.txt; Calendar filters them
// by the selected day. Legacy `0|Task text` entries remain treated as today.
class TodoApp final : public IApp {
 public:
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  AppId id() const override { return AppId::Tasks; }
  const char* title() const override { return "To Do"; }
  bool onStart(AppContext& context) override;
  void onTick(AppContext& context, uint32_t nowMs) override;
  void onStop(AppContext&) override {}
 private:
  void load(AppContext& context);
  void save(AppContext& context);
  void draw(AppContext& context);
  void drawList(AppContext& context);
  void drawEditor(AppContext& context);
  void drawKeyboard();
  void drawActionRows();
  // Redraws only the entry field after a key press; the keyboard remains
  // unchanged, avoiding an unnecessary e-paper refresh of the whole editor.
  void drawDraftField();
  void handleTap(AppContext& context, int x, int y);
  NavigationCallback navigator_{nullptr};
  String tasks_[8]; String dueDates_[8]; bool complete_[8]{}; uint8_t count_{0};
  bool editing_{false}; bool shift_{false}; String draft_; String status_;
};
