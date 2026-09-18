#include "apps/utilities/UtilitiesApps.h"
#include <M5Unified.h>
#include <SD.h>
#include "app/AppContext.h"
#include "services/Services.h"
#include "ui/Chrome.h"
#include "ui/Icons.h"
#include "ui/Theme.h"

namespace {
void button(int x, int y, int w, int h, const char* label) {
  M5.Display.fillRoundRect(x,y,w,h,7,TFT_BLACK); M5.Display.fillRoundRect(x+3,y+3,w-6,h-6,4,TFT_WHITE);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString(label,x+w/2,y+h/2);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}
// Calculator-specific key treatment: number keys stay light, while the
// operation column follows the supplied reference with a solid dark surface
// and high-contrast label. Both retain the OS's 3px construction.
void calculatorKey(int x, int y, int w, int h, const char* label, bool operation) {
  M5.Display.fillRoundRect(x, y, w, h, 9, TFT_BLACK);
  if (!operation) M5.Display.fillRoundRect(x + 3, y + 3, w - 6, h - 6, 6, TFT_WHITE);
  else M5.Display.drawRoundRect(x + 3, y + 3, w - 6, h - 6, 6, TFT_WHITE);
  // One shared bold face and size for every keypad label—including +/-, %,
  // AC, M+, MR and operators—keeps visual weight and centering consistent.
  M5.Display.setFont(&fonts::FreeSans24pt7b);
  M5.Display.setTextColor(operation ? TFT_WHITE : TFT_BLACK, operation ? TFT_BLACK : TFT_WHITE);
  M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString(label, x + w / 2, y + h / 2);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setTextColor(TFT_BLACK, TFT_WHITE); M5.Display.setFont(nullptr);
}
void line(int y, const String& label, const String& value) {
  M5.Display.fillRoundRect(28,y,484,66,7,TFT_BLACK); M5.Display.fillRoundRect(31,y+3,478,60,4,TFT_WHITE);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(ML_DATUM); M5.Display.drawString(label,48,y+33); M5.Display.setTextDatum(MR_DATUM); M5.Display.drawString(value,492,y+33); M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}
void monitorRow(int y, ui::Icon icon, const char* label, const String& value) {
  M5.Display.fillRect(28,y,484,82,TFT_WHITE); ui::drawIcon(icon,58,y+31,30);
  M5.Display.setFont(&fonts::FreeSans12pt7b);M5.Display.setTextDatum(ML_DATUM);M5.Display.drawString(label,92,y+25);
  M5.Display.setFont(&fonts::FreeSans9pt7b);M5.Display.drawString(value,92,y+53);
  M5.Display.fillRect(48,y+70,444,3,TFT_BLACK);M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);
}
ui::ChromeOptions footer() { ui::ChromeOptions c; c.showBack=false; c.showHome=true; return c; }
// Leave a deliberately generous status/action gutter below the canvas. This
// trims roughly five percent from the drawable height but prevents partial
// refreshes from cutting into the action row's upper border.
int writingToolbarTop() { return ui::Chrome::headerHeight() + 50; }
int writingCanvasBottom() { return ui::Chrome::footerTop() - 18; }
int writingStatusY() { return ui::Chrome::headerHeight() + 23; }
}

void UtilityAppBase::drawShell(AppContext& context, const char* title) const { M5.Display.fillScreen(TFT_WHITE); ui::Chrome::drawHeader(context,title); ui::Chrome::drawFooter(footer()); }
bool UtilityAppBase::handleHome(int x,int y) const { return ui::Chrome::hitTestFooter(x,y,footer()) == ui::FooterAction::Home; }

