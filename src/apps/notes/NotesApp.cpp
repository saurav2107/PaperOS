#include "apps/notes/NotesApp.h"
#include <SD.h>
#include <M5Unified.h>
#include "app/AppContext.h"
#include "services/Services.h"
#include "ui/Chrome.h"
#include "ui/Icons.h"
#include "ui/Theme.h"
#include <time.h>

namespace {
void framedButton(int x,int y,int w,int h){ui::Theme::drawButtonFrame(x,y,w,h);}
}

bool NotesApp::onStart(AppContext& context){text_="";shift_=false;status_=context.storage.mounted()?"New note - type then Save":"Insert SD card to save notes";draw(context);return true;}
void NotesApp::append(char c){if(text_.length()<240)text_+=c;}
void NotesApp::save(AppContext& context){
  if(!context.storage.mounted()){status_="SD card is not mounted";return;}if(text_.isEmpty()){status_="Write something before saving";return;}
  tm local{}; if(!context.time.localTime(local)){status_="Set device time before saving notes";return;}
  char date[16];strftime(date,sizeof(date),"%Y-%m-%d",&local); SD.mkdir("/PaperOS");SD.mkdir("/PaperOS/notes");const String folder=String("/PaperOS/notes/")+date; if(!SD.exists(folder))SD.mkdir(folder);
  uint16_t sequence=1;String path;do{char fileName[20];snprintf(fileName,sizeof(fileName),"note_%03u.txt",sequence++);path=folder+"/"+fileName;}while(SD.exists(path)&&sequence<1000);
  File f=SD.open(path,FILE_WRITE);if(!f){status_="Could not create note";return;}f.print(text_);f.close();status_="Saved: "+path;text_="";
}
void NotesApp::drawTextField(){M5.Display.fillRect(18,84,504,458,TFT_WHITE);framedButton(18,84,504,420);M5.Display.setTextSize(2);M5.Display.setCursor(34,108);M5.Display.print(text_);M5.Display.setTextSize(1);M5.Display.setCursor(26,520);M5.Display.print(status_);}
void NotesApp::drawActionRow(){
  // Compact edit row: SHIFT, comma/period, SPACE, question/exclamation, BACK.
  framedButton(12,738,56,52); if(shift_) M5.Display.fillRoundRect(16,742,48,44,5,TFT_BLACK);
  ui::drawIcon(ui::Icon::Shift,40,764,20);
  const char leadingSymbols[]={',','.'}; for(int i=0;i<2;++i){const int x=74+i*38;framedButton(x,738,32,52);M5.Display.setTextSize(2);char key[]={leadingSymbols[i],0};M5.Display.drawString(key,x+10,753);}
  framedButton(150,738,180,52);M5.Display.setTextSize(1);M5.Display.drawString("SPACE",214,756);
  const char trailingSymbols[]={'?','!'}; for(int i=0;i<2;++i){const int x=336+i*38;framedButton(x,738,32,52);M5.Display.setTextSize(2);char key[]={trailingSymbols[i],0};M5.Display.drawString(key,x+10,753);}
  framedButton(412,738,116,52);ui::drawIcon(ui::Icon::Backspace,434,764,22);M5.Display.setTextSize(1);M5.Display.drawString("BACK",450,756);
  // Match the compact 150 x 60 Pomodoro controls, including icon-first labels.
  framedButton(110,800,150,60);ui::drawIcon(ui::Icon::Check,136,830,26);M5.Display.setFont(&fonts::FreeSans9pt7b);M5.Display.setTextDatum(ML_DATUM);M5.Display.drawString("SAVE",158,830);M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);
  framedButton(280,800,150,60);ui::drawIcon(ui::Icon::Delete,306,830,26);M5.Display.setFont(&fonts::FreeSans9pt7b);M5.Display.setTextDatum(ML_DATUM);M5.Display.drawString("CLEAR",328,830);M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);
}
void NotesApp::drawKeyboard(){const char* rows[]={"QWERTYUIOP","ASDFGHJKL","ZXCVBNM"};const int starts[]={18,43,93};for(int r=0;r<3;++r)for(int i=0;rows[r][i];++i){const int x=starts[r]+i*50,y=558+r*60;framedButton(x,y,46,52);char key[]={rows[r][i],0};if(!shift_)key[0]=tolower(key[0]);M5.Display.setTextSize(2);M5.Display.drawString(key,x+15,y+14);}drawActionRow();}
void NotesApp::draw(AppContext& context){M5.Display.clear();ui::Chrome::drawHeader(context,"Notes");ui::ChromeOptions chrome;chrome.showBack=false;chrome.showHome=true;ui::Chrome::drawFooter(chrome);drawTextField();drawKeyboard();}
void NotesApp::onTick(AppContext& context,uint32_t){const auto&t=M5.Touch.getDetail();if(!t.wasPressed())return;if(ui::Chrome::hitTestFooter(t.x,t.y)!=ui::FooterAction::None){if(navigator_)navigator_(AppId::Launcher);return;}if(t.y>=558&&t.y<730){const int row=(t.y-558)/60;const char* rows[]={"QWERTYUIOP","ASDFGHJKL","ZXCVBNM"};const int starts[]={18,43,93};const int col=(t.x-starts[row])/50;if(row>=0&&row<3&&t.x>=starts[row]&&col>=0&&rows[row][col]){char value=rows[row][col];if(!shift_)value=tolower(value);append(value);drawTextField();}return;}if(t.y>=738&&t.y<=790){if(t.x>=12&&t.x<68){shift_=!shift_;drawKeyboard();}else if(t.x>=74&&t.x<106){append(',');drawTextField();}else if(t.x>=112&&t.x<144){append('.');drawTextField();}else if(t.x>=150&&t.x<330){append(' ');drawTextField();}else if(t.x>=336&&t.x<368){append('?');drawTextField();}else if(t.x>=374&&t.x<406){append('!');drawTextField();}else if(t.x>=412&&t.x<528&&!text_.isEmpty()){text_.remove(text_.length()-1);drawTextField();}return;}if(t.y>=800&&t.y<=860){if(t.x>=110&&t.x<=260){save(context);drawTextField();}else if(t.x>=280&&t.x<=430){text_="";status_="Note cleared";drawTextField();}}}
