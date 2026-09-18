#include "apps/calendar/CalendarApp.h"
#include <SD.h>
#include <time.h>
#include <M5Unified.h>
#include "app/AppContext.h"
#include "services/Services.h"
#include "ui/Chrome.h"
#include "ui/Icons.h"

namespace {
String dateKey(int year, int month, int day) {
  char value[11]; snprintf(value, sizeof(value), "%04d-%02d-%02d", year, month + 1, day); return String(value);
}
}

bool CalendarApp::onStart(AppContext& context) {
  tm today{}; context.time.localTime(today);
  year_ = today.tm_year + 1900; month_ = today.tm_mon; selectedDay_ = today.tm_mday;
  draw(context); return true;
}

void CalendarApp::changeMonth(int delta) {
  tm value{}; value.tm_year = year_ - 1900; value.tm_mon = month_ + delta; value.tm_mday = 1; value.tm_isdst = -1;
  mktime(&value); year_ = value.tm_year + 1900; month_ = value.tm_mon; selectedDay_ = 1;
}

void CalendarApp::draw(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE); ui::ChromeOptions chrome; chrome.showBack=false; chrome.showHome=true; chrome.showPrevious=true; chrome.showNext=true; ui::Chrome::drawHeader(context,"Calendar");
  tm today{}; context.time.localTime(today);
  tm first{}; first.tm_year=year_-1900; first.tm_mon=month_; first.tm_mday=1; first.tm_isdst=-1; mktime(&first);
  char monthTitle[32]; strftime(monthTitle,sizeof(monthTitle),"%B %Y",&first);
  M5.Display.setTextDatum(MC_DATUM); M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.drawString(monthTitle,270,96); M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
  // Native month grid: first.tm_wday supplies the Sunday offset and the
  // days-in-month calculation handles leap years without a calendar library.
  const int firstWeekday=first.tm_wday;
  const int year=year_, month=month_; const bool leap=(year%4==0&&(year%100!=0||year%400==0));
  const int days[]={31,leap?29:28,31,30,31,30,31,31,30,31,30,31}; const int gridX=22,gridY=126,cellW=71,cellH=39;
  const char* weekdays[]={"SUN","MON","TUE","WED","THU","FRI","SAT"};
  for(int col=0;col<7;++col){M5.Display.drawRect(gridX+col*cellW,gridY,cellW,28,TFT_BLACK);M5.Display.setTextSize(1);M5.Display.drawString(weekdays[col],gridX+col*cellW+14,gridY+9);}
  for(int day=1;day<=days[month];++day){const int index=firstWeekday+day-1,row=index/7,col=index%7,x=gridX+col*cellW,y=gridY+28+row*cellH;M5.Display.drawRect(x,y,cellW,cellH,TFT_BLACK);const bool selected=day==selectedDay_;if(selected){M5.Display.fillRoundRect(x+5,y+5,cellW-10,cellH-10,5,TFT_BLACK);M5.Display.setTextColor(TFT_WHITE,TFT_BLACK);}else M5.Display.setTextColor(TFT_BLACK,TFT_WHITE);M5.Display.setTextSize(2);char number[4];snprintf(number,sizeof(number),"%d",day);M5.Display.drawString(number,x+cellW/2-M5.Display.textWidth(number)/2,y+11);M5.Display.setTextColor(TFT_BLACK,TFT_WHITE);}
  const String selectedDate=dateKey(year_,month_,selectedDay_);
  const int tasksTop=414; M5.Display.setTextSize(2);M5.Display.setCursor(24,tasksTop);M5.Display.print(selectedDate==dateKey(today.tm_year+1900,today.tm_mon,today.tm_mday)?"TODAY TASKS":"TASKS FOR "+selectedDate);
  uint8_t row=0; if(context.storage.mounted() && SD.exists("/PaperOS/todo/tasks.txt")){File file=SD.open("/PaperOS/todo/tasks.txt",FILE_READ);while(file.available()&&row<6){String line=file.readStringUntil('\n');line.trim();if(line.length()<3||line[0]=='1')continue;const int delimiter=line.indexOf('|',2);const String due=delimiter>=0?line.substring(2,delimiter):dateKey(today.tm_year+1900,today.tm_mon,today.tm_mday);if(due!=selectedDate)continue;const int y=tasksTop+38+row*62;M5.Display.drawRoundRect(22,y,496,52,7,TFT_BLACK);ui::drawIcon(ui::Icon::Check,48,y+26,22);M5.Display.setTextSize(1);String label=delimiter>=0?line.substring(delimiter+1):line.substring(2);if(label.length()>43)label=label.substring(0,40)+"...";M5.Display.drawString(label,76,y+19);++row;}file.close();}
  if(row==0){M5.Display.setTextSize(1);M5.Display.setCursor(24,tasksTop+42);M5.Display.print("No open tasks for this date.");}
  ui::Chrome::drawFooter(chrome);
}
void CalendarApp::onTick(AppContext& context,uint32_t){const auto& touch=M5.Touch.getDetail();if(!touch.wasPressed())return;ui::ChromeOptions chrome;chrome.showBack=false;chrome.showHome=true;chrome.showPrevious=true;chrome.showNext=true;const auto action=ui::Chrome::hitTestFooter(touch.x,touch.y,chrome);if(action==ui::FooterAction::Home){if(navigator_)navigator_(AppId::Launcher);return;}if(action==ui::FooterAction::Previous){changeMonth(-1);draw(context);return;}if(action==ui::FooterAction::Next){changeMonth(1);draw(context);return;}const int gridX=22,gridY=126,cellW=71,cellH=39;const int firstWeekday=[](int year,int month){tm value{};value.tm_year=year-1900;value.tm_mon=month;value.tm_mday=1;value.tm_isdst=-1;mktime(&value);return value.tm_wday;}(year_,month_);if(touch.x>=gridX&&touch.x<gridX+cellW*7&&touch.y>=gridY+28&&touch.y<gridY+28+cellH*6){const int col=(touch.x-gridX)/cellW,row=(touch.y-gridY-28)/cellH,day=row*7+col-firstWeekday+1;const bool leap=(year_%4==0&&(year_%100!=0||year_%400==0));const int days[]={31,leap?29:28,31,30,31,30,31,31,30,31,30,31};if(day>=1&&day<=days[month_]){selectedDay_=day;draw(context);}}}
