#pragma once
#include "app/IApp.h"
#include "apps/native/StatusApp.h"

class PomodoroApp final : public IApp {
 public:
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  AppId id() const override { return AppId::Pomodoro; }
  const char* title() const override { return "Pomodoro"; }
  // Called by AppManager when Pomodoro becomes the active application.
  // context is the OS-owned service bundle; it is not constructed by the app.
  bool onStart(AppContext& context) override;
  // Called once per main-loop pass. nowMs is Arduino millis(), supplied by
  // AppManager, and is used to advance the timer without a FreeRTOS task.
  void onTick(AppContext& context, uint32_t nowMs) override;
  // Called by AppManager before navigation to another application.
  void onStop(AppContext& context) override;
 private:
  // Draws Chrome plus content when fullRefresh is true. Normal timer updates
  // deliberately call drawTimer instead, preserving the rest of the e-paper.
  void draw(AppContext&, bool fullRefresh = false);
  // Renders the 400px timer square. resetLayout is true for a style/duration
  // change; false for a one-second tick so only changed timer pixels are drawn.
  void drawTimer(AppContext&, bool resetLayout = false);
  // Draws the six static controls above the shared footer.
  void drawButtons();
  // Redraws only app-owned content: timer square and button strip, never Chrome.
  void drawContent(AppContext&);
  // Calculates elapsed/remaining time from local state and nowMs from onTick.
  void updateTimer(AppContext&, uint32_t);
  // x/y originate from M5.Touch.getDetail() in onTick; routes a tap to a
  // footer action, refresh, preset, transport control, or style change.
  void handleTap(AppContext&, int, int);
  // State transitions invoked only by handleTap after a validated control hit.
  void start(); void stop(); void pause(); void selectDuration(uint8_t); void refresh(AppContext&);
  NavigationCallback navigator_{nullptr};
  uint16_t durationMinutes_{25};
  uint16_t elapsedSeconds_{0};
  uint32_t lastSecondMs_{0};
  uint32_t lastActivityMs_{0};
  bool running_{false}; bool paused_{false}; bool locked_{false};
};
