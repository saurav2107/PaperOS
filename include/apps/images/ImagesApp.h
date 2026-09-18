#pragma once

#include "app/IApp.h"
#include "apps/native/StatusApp.h"
#include "ui/Chrome.h"

// SD-backed image browser.  It retains only filenames, not decoded images, so
// opening Photos has a predictable RAM cost even with a large photo library.
class ImagesApp final : public IApp {
 public:
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  AppId id() const override { return AppId::Images; }
  const char* title() const override { return "Photos"; }
  bool onStart(AppContext& context) override;
  void onTick(AppContext& context, uint32_t nowMs) override;
  void onStop(AppContext&) override {}

 private:
  static constexpr uint8_t MaxImages = 48;
  void scanLibrary(AppContext& context);
  bool isSupported(const String& path) const;
  void draw(AppContext& context);
  void drawImage();
  void drawLoading() const;
  ui::ChromeOptions chromeOptions() const;

  String images_[MaxImages];
  uint8_t imageCount_{0};
  uint8_t current_{0};
  bool fullScreen_{false};
  uint32_t lastTapMs_{0};
  String status_;
  NavigationCallback navigator_{nullptr};
};