bool CalculatorApp::onStart(AppContext& context) { value_="0"; previousEntry_=""; accumulator_=0.0f; pending_=0; startsNewValue_=true; draw(context); return true; }
void CalculatorApp::draw(AppContext& context) {
  drawShell(context,title());
  drawDisplay();
  // Top control row keeps memory and one-character delete with the keypad,
  // leaving the display purely for the calculation and its history.
  calculatorKey(22,370,115,64,"AC",true); calculatorKey(146,370,115,64,"M+",true);
  calculatorKey(270,370,115,64,"MR",true); calculatorKey(394,370,115,64,"",true);
  // Bold back-arrow (not a whole-page navigation action): removes one digit.
  M5.Display.fillTriangle(424,402,450,386,450,418,TFT_WHITE); M5.Display.fillRect(448,397,28,10,TFT_WHITE);
  const char* rows[][4] = {{"+/-","%","/","*"},{"7","8","9","-"},{"4","5","6","+"},{"1","2","3",""}};
  for (int row=0; row<4; ++row) for (int col=0; col<4; ++col) {
    if (!rows[row][col][0]) continue;
    const bool operation = row == 0 || col == 3;
    calculatorKey(22+col*124,444+row*74,115,64,rows[row][col],operation);
  }
  calculatorKey(22,740,115,64,"0",false); calculatorKey(146,740,115,64,".",false);
  // Equals begins immediately after the "3" column and occupies two rows.
  calculatorKey(394,666,115,138,"=",true);
}
void CalculatorApp::drawDisplay() const {
  // This is the only region redrawn after a calculation. Header, footer and
  // keypad remain untouched, avoiding a full-screen e-paper flash per tap.
  M5.Display.fillRect(18,82,504,250,TFT_WHITE);
  M5.Display.fillRoundRect(18,82,504,250,11,TFT_BLACK); M5.Display.fillRoundRect(21,85,498,244,8,TFT_WHITE);
  M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.setTextDatum(MR_DATUM);
  M5.Display.drawString(previousEntry_,494,158);
  // Orbitron is the bundled digital/LCD-style face. It gives numerical
  // results a distinct calculator display appearance without an SD font.
  M5.Display.setFont(&fonts::Orbitron_Light_32); M5.Display.setTextSize(1.4f); M5.Display.drawString(value_,494,266);
  M5.Display.setTextSize(1); M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}
void CalculatorApp::applyPending() {
  const float rhs = value_.toFloat();
  switch (pending_) { case '+': accumulator_+=rhs; break; case '-': accumulator_-=rhs; break; case '*': accumulator_*=rhs; break; case '/': if (rhs != 0.0f) accumulator_/=rhs; break; }
  value_ = String(accumulator_, 6); while (value_.endsWith("0")) value_.remove(value_.length()-1); if (value_.endsWith(".")) value_.remove(value_.length()-1);
}
void CalculatorApp::onTick(AppContext& context,uint32_t) {
  const auto& t=M5.Touch.getDetail(); if(!t.wasPressed())return;
  if(handleHome(t.x,t.y)){if(navigator_)navigator_(AppId::Launcher);return;}
  const char* key = "";
  if (t.y >= 370 && t.y <= 434) {
    const int col=(t.x-22)/124; if(col<0||col>3)return;
    if (col == 0) key="AC"; else if(col == 1) key="M+"; else if(col == 2) key="MR"; else key="BACK";
  } else if (t.x >= 394 && t.x <= 509 && t.y >= 666 && t.y <= 804) {
    key="=";
  } else if (t.x >= 22 && t.x <= 509 && t.y >= 444 && t.y < 740) {
    const int col=(t.x-22)/124,row=(t.y-444)/74; if(col<0||col>3||row<0||row>3)return;
    static const char* const rows[][4] = {{"+/-","%","/","*"},{"7","8","9","-"},{"4","5","6","+"},{"1","2","3",""}};
    key=rows[row][col];
  } else if (t.x >= 22 && t.x <= 261 && t.y >= 740 && t.y <= 804) {
    key=t.x<137?"0":".";
  } else return;
  if(!key[0])return;
  if(String(key)=="M+") { memory_ += value_.toFloat(); previousEntry_ = "M = " + String(memory_, 4); drawDisplay(); M5.Display.display(18,82,504,250); return; }
  if(String(key)=="MR") { value_ = String(memory_, 6); while(value_.endsWith("0"))value_.remove(value_.length()-1); if(value_.endsWith("."))value_.remove(value_.length()-1); previousEntry_ = "MEMORY RECALL"; startsNewValue_ = false; drawDisplay(); M5.Display.display(18,82,504,250); return; }
  if(String(key)=="BACK") { if(!startsNewValue_ && value_.length()>1)value_.remove(value_.length()-1); else value_="0"; drawDisplay(); M5.Display.display(18,82,504,250); return; }
  if(key[0]>='0'&&key[0]<='9') { if(startsNewValue_){value_=key;startsNewValue_=false;} else if(value_.length()<12)value_+=key; }
  else if(key[0]=='.') { if(startsNewValue_){value_="0.";startsNewValue_=false;} else if(value_.indexOf('.')<0)value_+='.'; }
  else if(String(key)=="AC") { value_="0";previousEntry_="";accumulator_=0;pending_=0;startsNewValue_=true; }
  else if(String(key)=="+/-") { if(value_!="0") value_ = value_.startsWith("-") ? value_.substring(1) : "-"+value_; }
  else if(key[0]=='%') {
    // Match a normal pocket calculator rather than merely dividing by 100:
    // 200 + 10 % = 220, and 200 - 10 % = 180. For multiplication/division,
    // percentage remains the literal fractional value (10 % becomes 0.1).
    float percent = value_.toFloat() / 100.0f;
    if (pending_ == '+' || pending_ == '-') percent *= accumulator_;
    value_ = String(percent, 6);
    while (value_.endsWith("0")) value_.remove(value_.length()-1);
    if (value_.endsWith(".")) value_.remove(value_.length()-1);
    startsNewValue_ = false;
  }
  else if(key[0]=='=') {
    if(pending_ && !startsNewValue_) { previousEntry_=String(accumulator_, 5)+" "+String(pending_)+" "+value_+" ="; applyPending();pending_=0;startsNewValue_=true; }
  }
  else {
    if(pending_ && !startsNewValue_) { previousEntry_=String(accumulator_, 5)+" "+String(pending_)+" "+value_+" ="; applyPending(); }
    else accumulator_=value_.toFloat();
    pending_=key[0]; previousEntry_=value_+" "+String(pending_); startsNewValue_=true;
  }
  drawDisplay();
  M5.Display.display(18, 82, 504, 250);
}

