#include "apps/pomodoro/PomodoroApp.h"
#include <math.h>
#include <SD.h>
#include <M5Unified.h>
#include "app/AppContext.h"
#include "services/Services.h"
#include "ui/Chrome.h"
#include "ui/Icons.h"
#include "ui/Theme.h"

namespace {
ui::ChromeOptions pomodoroChrome() { ui::ChromeOptions options; options.showBack = false; options.showHome = true; return options; }
void boldButton(int x, int y, int w, int h, ui::Icon icon, const char* label) {
  ui::Theme::drawButtonFrame(x, y, w, h);
  ui::drawIcon(icon, x + 25, y + h / 2, 28);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(ML_DATUM);
  M5.Display.drawString(label, x + 48, y + h / 2 + 1);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}
}

bool PomodoroApp::onStart(AppContext& c){durationMinutes_=25;elapsedSeconds_=0;running_=paused_=locked_=false;lastSecondMs_=lastActivityMs_=millis();M5.Speaker.setVolume(200);draw(c,true);return true;}
void PomodoroApp::drawTimer(AppContext& c, bool resetLayout){
  constexpr int squareX=70,squareY=120,squareSize=400,cx=270,cy=320,outer=174,inner=144;
  // A full timer-layout redraw happens only on a style, duration, or control
  // change. Normal one-second ticks touch the rings/bar and central time only.
  if (resetLayout) {
    M5.Display.fillRect(squareX,squareY,squareSize,squareSize,TFT_WHITE);
    // Use the exact shared button frame: three-pixel outer border, matching
    // radius and inner inset. This prevents the timer card looking lighter
    // than the six controls below it.
    ui::Theme::drawButtonFrame(squareX, squareY, squareSize, squareSize);
  }
  const int mins=elapsedSeconds_/60,secs=elapsedSeconds_%60;
  const uint32_t remain=durationMinutes_*60UL-elapsedSeconds_; char txt[8]; snprintf(txt,sizeof(txt),"%02lu:%02lu",remain/60,remain%60);
  const float progress = durationMinutes_ ? static_cast<float>(elapsedSeconds_) / (durationMinutes_ * 60.0f) : 0.0f;
  if (c.pomodoroStyle.style() == PomodoroStyle::Ring) {
    for(int i=0;i<60;++i){float a=(i*6-90)*PI/180;M5.Display.fillCircle(cx+outer*cosf(a),cy+outer*sinf(a),4,i<secs?TFT_WHITE:TFT_BLACK);}
    for(int i=0;i<durationMinutes_;++i){float a=(i*(360.0f/durationMinutes_)-90)*PI/180;M5.Display.fillCircle(cx+inner*cosf(a),cy+inner*sinf(a),5,i<mins?TFT_WHITE:TFT_BLACK);}
  } else if (c.pomodoroStyle.style() == PomodoroStyle::Progress) {
    M5.Display.fillRect(88,300,364,48,TFT_WHITE);
    M5.Display.drawRoundRect(92, 305, 356, 38, 12, TFT_BLACK);
    const int fill = static_cast<int>(344 * progress); if (fill > 0) M5.Display.fillRoundRect(98,311,fill,26,8,TFT_BLACK);
  } else if (c.pomodoroStyle.style() == PomodoroStyle::Focus) {
    M5.Display.drawCircle(cx,cy,174,TFT_BLACK); M5.Display.drawCircle(cx,cy,171,TFT_BLACK);
    M5.Display.drawArc(cx,cy,150,160,-90,static_cast<int>(-90+360*progress),TFT_BLACK);
  }
  // This central region is intentionally inside the inner ring, so updating
  // digits cannot erase progress pixels.
  M5.Display.fillRoundRect(142,258,256,126,10,TFT_WHITE);
  M5.Display.setTextDatum(MC_DATUM); M5.Display.setFont(nullptr); M5.Display.setTextSize(7); M5.Display.drawString(txt,cx,cy-28); M5.Display.setTextSize(1);
  M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.drawString(running_?(paused_?"PAUSED":"RUNNING"):"READY",cx,cy+42);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.drawString(c.pomodoroStyle.name(),cx,cy+72);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}
