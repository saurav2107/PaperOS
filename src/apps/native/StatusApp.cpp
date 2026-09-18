#include "apps/native/StatusApp.h"
#include <M5Unified.h>
#include "app/AppContext.h"
#include "services/Services.h"
#include "ui/Chrome.h"

bool StatusApp::onStart(AppContext& context) {
  M5.Display.clear();
  ui::Chrome::drawHeader(context, title_);
  ui::Chrome::drawFooter();
  M5.Display.setCursor(26, 108);
  M5.Display.setTextSize(2);
  M5.Display.println(description_);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(26, 170);
  M5.Display.printf("Wi-Fi: %s\nBattery: %u%%\nSD: %s",
      context.network.connected() ? "connected" : "offline",
      context.power.batteryPercent(), context.storage.mounted() ? "mounted" : "not mounted");
  return true;
}

void StatusApp::onTick(AppContext&, uint32_t) {
  const auto& touch = M5.Touch.getDetail();
  if (!touch.wasPressed() || !navigator_) return;
  const auto action = ui::Chrome::hitTestFooter(touch.x, touch.y);
  if (action == ui::FooterAction::Back || action == ui::FooterAction::Home) navigator_(AppId::Launcher);
}
