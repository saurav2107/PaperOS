#include "apps/images/ImagesApp.h"

#include <SD.h>
#include <M5Unified.h>
#include <algorithm>
#include "app/AppContext.h"
#include "services/Services.h"
#include "ui/Chrome.h"
#include "ui/Icons.h"

namespace {
constexpr const char* kPhotosFolder = "/PaperOS/photos";

// ESP32 SD implementations are inconsistent here: openNextFile().name() may
// be a full path, a path relative to the opened folder, or just a filename.
// Normalise it once so every decoder receives an absolute SD-card path.
String photoPath(const String& entryName) {
  if (entryName.startsWith(String(kPhotosFolder) + "/")) return entryName;
  String filename = entryName;
  while (filename.startsWith("/")) filename.remove(0, 1);
  return String(kPhotosFolder) + "/" + filename;
}

uint16_t readBigEndian16(File& file) { const int a=file.read(), b=file.read(); return a < 0 || b < 0 ? 0 : (uint16_t(a) << 8) | uint16_t(b); }
uint32_t readBigEndian32(File& file) { const uint16_t hi=readBigEndian16(file), lo=readBigEndian16(file); return (uint32_t(hi) << 16) | lo; }
uint32_t readLittleEndian32(File& file) { uint32_t value=0; for(uint8_t i=0;i<4;++i){const int byte=file.read();if(byte<0)return 0;value|=uint32_t(byte)<<(i*8);}return value; }

// Read only image metadata, never the decoded bitmap. This lets the app
// calculate a centred contain-fit scale without consuming PSRAM for a copy.
bool imageSize(const String& path, int& width, int& height) {
  width=height=0; File file=SD.open(path,FILE_READ); if(!file)return false;
  const String lower=[](String value){value.toLowerCase();return value;}(path);
  if(lower.endsWith(".png")) {
    file.seek(16); width=static_cast<int>(readBigEndian32(file)); height=static_cast<int>(readBigEndian32(file));
  } else if(lower.endsWith(".bmp")) {
    file.seek(18); width=static_cast<int>(readLittleEndian32(file)); height=static_cast<int>(readLittleEndian32(file)); if(height<0)height=-height;
  } else {
    // JPEG dimensions are in an SOF segment. Scan markers; EXIF is not
    // decoded here because M5GFX already handles the actual image stream.
    if(file.read()!=0xFF || file.read()!=0xD8){file.close();return false;}
    while(file.available()) { int marker; do { marker=file.read(); } while(marker==0xFF && file.available()); if(marker<0)break;
      if(marker==0xD8 || marker==0xD9)continue; const uint16_t length=readBigEndian16(file); if(length<2)break;
      if((marker>=0xC0&&marker<=0xC3)||(marker>=0xC5&&marker<=0xC7)||(marker>=0xC9&&marker<=0xCB)||(marker>=0xCD&&marker<=0xCF)) { file.read(); height=readBigEndian16(file); width=readBigEndian16(file); break; }
      file.seek(file.position()+length-2);
    }
  }
  file.close(); return width>0 && height>0;
}
}

bool ImagesApp::onStart(AppContext& context) {
  fullScreen_=false; lastTapMs_=0;
  scanLibrary(context);
  draw(context);
  return true;
}

bool ImagesApp::isSupported(const String& path) const {
  return path.endsWith(".jpg") || path.endsWith(".JPG")
      || path.endsWith(".jpeg") || path.endsWith(".JPEG")
      || path.endsWith(".png") || path.endsWith(".PNG")
      || path.endsWith(".bmp") || path.endsWith(".BMP");
}

void ImagesApp::scanLibrary(AppContext& context) {
  imageCount_ = current_ = 0;
  if (!context.storage.mounted()) { status_ = "INSERT A MICROSD CARD"; return; }
  SD.mkdir("/PaperOS"); SD.mkdir(kPhotosFolder);
  File folder = SD.open(kPhotosFolder);
  if (!folder || !folder.isDirectory()) { status_ = "COULD NOT OPEN /PaperOS/photos"; return; }
  while (imageCount_ < MaxImages) {
    File entry = folder.openNextFile();
    if (!entry) break;
    const String path = photoPath(String(entry.name()));
    if (!entry.isDirectory() && isSupported(path)) images_[imageCount_++] = path;
    entry.close();
  }
  folder.close();
  status_ = imageCount_ ? "" : "COPY JPG, PNG, OR BMP FILES TO /PaperOS/photos";
}

ui::ChromeOptions ImagesApp::chromeOptions() const {
  ui::ChromeOptions options;
  options.showBack = false;
  options.showHome = true;
  options.showPrevious = current_ > 0;
  options.showNext = imageCount_ && current_ + 1 < imageCount_;
  return options;
}