bool ConverterApp::onStart(AppContext& context) { value_="1"; unit_=0; draw(context); return true; }
void ConverterApp::draw(AppContext& context) {
  drawShell(context,title());
  const char* labels[] = {"Celsius to Fahrenheit", "Kilometres to Miles", "Kilograms to Pounds", "Litres to Gallons", "Centimetres to Inches", "Kilopascal to PSI"};
  const float factors[] = {1.0f, .621371f, 2.20462f, .264172f, .393701f, .145038f};
  const float input=value_.toFloat(); const float output=unit_==0 ? input*1.8f+32.0f : input*factors[unit_];
  line(112,"Conversion",labels[unit_]); line(190,"Input",value_); line(268,"Result",String(output,2));
  button(100,362,340,58,"CHANGE UNIT");
  const char* keys[]={"1","2","3","4","5","6","7","8","9","CLEAR","0","BACK"};
  for(int i=0;i<12;++i) button(54+(i%3)*150,446+(i/3)*68,132,52,keys[i]);
}
void ConverterApp::onTick(AppContext& context,uint32_t) {
  const auto& t=M5.Touch.getDetail(); if(!t.wasPressed())return; if(handleHome(t.x,t.y)){if(navigator_)navigator_(AppId::Launcher);return;}
  if(t.y>=362&&t.y<=420){unit_=(unit_+1)%6;draw(context);return;} if(t.y<446||t.y>718)return;
  int col=(t.x-54)/150,row=(t.y-446)/68; if(col<0||col>2||row<0||row>3)return; int i=row*3+col;
  if(i==9)value_=""; else if(i==11){if(value_.length())value_.remove(value_.length()-1);} else if(i==10){if(value_.length()<8)value_+='0';} else if(i<9&&value_.length()<8)value_+=char('1'+i); draw(context);
}

bool AlarmApp::onStart(AppContext& context) { draw(context); return true; }
void AlarmApp::draw(AppContext& context) { drawShell(context,title()); char time[12]; snprintf(time,sizeof(time),"%02u:%02u",hour_,minute_); M5.Display.setFont(&fonts::FreeSans24pt7b); M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString(time,270,200); M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr); line(280,"Daily alarm",enabled_?"ON":"OFF"); button(54,390,132,68,"HOUR +"); button(204,390,132,68,"MIN +"); button(354,390,132,68,enabled_?"DISABLE":"ENABLE"); M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString("Alarm schedule is retained while Paper OS is running.",270,520); M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr); }
void AlarmApp::onTick(AppContext& context,uint32_t) { const auto& t=M5.Touch.getDetail(); if(!t.wasPressed())return; if(handleHome(t.x,t.y)){if(navigator_)navigator_(AppId::Launcher);return;} if(t.y<390||t.y>458)return; if(t.x<186)hour_=(hour_+1)%24; else if(t.x<336)minute_=(minute_+5)%60; else enabled_=!enabled_; draw(context); }

