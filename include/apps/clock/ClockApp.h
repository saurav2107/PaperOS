#pragma once

#include "app/IApp.h"
#include "apps/native/StatusApp.h"

// E-paper friendly flip clock: its state changes once per minute, so it avoids
// the unnecessary continuous animation used by LCD flip-clock implementations.
class ClockApp final : public IApp {
 public:
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  AppId id() const override { return AppId::Clock; }
  const char* title() const override { return "Clock"; }
  // AppManager calls this when Clock is opened. It switches the display to
  // landscape before drawing the persisted ClockFaceService selection.
  bool onStart(AppContext& context) override;
  // nowMs is Arduino millis() forwarded by AppManager. Touch data is read from
  // M5.Touch here; a content tap cycles the persisted clock-face preference.
  void onTick(AppContext& context, uint32_t nowMs) override;
  // Restores portrait orientation before AppManager activates the next app.
  void onStop(AppContext& context) override;

 private:
  // force requests a complete face redraw. The tm value is obtained from
  // context.time.localTime() inside draw, rather than being owned by ClockApp.
  void draw(AppContext&, bool force = false);
  // Face-specific renderers. local is the already-resolved local time from
  // TimeService; context provides temperature and the shared Chrome services.
  void drawDigital(AppContext&, const tm& local);
  void drawAnalog(AppContext&, const tm& local);
  void drawDateTemperature(AppContext&, const tm& local);
  void drawDotted(AppContext&, const tm& local);
  void drawPanel(AppContext&, const tm& local);
  void drawDarkFlip(AppContext&, const tm& local, bool force);
  String darkTileValues_[7];
  int darkHour_{-1};
  NavigationCallback navigator_{nullptr};
  int lastMinute_{-1};
  int lastWidth_ = 0; // Tracks rotation changes
  bool fullScreen_{false};
  bool pendingFaceChange_{false};
  uint32_t lastTapMs_{0};

  // Flip-face primitives. Coordinates are calculated by draw from Chrome's
  // runtime header/footer metrics, so they work after a rotation change.
  void drawColon(bool show, int centerX, int yTop, int cardHeight);
  void drawDigit(int x, int y, int w, int h, char digit, bool darkTop) const;
};