void ImagesApp::drawImage() {
  // The application content is deliberately bounded below the common header
  // and footer.  The source image may be larger; M5GFX clips it to the panel.
  const int header = fullScreen_ ? 0 : ui::Chrome::headerHeight(), footer = fullScreen_ ? M5.Display.height() : ui::Chrome::footerTop();
  const int detailTop = fullScreen_ ? footer : footer - 42;
  M5.Display.fillRect(0, header, M5.Display.width(), footer - header, TFT_WHITE);
  if (!imageCount_) return;
  // Decoding a large SD image can take a noticeable moment on e-paper. Draw
  // and commit a static loading card before opening the decoder, so the UI
  // communicates that the tap was accepted rather than appearing frozen.
  drawLoading();
  M5.Display.display(0, header, M5.Display.width(), footer - header);
  M5.Display.waitDisplay();
  const String& path = images_[current_];
  Serial.printf("[PHOTOS] rendering %s\n", path.c_str());
  int sourceWidth=0, sourceHeight=0;
  if(!imageSize(path,sourceWidth,sourceHeight)) { status_="COULD NOT READ IMAGE SIZE"; return; }
  const int canvasWidth=M5.Display.width()-20, canvasHeight=detailTop-header-16;
  const float scale=std::min(float(canvasWidth)/sourceWidth,float(canvasHeight)/sourceHeight);
  const int renderedWidth=static_cast<int>(sourceWidth*scale), renderedHeight=static_cast<int>(sourceHeight*scale);
  const int x=(M5.Display.width()-renderedWidth)/2, y=header+(canvasHeight-renderedHeight)/2;
  if (path.endsWith(".jpg") || path.endsWith(".JPG") || path.endsWith(".jpeg") || path.endsWith(".JPEG"))
    M5.Display.drawJpgFile(SD, path.c_str(), x, y, 0, 0, 0, 0, scale, scale);
  else if (path.endsWith(".png") || path.endsWith(".PNG"))
    M5.Display.drawPngFile(SD, path.c_str(), x, y, 0, 0, 0, 0, scale, scale);
  else
    M5.Display.drawBmpFile(SD, path.c_str(), x, y, 0, 0, 0, 0, scale, scale);
}

void ImagesApp::drawLoading() const {
  const int header = fullScreen_ ? 0 : ui::Chrome::headerHeight();
  const int footer = fullScreen_ ? M5.Display.height() : ui::Chrome::footerTop();
  // A compact, explicit loading state prevents a large SD-card image decode
  // from looking like a frozen e-paper panel.  The fixed bar is intentionally
  // static: animation would itself cause distracting screen flashing.
  const int cardW = 264, cardH = 70, x = (M5.Display.width() - cardW) / 2, y = (header + footer) / 2 - cardH / 2;
  M5.Display.fillRoundRect(x, y, cardW, cardH, 9, TFT_BLACK);
  M5.Display.fillRoundRect(x + 3, y + 3, cardW - 6, cardH - 6, 6, TFT_WHITE);
  ui::drawIcon(ui::Icon::Image, x + 29, y + 27, 24);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(ML_DATUM);
  M5.Display.drawString("Loading image", x + 53, y + 24);
  const int barX = x + 53, barY = y + 42, barW = cardW - 76;
  M5.Display.drawRoundRect(barX, barY, barW, 11, 5, TFT_BLACK);
  M5.Display.fillRoundRect(barX + 3, barY + 3, (barW - 6) * 3 / 5, 5, 2, TFT_BLACK);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}

void ImagesApp::draw(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE);
  if(fullScreen_){drawImage();return;}
  ui::Chrome::drawHeader(context, "Photos");
  drawImage();
  if (imageCount_) {
    const int detailsY = ui::Chrome::footerTop() - 31;
    M5.Display.fillRect(12, detailsY - 4, 516, 30, TFT_WHITE);
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    String details = String(current_ + 1) + " / " + imageCount_ + "  " + images_[current_];
    while(details.length() && M5.Display.textWidth(details) > 492) details.remove(details.length()-1);
    if(details.length() < String(current_ + 1).length() + 5) details=String(current_ + 1)+" / "+imageCount_;
    else if(details.length() < String(current_ + 1).length() + 5 + images_[current_].length()) details += "...";
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.drawString(details, M5.Display.width()/2, detailsY + 8);
    M5.Display.setTextDatum(TL_DATUM);
    M5.Display.setFont(nullptr);
  } else {
    const int contentCenter = (ui::Chrome::headerHeight() + ui::Chrome::footerTop()) / 2;
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setFont(&fonts::FreeSans12pt7b);
    M5.Display.drawString("NO PHOTOS FOUND", M5.Display.width() / 2, contentCenter - 24);
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.drawString(status_, M5.Display.width() / 2, contentCenter + 20);
    M5.Display.setTextDatum(TL_DATUM);
    M5.Display.setFont(nullptr);
  }
  ui::Chrome::drawFooter(chromeOptions());
}

void ImagesApp::onTick(AppContext& context, uint32_t) {
  const auto& touch = M5.Touch.getDetail();
  if (!touch.wasPressed() && !touch.wasReleased()) return;
  if(fullScreen_){
    if(touch.wasPressed()){const uint32_t now=millis();if(now-lastTapMs_<380UL){fullScreen_=false;lastTapMs_=0;draw(context);return;}lastTapMs_=now;return;}
    if(touch.wasReleased()){const int dx=touch.x-touch.base_x;if(dx>70&&current_>0){--current_;draw(context);}else if(dx<-70&&current_+1<imageCount_){++current_;draw(context);}return;}
  }
  if(touch.wasPressed()&&touch.y>ui::Chrome::headerHeight()&&touch.y<ui::Chrome::footerTop()) {const uint32_t now=millis();if(now-lastTapMs_<380UL){fullScreen_=true;lastTapMs_=0;draw(context);return;}lastTapMs_=now;}
  const auto action = touch.wasPressed() ? ui::Chrome::hitTestFooter(touch.x, touch.y, chromeOptions()) : ui::Chrome::swipeAction(touch.base_x,touch.base_y,touch.x,touch.y,chromeOptions());
  if (action == ui::FooterAction::Home) {
    if (navigator_) navigator_(AppId::Launcher);
  } else if (action == ui::FooterAction::Previous && current_ > 0) {
    --current_; draw(context);
  } else if (action == ui::FooterAction::Next && current_ + 1 < imageCount_) {
    ++current_; draw(context);
  }
}