bool ShoppingListApp::onStart(AppContext& context) { load(context); draw(context); return true; }
void ShoppingListApp::load(AppContext& context) { count_=0; status_=""; if(!context.storage.mounted()){status_="SD card not mounted";return;} SD.mkdir("/PaperOS");SD.mkdir("/PaperOS/shopping"); File f=SD.open("/PaperOS/shopping/list.txt",FILE_READ); while(f&&f.available()&&count_<6){String s=f.readStringUntil('\n');s.trim();if(s.length()>2){checked_[count_]=s[0]=='1';items_[count_++]=s.substring(2);}} if(f)f.close(); }
void ShoppingListApp::save(AppContext& context) { if(!context.storage.mounted())return; SD.mkdir("/PaperOS");SD.mkdir("/PaperOS/shopping"); File f=SD.open("/PaperOS/shopping/list.txt",FILE_WRITE); if(!f){status_="Could not save";return;} for(uint8_t i=0;i<count_;++i)f.printf("%c|%s\n",checked_[i]?'1':'0',items_[i].c_str()); f.close(); status_="Saved to SD"; }
void ShoppingListApp::draw(AppContext& context) { drawShell(context,title()); M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.drawString(status_.isEmpty()?"Tap an item to mark it complete":status_,30,95); M5.Display.setFont(nullptr); for(uint8_t i=0;i<count_;++i){line(130+i*76,items_[i],checked_[i]?"DONE":"");} button(150,620,240,62,"ADD ITEM"); }
void ShoppingListApp::onTick(AppContext& context,uint32_t) { const auto& t=M5.Touch.getDetail(); if(!t.wasPressed())return; if(handleHome(t.x,t.y)){if(navigator_)navigator_(AppId::Launcher);return;} if(t.y>=130&&t.y<130+count_*76){uint8_t i=(t.y-130)/76;checked_[i]=!checked_[i];save(context);draw(context);return;} if(t.y>=620&&t.y<=682&&count_<6){items_[count_]="Shopping item "+String(count_+1);checked_[count_]=false;++count_;save(context);draw(context);} }

bool BackupRestoreApp::onStart(AppContext& context) { status_="Create a safe settings snapshot on SD"; draw(context); return true; }
void BackupRestoreApp::draw(AppContext& context) { drawShell(context,title()); line(130,"SD storage",context.storage.mounted()?"READY":"NOT MOUNTED"); line(212,"Backup folder","/PaperOS/backup"); M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString(status_,270,350); M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr); button(120,440,300,64,"CREATE SNAPSHOT"); }
void BackupRestoreApp::onTick(AppContext& context,uint32_t) { const auto& t=M5.Touch.getDetail();if(!t.wasPressed())return;if(handleHome(t.x,t.y)){if(navigator_)navigator_(AppId::Launcher);return;}if(t.y>=440&&t.y<=504){if(!context.storage.mounted())status_="SD card is not mounted";else{if(!SD.exists("/PaperOS"))SD.mkdir("/PaperOS");if(!SD.exists("/PaperOS/backup"))SD.mkdir("/PaperOS/backup");File f=SD.open("/PaperOS/backup/system.txt",FILE_WRITE);if(f){f.printf("Paper OS snapshot\nFree heap: %u\n",ESP.getFreeHeap());f.close();status_="Snapshot saved to /PaperOS/backup";}else status_="Could not write snapshot";}draw(context);}}

bool WritingApp::onStart(AppContext& context) {
  clear();
  savedPath_ = ""; libraryOpen_ = false;
  status_ = context.storage.mounted() ? "Draw with your finger" : "SD card not mounted - save unavailable";
  draw(context);
  return true;
}

bool WritingApp::withinCanvas(int x, int y) const {
  return x >= kCanvasLeft + 3 && x <= kCanvasRight - 3 && y >= kCanvasTop + 3 && y <= writingCanvasBottom() - 3;
}

void WritingApp::drawCanvas() const {
  const int bottom = writingCanvasBottom();
  ui::Theme::drawFrame(kCanvasLeft, kCanvasTop, kCanvasRight - kCanvasLeft, bottom - kCanvasTop, 10);
}