void PomodoroApp::drawButtons(){
  const int xs[]={30,200,370};
  const char* preset[]={"25 MIN","5 MIN","30 MIN"};
  for(int i=0;i<3;++i) boldButton(xs[i],720,140,60,ui::Icon::Timer,preset[i]);
  const ui::Icon controls[]={ui::Icon::Play,ui::Icon::Pause,ui::Icon::Stop}; const char* labels[]={"START","PAUSE","STOP"};
  for(int i=0;i<3;++i) boldButton(xs[i],800,140,60,controls[i],labels[i]);
}
void PomodoroApp::drawContent(AppContext& c){ drawTimer(c,true); M5.Display.fillRect(20,700,500,165,TFT_WHITE); drawButtons(); }
void PomodoroApp::draw(AppContext& c,bool full){if(full){M5.Display.fillScreen(TFT_WHITE);ui::Chrome::drawHeader(c,"Pomodoro");ui::Chrome::drawFooter(pomodoroChrome());ui::Theme::drawButtonFrame(470,70,46,46);ui::drawIcon(ui::Icon::Refresh,493,93,26);}drawContent(c);}
void PomodoroApp::start(){if(!running_){running_=true;elapsedSeconds_=0;}paused_=false;lastSecondMs_=millis();M5.Speaker.tone(800,100);}void PomodoroApp::stop(){running_=paused_=false;elapsedSeconds_=0;M5.Speaker.tone(400,100);}void PomodoroApp::pause(){if(running_){paused_=true;M5.Speaker.tone(600,100);}}void PomodoroApp::selectDuration(uint8_t m){durationMinutes_=m;running_=paused_=false;elapsedSeconds_=0;}
void PomodoroApp::refresh(AppContext& c){draw(c,true);}
void PomodoroApp::handleTap(AppContext& c,int x,int y){lastActivityMs_=millis();if(ui::Chrome::hitTestFooter(x,y,pomodoroChrome())!=ui::FooterAction::None){if(navigator_)navigator_(AppId::Launcher);return;}if(x>=470&&x<=516&&y>=70&&y<=116){refresh(c);return;}if(y>=720&&y<=780){if(x>=30&&x<=170)selectDuration(25);else if(x>=200&&x<=340)selectDuration(5);else if(x>=370&&x<=510)selectDuration(30);drawTimer(c,true);M5.Display.display(70,120,400,400);return;}if(y>=800&&y<=860){if(x>=30&&x<=170)start();else if(x>=200&&x<=340)pause();else if(x>=370&&x<=510)stop();drawTimer(c,true);M5.Display.display(70,120,400,400);return;}if(y>=120&&y<=520){c.pomodoroStyle.next();drawTimer(c,true);M5.Display.display(70,120,400,400);}}
void PomodoroApp::updateTimer(AppContext& c,uint32_t now){if(!running_||paused_||now-lastSecondMs_<1000)return;lastSecondMs_+=1000;++elapsedSeconds_;if(elapsedSeconds_>=durationMinutes_*60UL){running_=paused_=false;elapsedSeconds_=0;M5.Speaker.tone(600,500);drawTimer(c,true);M5.Display.display(70,120,400,400);return;}drawTimer(c);M5.Display.display(70,120,400,400);}
void PomodoroApp::onTick(AppContext& c,uint32_t now){const auto&t=M5.Touch.getDetail();if(locked_){if(t.wasPressed()){locked_=false;lastActivityMs_=now;draw(c,true);}return;}if(t.wasPressed())handleTap(c,t.x,t.y);updateTimer(c,now);if(!running_&&now-lastActivityMs_>5UL*60UL*1000UL){locked_=true;M5.Display.fillScreen(TFT_WHITE);if(c.storage.mounted()&&SD.exists("/PaperOS/pomodoro/pomodoro.png"))M5.Display.drawPngFile(SD,"/PaperOS/pomodoro/pomodoro.png",0,200);M5.Display.setTextDatum(MC_DATUM);M5.Display.setTextSize(4);M5.Display.drawString("POMODORO",270,430);M5.Display.setTextSize(2);M5.Display.drawString("Sleep mode - tap to resume",270,480);M5.Display.setTextDatum(TL_DATUM);}}
void PomodoroApp::onStop(AppContext&){running_=paused_=false;}
