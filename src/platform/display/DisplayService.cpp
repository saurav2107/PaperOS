#include "services/Services.h"
#include <M5Unified.h>

void DisplayService::begin() { setPortrait(); clear(); }
void DisplayService::setFlipped(bool flipped) { flipped_ = flipped; setPortrait(); }
void DisplayService::setPortrait() { M5.Display.setRotation(flipped_ ? 2 : 0); }
void DisplayService::setLandscape() { M5.Display.setRotation(flipped_ ? 3 : 1); }
void DisplayService::clear() { M5.Display.clear(); }
void DisplayService::header(const char* title) { M5.Display.setCursor(20,20); M5.Display.setTextSize(2); M5.Display.printf("Paper OS - %s\n",title); M5.Display.drawFastHLine(20,55,500,TFT_BLACK); }
void DisplayService::message(const char* title,const char* detail) { clear(); header(title); M5.Display.setCursor(20,90); M5.Display.setTextSize(1); M5.Display.println(detail); }
void DisplayService::resetToLauncherState() { setPortrait(); clear(); }