void WritingApp::drawActions() const {
  constexpr int width = 72, gap = 4, height = 64;
  const int y = writingToolbarTop();
  const char* labels[] = {"PEN", "ERASE", "UNDO", "CLEAR", "NEW", "VIEW", "SAVE"};
  const ui::Icon icons[] = {ui::Icon::Pen, ui::Icon::Eraser, ui::Icon::Undo, ui::Icon::Delete, ui::Icon::FolderAdd, ui::Icon::Folder, ui::Icon::Check};
  for (int i = 0; i < 7; ++i) {
    const int x = 6 + i * (width + gap);
    const bool selected = (i == 0 && !eraser_) || (i == 1 && eraser_);
    ui::Theme::drawButtonFrame(x, y, width, height);
    if (selected) M5.Display.fillRect(x + 8, y + 6, width - 16, 4, TFT_BLACK);
    ui::drawIcon(icons[i], x + width / 2, y + 26, 24);
    M5.Display.setFont(&fonts::FreeSansBold9pt7b); M5.Display.setTextDatum(MC_DATUM);
    M5.Display.drawString(labels[i], x + width / 2, y + 50);
  }
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setTextColor(TFT_BLACK, TFT_WHITE); M5.Display.setFont(nullptr);
}

void WritingApp::drawStatus() const {
  const int y = writingStatusY();
  M5.Display.fillRect(18, y - 18, 504, 34, TFT_WHITE);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(MC_DATUM);
  String message = status_;
  while (message.length() && M5.Display.textWidth(message) > 490) message.remove(message.length() - 1);
  if (message != status_) message += "...";
  M5.Display.drawString(message, 270, y);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}

void WritingApp::draw(AppContext& context) {
  drawShell(context, title());
  drawCanvas();
  drawStrokes();
  drawStatus();
  drawActions();
}

void WritingApp::scanDrawings(AppContext& context) {
  sketchCount_ = 0; sketchPage_ = 0;
  if (!context.storage.mounted()) { status_ = "SD card is not mounted"; return; }
  File root = SD.open("/PaperOS/writing");
  if (!root || !root.isDirectory()) { if (root) root.close(); status_ = "No saved drawings"; return; }
  while (sketchCount_ < kMaxSketches) {
    File folder = root.openNextFile(); if (!folder) break;
    if (!folder.isDirectory()) { folder.close(); continue; }
    String folderPath = String(folder.name());
    if (!folderPath.startsWith("/PaperOS/writing/")) folderPath = String("/PaperOS/writing/") + folderPath;
    while (sketchCount_ < kMaxSketches) {
      File entry = folder.openNextFile(); if (!entry) break;
      String name = String(entry.name());
      if (!entry.isDirectory() && name.endsWith(".draw")) {
        if (!name.startsWith(folderPath + "/")) name = folderPath + "/" + name.substring(name.lastIndexOf('/') + 1);
        sketches_[sketchCount_++] = name;
      }
      entry.close();
    }
    folder.close();
  }
  root.close();
  status_ = sketchCount_ ? String(sketchCount_) + " saved drawings" : "No saved drawings";
}

void WritingApp::drawLibrary(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE); ui::Chrome::drawHeader(context, "Saved sketches");
  ui::ChromeOptions chrome; chrome.showBack = true; chrome.showHome = true;
  chrome.showPrevious = sketchPage_ > 0; chrome.showNext = (sketchPage_ + 1) * kSketchesPerPage < sketchCount_;
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString(status_, 270, 94); M5.Display.setTextDatum(TL_DATUM);
  const uint8_t first = sketchPage_ * kSketchesPerPage;
  for (uint8_t row = 0; row < kSketchesPerPage && first + row < sketchCount_; ++row) {
    const int y = 116 + row * 96; const String& path = sketches_[first + row];
    M5.Display.fillRoundRect(24, y, 492, 78, 8, TFT_BLACK); M5.Display.fillRoundRect(27, y + 3, 486, 72, 5, TFT_WHITE);
    String label = path.substring(String("/PaperOS/writing/").length());
    M5.Display.setFont(&fonts::FreeSansBold9pt7b); M5.Display.drawString(label, 48, y + 30);
    M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.drawString("Tap to open", 48, y + 56);
    ui::drawIcon(ui::Icon::ArrowRight, 490, y + 39, ui::Theme::NextIconSize); M5.Display.setTextDatum(TL_DATUM);
  }
  M5.Display.setFont(nullptr); ui::Chrome::drawFooter(chrome);
}

