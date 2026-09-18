#include "apps/todo/TodoApp.h"
#include <ctype.h>
#include <SD.h>
#include <M5Unified.h>
#include "app/AppContext.h"
#include "services/Services.h"
#include "ui/Chrome.h"
#include "ui/Icons.h"
#include "ui/Theme.h"

namespace {
constexpr const char* kTasksFile = "/PaperOS/todo/tasks.txt";
String todayKey(AppContext& context) { tm local{}; context.time.localTime(local); char value[11]; snprintf(value,sizeof(value),"%04d-%02d-%02d",local.tm_year+1900,local.tm_mon+1,local.tm_mday); return String(value); }
}

bool TodoApp::onStart(AppContext& context) { load(context); editing_ = false; shift_ = false; draw(context); return true; }
void TodoApp::load(AppContext& context) {
  count_ = 0; status_ = "";
  if (!context.storage.mounted()) { status_ = "SD card is not mounted"; return; }
  if (!SD.exists("/PaperOS")) SD.mkdir("/PaperOS"); if (!SD.exists("/PaperOS/todo")) SD.mkdir("/PaperOS/todo");
  File file = SD.open(kTasksFile, FILE_READ); if (!file) return;
  while (file.available() && count_ < 6) { String line = file.readStringUntil('\n'); line.trim(); if (line.length() < 3) continue; complete_[count_] = line[0] == '1'; const int separator=line.indexOf('|',2); if(separator>=0){dueDates_[count_]=line.substring(2,separator);tasks_[count_]=line.substring(separator+1);}else{dueDates_[count_]=todayKey(context);tasks_[count_]=line.substring(2);} ++count_; }
  file.close();
}
void TodoApp::save(AppContext& context) {
  if (!context.storage.mounted()) { status_ = "SD card is not mounted"; return; }
  if (!SD.exists("/PaperOS")) SD.mkdir("/PaperOS"); if (!SD.exists("/PaperOS/todo")) SD.mkdir("/PaperOS/todo");
  if (SD.exists(kTasksFile)) SD.remove(kTasksFile);
  File file = SD.open(kTasksFile, FILE_WRITE); if (!file) { status_ = "Could not save tasks"; return; }
  for (uint8_t i=0;i<count_;++i) file.printf("%c|%s|%s\n", complete_[i] ? '1' : '0', dueDates_[i].c_str(), tasks_[i].c_str());
  file.close(); status_ = "Saved to SD: /PaperOS/todo/tasks.txt";
}
void TodoApp::draw(AppContext& context) { M5.Display.fillScreen(TFT_WHITE); ui::ChromeOptions chrome; chrome.showBack=false; chrome.showHome=true; ui::Chrome::drawHeader(context,"To Do"); if(editing_) drawEditor(context); else drawList(context); ui::Chrome::drawFooter(chrome); }
void TodoApp::drawList(AppContext& context) {
  M5.Display.setTextSize(1); M5.Display.setCursor(24,88); M5.Display.print(status_.isEmpty() ? String(count_) + " TASKS ON SD" : status_);
  for(uint8_t i=0;i<count_;++i) { const int y=118+i*86; ui::Theme::drawFrame(22,y,496,70,8); ui::drawIcon(complete_[i]?ui::Icon::Check:ui::Icon::Note,52,y+35,28); M5.Display.setTextSize(2); String label=tasks_[i]; if(label.length()>25) label=label.substring(0,22)+"..."; M5.Display.drawString(label,78,y+22); ui::Theme::drawButtonFrame(454,y+12,48,46);ui::drawIcon(ui::Icon::Delete,478,y+35,24); }
  ui::Theme::drawButtonFrame(130,730,280,62); ui::drawIcon(ui::Icon::FolderAdd,164,761,30); M5.Display.setTextSize(2); M5.Display.drawString("ADD TASK",200,748);
  (void)context;
}
void TodoApp::drawEditor(AppContext&) {
  M5.Display.setTextSize(1); M5.Display.setCursor(24,92); M5.Display.print("NEW TASK FOR TODAY - TAP LETTERS, THEN SAVE");
  drawDraftField();
  drawKeyboard();
}
void TodoApp::drawDraftField(){M5.Display.fillRect(18,118,504,420,TFT_WHITE);ui::Theme::drawFrame(18,118,504,390);M5.Display.setTextSize(2);M5.Display.setCursor(36,144);M5.Display.print(draft_);M5.Display.setTextSize(1);M5.Display.setCursor(24,520);M5.Display.print("Tasks save to /PaperOS/todo and appear in Calendar.");}
void TodoApp::drawActionRows(){auto button=[](int x,int y,int w,int h){ui::Theme::drawButtonFrame(x,y,w,h);};button(12,738,56,52);if(shift_)M5.Display.fillRoundRect(16,742,48,44,5,TFT_BLACK);ui::drawIcon(ui::Icon::Shift,40,764,20);const char leading[]={',','.'};for(int i=0;i<2;++i){const int x=74+i*38;button(x,738,32,52);M5.Display.setTextSize(2);char key[]={leading[i],0};M5.Display.drawString(key,x+10,753);}button(150,738,180,52);M5.Display.setTextSize(1);M5.Display.drawString("SPACE",214,756);const char trailing[]={'?','!'};for(int i=0;i<2;++i){const int x=336+i*38;button(x,738,32,52);M5.Display.setTextSize(2);char key[]={trailing[i],0};M5.Display.drawString(key,x+10,753);}button(412,738,116,52);ui::drawIcon(ui::Icon::Backspace,434,764,22);M5.Display.setTextSize(1);M5.Display.drawString("BACK",450,756);button(110,800,150,60);ui::drawIcon(ui::Icon::Check,136,830,26);M5.Display.setFont(&fonts::FreeSans9pt7b);M5.Display.setTextDatum(ML_DATUM);M5.Display.drawString("SAVE",158,830);M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);button(280,800,150,60);ui::drawIcon(ui::Icon::Delete,306,830,26);M5.Display.setFont(&fonts::FreeSans9pt7b);M5.Display.setTextDatum(ML_DATUM);M5.Display.drawString("CLEAR",328,830);M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);}
void TodoApp::drawKeyboard(){const char* rows[]={"QWERTYUIOP","ASDFGHJKL","ZXCVBNM"};const int starts[]={18,43,93};for(int row=0;row<3;++row)for(int col=0;rows[row][col];++col){const int x=starts[row]+col*50,y=558+row*60;M5.Display.fillRoundRect(x,y,46,52,7,TFT_BLACK);M5.Display.fillRoundRect(x+3,y+3,40,46,5,TFT_WHITE);char key[]={rows[row][col],0};if(!shift_)key[0]=tolower(key[0]);M5.Display.setTextSize(2);M5.Display.drawString(key,x+15,y+14);}drawActionRows();}
void TodoApp::handleTap(AppContext& context,int x,int y) {
  ui::ChromeOptions chrome; chrome.showBack=false; chrome.showHome=true; if(ui::Chrome::hitTestFooter(x,y,chrome)==ui::FooterAction::Home){if(navigator_)navigator_(AppId::Launcher);return;}
  if(!editing_) { if(x>=130&&x<=410&&y>=730&&y<=792){editing_=true;draft_="";shift_=false;draw(context);return;} for(uint8_t i=0;i<count_;++i){const int rowY=118+i*86;if(y>=rowY&&y<=rowY+70&&x>=22&&x<=518){if(x>=454){for(uint8_t move=i;move+1<count_;++move){tasks_[move]=tasks_[move+1];dueDates_[move]=dueDates_[move+1];complete_[move]=complete_[move+1];}--count_;status_="Task deleted";}else{complete_[i]=!complete_[i];status_=complete_[i]?"Task completed":"Task reopened";}save(context);draw(context);return;}} return; }
  if(y>=558&&y<730){const int row=(y-558)/60;const char* rows[]={"QWERTYUIOP","ASDFGHJKL","ZXCVBNM"};const int starts[]={18,43,93};const int col=(x-starts[row])/50;if(row>=0&&row<3&&x>=starts[row]&&col>=0&&rows[row][col]&&draft_.length()<48){char value=rows[row][col];if(!shift_)value=tolower(value);draft_+=value;drawDraftField();}return;}
  if(y>=738&&y<=790){if(x>=12&&x<68){shift_=!shift_;drawKeyboard();}else if(x>=74&&x<106){draft_+=',';drawDraftField();}else if(x>=112&&x<144){draft_+='.';drawDraftField();}else if(x>=150&&x<330&&draft_.length()<48){draft_+=' ';drawDraftField();}else if(x>=336&&x<368){draft_+='?';drawDraftField();}else if(x>=374&&x<406){draft_+='!';drawDraftField();}else if(x>=412&&x<528&&!draft_.isEmpty()){draft_.remove(draft_.length()-1);drawDraftField();}return;}
  if(y>=800&&y<=860){if(x>=110&&x<=260&&draft_.length()&&count_<6){tasks_[count_]=draft_;dueDates_[count_]=todayKey(context);complete_[count_]=false;++count_;save(context);editing_=false;draw(context);}else if(x>=280&&x<=430){draft_="";drawDraftField();}}
}
void TodoApp::onTick(AppContext& context,uint32_t){const auto& touch=M5.Touch.getDetail();if(touch.wasPressed())handleTap(context,touch.x,touch.y);}
