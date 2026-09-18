#include "apps/dashboard/DashboardApp.h"

#include <M5Unified.h>
#include "app/AppContext.h"
#include "services/Services.h"
#include "ui/Icons.h"
#include "ui/Chrome.h"

namespace {
void line3(int x0, int y0, int x1, int y1) {
  M5.Display.drawLine(x0, y0, x1, y1, TFT_BLACK);
  M5.Display.drawLine(x0 + 1, y0, x1 + 1, y1, TFT_BLACK);
  M5.Display.drawLine(x0, y0 + 1, x1, y1 + 1, TFT_BLACK);
}
void card(int x, int y, int w, int h) {
  M5.Display.drawRoundRect(x, y, w, h, 12, TFT_BLACK);
}
void label(const char* text, int x, int y) {
  M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.setTextDatum(MC_DATUM); M5.Display.setTextSize(1); M5.Display.drawString(text, x, y); M5.Display.setFont(nullptr);
}
}

bool DashboardApp::onStart(AppContext& context) { dirty_ = true; draw(context); return true; }

void DashboardApp::draw(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE); ui::Chrome::drawHeader(context, "Home Dashboard");
  M5.Display.setFont(&fonts::FreeSansBold18pt7b); M5.Display.setTextDatum(MC_DATUM); M5.Display.setTextSize(1); M5.Display.drawString("HOME", 270, 134);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.drawString("NOIDA, INDIA", 270, 180); M5.Display.setFont(nullptr);
  const int left=22, right=278, top=224, bottom=454, width=240, height=194;
  card(left,top,width,height); card(right,top,width,height); card(left,bottom,width,height); card(right,bottom,width,height);
  ui::drawIcon(ui::Icon::Home,left+54,top+55,52); ui::drawIcon(ui::Icon::Wifi,right+54,top+55,52);
  ui::drawIcon(ui::Icon::Battery,left+54,bottom+55,52); ui::drawIcon(ui::Icon::Sleep,right+54,bottom+55,52);
  label("HOME ASSISTANT", left+143, top+45); label(context.network.connected()?"READY":"OFFLINE", left+143, top+86);
  label("WI-FI", right+143, top+45); label(context.network.connected()?"CONNECTED":"OFFLINE", right+143, top+86);
  label("BATTERY", left+143, bottom+45); label((String(context.power.batteryPercent())+"%").c_str(), left+143, bottom+86);
  label("AUTO SLEEP", right+143, bottom+45);
  const String sleep = context.power.inactivityMinutes() ? String(context.power.inactivityMinutes()) + " MIN" : "OFF";
  label(sleep.c_str(), right+143, bottom+86);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextSize(1); M5.Display.drawString("Dashboard updates only when status changes", 270, 710); M5.Display.setFont(nullptr);
  M5.Display.setTextDatum(TL_DATUM);
  ui::Chrome::drawFooter();
  dirty_ = false; lastOnline_ = context.network.connected(); lastBattery_ = context.power.batteryPercent();
}

void DashboardApp::onTick(AppContext& context, uint32_t now) {
  const auto& touch = M5.Touch.getDetail();
  if (touch.wasPressed() && ui::Chrome::hitTestFooter(touch.x, touch.y) != ui::FooterAction::None) { if (navigator_) navigator_(AppId::Launcher); return; }
  // State is sampled sparsely; redraw is still conditional, not periodic.
  if (now >= nextStateCheckMs_) { nextStateCheckMs_ = now + 30000UL; if (lastOnline_ != context.network.connected() || lastBattery_ != context.power.batteryPercent()) dirty_ = true; }
  if (dirty_) draw(context);
}