void WritingApp::drawSegment(const Point& from, const Point& to, bool eraser) const {
  const uint16_t colour = eraser ? TFT_WHITE : TFT_BLACK;
  if (eraser) {
    // A 22px circular eraser has a clear, deliberate feel on the PaperS3.
    M5.Display.fillCircle(to.x, to.y, 11, colour);
    M5.Display.drawLine(from.x, from.y, to.x, to.y, colour);
  } else {
    // Parallel lines approximate a 4px pen without allocating a sprite.
    for (int offset = -2; offset <= 2; ++offset) M5.Display.drawLine(from.x + offset, from.y, to.x + offset, to.y, colour);
    M5.Display.fillCircle(to.x, to.y, 2, colour);
  }
}

void WritingApp::refreshSegment(const Point& from, const Point& to) const {
  const int padding = eraser_ ? 14 : 5;
  int left = from.x < to.x ? from.x : to.x, right = from.x > to.x ? from.x : to.x;
  int top = from.y < to.y ? from.y : to.y, bottom = from.y > to.y ? from.y : to.y;
  left = left - padding < kCanvasLeft ? kCanvasLeft : left - padding;
  right = right + padding > kCanvasRight ? kCanvasRight : right + padding;
  top = top - padding < kCanvasTop ? kCanvasTop : top - padding;
  bottom = bottom + padding > writingCanvasBottom() ? writingCanvasBottom() : bottom + padding;
  // The normal drawing path refreshes only the small rectangle containing the
  // new ink. Header, footer, action row and prior strokes stay untouched.
  M5.Display.display(left, top, right - left + 1, bottom - top + 1);
}

void WritingApp::drawStrokes() const {
  for (uint8_t i = 0; i < strokeCount_; ++i) {
    const Stroke& stroke = strokes_[i];
    for (uint16_t p = 1; p < stroke.count; ++p) drawSegment(points_[stroke.first + p - 1], points_[stroke.first + p], stroke.eraser);
    if (stroke.count == 1) drawSegment(points_[stroke.first], points_[stroke.first], stroke.eraser);
  }
}

void WritingApp::redrawCanvas() const {
  drawCanvas();
  drawStrokes();
  M5.Display.display(kCanvasLeft, kCanvasTop, kCanvasRight - kCanvasLeft, writingCanvasBottom() - kCanvasTop);
}

void WritingApp::clear() { pointCount_ = 0; strokeCount_ = 0; drawing_ = false; eraser_ = false; hasInk_ = false; }
void WritingApp::newDrawing() { clear(); savedPath_ = ""; status_ = "New canvas"; }

void WritingApp::undo() {
  if (!strokeCount_) { status_ = "Nothing to undo"; return; }
  const Stroke& removed = strokes_[strokeCount_ - 1];
  pointCount_ = removed.first;
  --strokeCount_;
  hasInk_ = false; for (uint8_t i = 0; i < strokeCount_; ++i) if (!strokes_[i].eraser) { hasInk_ = true; break; }
  status_ = "Last stroke removed";
  redrawCanvas();
}

void WritingApp::save(AppContext& context) {
  if (!context.storage.mounted()) { status_ = "SD card is not mounted"; return; }
  if (!hasInk_) { status_ = "Nothing to save"; return; }
  tm local{};
  String folder = "/PaperOS/writing/undated";
  if (context.time.localTime(local)) {
    char dated[32]; snprintf(dated, sizeof(dated), "/PaperOS/writing/%04d-%02d-%02d", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
    folder = dated;
  }
  SD.mkdir("/PaperOS"); SD.mkdir("/PaperOS/writing"); SD.mkdir(folder);
  String path = savedPath_;
  if (path.isEmpty()) {
    for (uint16_t serial = 1; serial <= 999; ++serial) {
      char candidate[64]; snprintf(candidate, sizeof(candidate), "%s/%03u.draw", folder.c_str(), serial);
      path = candidate;
      if (!SD.exists(path)) break;
    }
  }
  const String temporary = path + ".part";
  if (SD.exists(temporary)) SD.remove(temporary);
  File file = SD.open(temporary, FILE_WRITE);
  if (!file) { status_ = "Could not save drawing"; return; }
  file.println("PAPERS3_DRAW_V1");
  for (uint8_t i = 0; i < strokeCount_; ++i) {
    const Stroke& stroke = strokes_[i];
    file.printf("S,%u,%u,%u\n", stroke.eraser ? 1 : 0, stroke.first, stroke.count);
    for (uint16_t p = 0; p < stroke.count; ++p) {
      const Point& point = points_[stroke.first + p];
      file.printf("P,%d,%d\n", point.x, point.y);
    }
  }
  file.close();
  if (SD.exists(path)) SD.remove(path);
  if (!SD.rename(temporary, path)) { if (SD.exists(temporary)) SD.remove(temporary); status_ = "Could not replace drawing"; return; }
  savedPath_ = path;
  status_ = "Saved " + path.substring(path.lastIndexOf('/') + 1);
}

bool WritingApp::loadDrawing(const String& path) {
  File file = SD.open(path, FILE_READ); if (!file) return false;
  clear(); String line;
  while (file.available()) {
    line = file.readStringUntil('\n'); line.trim();
    if (line.startsWith("S,")) {
      unsigned int erased = 0, ignored = 0, expected = 0;
      if (sscanf(line.c_str(), "S,%u,%u,%u", &erased, &ignored, &expected) == 3 && strokeCount_ < kMaxStrokes)
        strokes_[strokeCount_++] = {pointCount_, 0, erased != 0};
    } else if (line.startsWith("P,") && strokeCount_ && pointCount_ < kMaxPoints) {
      int x = 0, y = 0;
      if (sscanf(line.c_str(), "P,%d,%d", &x, &y) == 2) { points_[pointCount_++] = {static_cast<int16_t>(x), static_cast<int16_t>(y)}; ++strokes_[strokeCount_ - 1].count; }
    }
  }
  file.close();
  for (uint8_t i = 0; i < strokeCount_; ++i) if (!strokes_[i].eraser && strokes_[i].count) { hasInk_ = true; break; }
  Serial.printf("[SKETCH] loaded %s: %u strokes, %u points\n", path.c_str(), strokeCount_, pointCount_);
  savedPath_ = path; status_ = "Opened " + path.substring(path.lastIndexOf('/') + 1); return true;
}

void WritingApp::onTick(AppContext& context, uint32_t) {
  const auto& touch = M5.Touch.getDetail();
  if (libraryOpen_) {
    if (!touch.wasPressed()) return;
    ui::ChromeOptions chrome; chrome.showBack = true; chrome.showHome = true; chrome.showPrevious = sketchPage_ > 0; chrome.showNext = (sketchPage_ + 1) * kSketchesPerPage < sketchCount_;
    const auto action = ui::Chrome::hitTestFooter(touch.x, touch.y, chrome);
    if (action == ui::FooterAction::Home) { if (navigator_) navigator_(AppId::Launcher); return; }
    if (action == ui::FooterAction::Back) { libraryOpen_ = false; draw(context); return; }
    if (action == ui::FooterAction::Previous) { --sketchPage_; drawLibrary(context); return; }
    if (action == ui::FooterAction::Next) { ++sketchPage_; drawLibrary(context); return; }
    const int row = (touch.y - 116) / 96, index = sketchPage_ * kSketchesPerPage + row;
    if (touch.y >= 116 && row >= 0 && row < kSketchesPerPage && index < sketchCount_) { if (loadDrawing(sketches_[index])) { libraryOpen_ = false; draw(context); } else { status_ = "Could not open drawing"; drawLibrary(context); } }
    return;
  }
  if (touch.wasPressed() && handleHome(touch.x, touch.y)) { if (navigator_) navigator_(AppId::Launcher); return; }
  if (touch.wasPressed() && touch.y >= writingToolbarTop() && touch.y <= writingToolbarTop() + 64) {
    const int item = (touch.x - 6) / 76;
    if (item < 0 || item > 6 || touch.x < 6 + item * 76 || touch.x > 78 + item * 76) return;
    if (item == 0) { eraser_ = false; status_ = "Pen ready"; drawActions(); }
    if (item == 1) { eraser_ = true; status_ = "Eraser ready"; drawActions(); }
    if (item == 2) undo();
    if (item == 3) { clear(); status_ = "Canvas cleared"; redrawCanvas(); drawActions(); }
    if (item == 4) { newDrawing(); redrawCanvas(); drawActions(); }
    if (item == 5) { libraryOpen_ = true; scanDrawings(context); drawLibrary(context); return; }
    if (item == 6) save(context);
    drawStatus();
    M5.Display.display(0, ui::Chrome::headerHeight(), M5.Display.width(), kCanvasTop - ui::Chrome::headerHeight());
    return;
  }
  if (touch.wasPressed() && withinCanvas(touch.x, touch.y)) {
    if (strokeCount_ >= kMaxStrokes || pointCount_ >= kMaxPoints) {
      status_ = strokeCount_ >= kMaxStrokes ? "Stroke limit reached - save or start new" : "Point limit reached - save or start new";
      drawStatus(); M5.Display.display(18, writingStatusY() - 16, 504, 32); return;
    }
    strokes_[strokeCount_] = {pointCount_, 1, eraser_};
    if (!eraser_) hasInk_ = true;
    lastPoint_ = {touch.x, touch.y}; points_[pointCount_++] = lastPoint_; ++strokeCount_; drawing_ = true;
    drawSegment(lastPoint_, lastPoint_, eraser_); refreshSegment(lastPoint_, lastPoint_);
    return;
  }
  if (drawing_ && touch.isPressed()) {
    if (!withinCanvas(touch.x, touch.y)) return;
    const int dx = touch.x - lastPoint_.x, dy = touch.y - lastPoint_.y;
    // Keep one point per ~5px movement. It still renders smooth handwriting,
    // but avoids recording several indistinguishable controller samples.
    if (dx * dx + dy * dy < 25) return;
    if (pointCount_ >= kMaxPoints) { drawing_ = false; status_ = "Point limit reached - save or start new"; drawStatus(); M5.Display.display(18, writingStatusY() - 16, 504, 32); return; }
    Point next{touch.x, touch.y};
    drawSegment(lastPoint_, next, eraser_);
    points_[pointCount_++] = next; ++strokes_[strokeCount_ - 1].count;
    refreshSegment(lastPoint_, next); lastPoint_ = next;
  }
  if (touch.wasReleased()) drawing_ = false;
}

bool SystemMonitorApp::onStart(AppContext& context) { draw(context); return true; }
void SystemMonitorApp::draw(AppContext& context) {
  // System Monitor is opened by Settings, therefore its footer returns to
  // Settings rather than unexpectedly dropping the user at the launcher.
  M5.Display.fillScreen(TFT_WHITE); ui::Chrome::drawHeader(context,title());
  ui::ChromeOptions chrome; chrome.showBack=true; chrome.showHome=false; ui::Chrome::drawFooter(chrome);
  drawReadings(context);
}
void SystemMonitorApp::drawReadings(AppContext& context) {
  monitorRow(110,ui::Icon::System,"Free heap",String(ESP.getFreeHeap()/1024)+" KB");
  monitorRow(200,ui::Icon::System,"Free PSRAM",String(ESP.getFreePsram()/1024)+" KB");
  monitorRow(290,ui::Icon::Battery,"Battery",String(context.power.batteryPercent())+"%");
  monitorRow(380,ui::Icon::Wifi,"Wi-Fi",context.network.connected()?"CONNECTED":"OFFLINE");
  monitorRow(470,ui::Icon::Folder,"SD card",context.storage.mounted()?"MOUNTED":"NOT MOUNTED");
  M5.Display.fillRect(36,580,468,36,TFT_WHITE);M5.Display.setFont(&fonts::FreeSans9pt7b);M5.Display.setTextDatum(MC_DATUM);M5.Display.drawString("Refreshes only this panel every 10 seconds",270,598);M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);
}
void SystemMonitorApp::onTick(AppContext& context,uint32_t now) {
  const auto&t=M5.Touch.getDetail(); ui::ChromeOptions chrome; chrome.showBack=true; chrome.showHome=false;
  if(t.wasPressed()&&ui::Chrome::hitTestFooter(t.x,t.y,chrome)==ui::FooterAction::Back){if(navigator_)navigator_(AppId::Settings);return;}
  if(now-lastDrawMs_>10000){lastDrawMs_=now;drawReadings(context);M5.Display.display(28,110,484,506);}
}
