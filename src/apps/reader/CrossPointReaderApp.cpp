#include "apps/reader/CrossPointReaderApp.h"

#include <SD.h>
#include <M5Unified.h>
#include <Preferences.h>
#include <ctype.h>
#include "app/AppContext.h"
#include "services/Services.h"
#include "ui/Chrome.h"
#include "ui/Icons.h"
#include "ui/Theme.h"

namespace {
bool isEpub(const String& name) { return name.endsWith(".epub") || name.endsWith(".EPUB"); }
bool isText(const String& name) { return name.endsWith(".txt") || name.endsWith(".TXT"); }
bool isMarkdown(const String& name) { return name.endsWith(".md") || name.endsWith(".MD") || name.endsWith(".markdown"); }
String libraryPath(const String& entryName) {
  // Arduino SD implementations differ: some return a full path from
  // openNextFile(), others return only a name relative to the open folder.
  if (entryName.startsWith("/PaperOS/books/")) return entryName;
  String relative = entryName;
  while (relative.startsWith("/")) relative.remove(0, 1);
  return String("/PaperOS/books/") + relative;
}
String fileSizeLabel(uint32_t bytes) {
  if (bytes >= 1024UL * 1024UL) return String(bytes / (1024.0f * 1024.0f), 1) + " MB";
  return String((bytes + 1023) / 1024) + " KB";
}
String regularCase(String value) {
  value.toLowerCase();
  bool firstLetter = true;
  for (size_t i = 0; i < value.length(); ++i) {
    if (isAlpha(value[i])) { if (firstLetter) value.setCharAt(i, toupper(value[i])); firstLetter = false; }
  }
  return value;
}
uint32_t pathHash(const String& value) {
  uint32_t hash = 2166136261UL;
  for (size_t i = 0; i < value.length(); ++i) { hash ^= static_cast<uint8_t>(value[i]); hash *= 16777619UL; }
  return hash;
}
String progressKey(char prefix, const String& path) { char key[14]; snprintf(key, sizeof(key), "%c%08lx", prefix, static_cast<unsigned long>(pathHash(path))); return String(key); }
String epubProgressKey(const char* prefix, const String& path) {
  char key[16]; snprintf(key, sizeof(key), "%s%08lx", prefix, static_cast<unsigned long>(pathHash(path))); return String(key);
}

void frame(int x, int y, int width, int height, bool selected = false) {
  ui::Theme::drawFrame(x, y, width, height, 10, selected);
}
void drawWrapped(const String& value, int x, int y, int width, int maxLines, uint16_t colour, uint16_t background) {
  M5.Display.setTextColor(colour, background);
  String line;
  int lineNumber = 0;
  int start = 0;
  while (start < static_cast<int>(value.length()) && lineNumber < maxLines) {
    int split = value.indexOf(' ', start);
    if (split < 0) split = value.length();
    const String word = value.substring(start, split);
    const String candidate = line.isEmpty() ? word : line + " " + word;
    if (!line.isEmpty() && M5.Display.textWidth(candidate) > width) {
      M5.Display.drawString(line, x, y + lineNumber * 27);
      line = word;
      ++lineNumber;
    } else {
      line = candidate;
    }
    start = split + 1;
  }
  if (lineNumber < maxLines && !line.isEmpty()) {
    if (start < static_cast<int>(value.length())) {
      while (line.length() && M5.Display.textWidth(line + "...") > width) line.remove(line.length() - 1);
      line += "...";
    }
    M5.Display.drawString(line, x, y + lineNumber * 27);
  }
}
void drawAppearanceRow(int y, const char* label, const String& value) {
  ui::Theme::drawFrame(20, y, 500, 94, 8);
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(ML_DATUM);
  M5.Display.drawString(label, 44, y + 47);
  if (value == ">") ui::drawIcon(ui::Icon::ArrowRight, 490, y + 47, ui::Theme::NextIconSize);
  else { M5.Display.setFont(&fonts::FreeSansBold12pt7b); M5.Display.setTextDatum(MR_DATUM); M5.Display.drawString(value, 492, y + 47); }
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}
}

bool CrossPointReaderApp::onStart(AppContext& context) {
  loadAppearance();
  scanLibrary(context); draw(context); return true;
}

void CrossPointReaderApp::loadAppearance() {
  Preferences preferences; preferences.begin("paperreader", true);
  readerFont_ = preferences.getUChar("font", 0) > 0 ? 1 : 0;
  readerScale_ = preferences.getUChar("scale", 1) > 1 ? 2 : 1;
  const uint8_t spacing = preferences.getUChar("spacing", 1);
  readerSpacing_ = spacing > 2 ? 1 : spacing;
  preferences.end();
}

void CrossPointReaderApp::saveAppearance() const {
  Preferences preferences; preferences.begin("paperreader", false);
  preferences.putUChar("font", readerFont_);
  preferences.putUChar("scale", readerScale_);
  preferences.putUChar("spacing", readerSpacing_);
  preferences.end();
}

void CrossPointReaderApp::selectReaderBodyFont() const {
  M5.Display.setFont(readerFont_ == 0 ? &fonts::FreeSerif12pt7b : &fonts::FreeSans12pt7b);
  M5.Display.setTextSize(readerScale_);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
}

void CrossPointReaderApp::resetEpubPagination() {
  textPage_ = 0;
  epubOffsets_[0] = {0, 0};
  epubPageEnd_ = {0, 0};
}

uint8_t CrossPointReaderApp::epubProgressPercent() const {
  if (!readingEpub_ || !epub_.chapterCount()) return 0;
  const uint32_t completed = static_cast<uint32_t>(epubChapter_) * 100;
  const uint32_t within = epub_.blockCount() ? static_cast<uint32_t>(epubOffsets_[textPage_].block) * 100 / epub_.blockCount() : 0;
  const uint32_t value = (completed + within) / epub_.chapterCount();
  return value > 100 ? 100 : static_cast<uint8_t>(value);
}

bool CrossPointReaderApp::jumpToEpubPercent(uint8_t percent, String& error) {
  if (!readingEpub_ || !epub_.chapterCount()) return false;
  const uint16_t targetChapter = percent >= 100 ? epub_.chapterCount() - 1
    : static_cast<uint16_t>((static_cast<uint32_t>(percent) * epub_.chapterCount()) / 100);
  if (targetChapter != epubChapter_ && !epub_.loadChapter(targetChapter, error)) return false;
  epubChapter_ = targetChapter;
  const uint8_t within = percent >= 100 ? 100 : static_cast<uint8_t>((static_cast<uint32_t>(percent) * epub_.chapterCount()) % 100);
  const uint16_t targetBlock = epub_.blockCount() ? static_cast<uint16_t>((static_cast<uint32_t>(within) * epub_.blockCount()) / 100) : 0;
  textPage_ = 0; epubOffsets_[0] = {targetBlock, 0}; epubPageEnd_ = epubOffsets_[0];
  saveEpubProgress(books_[selected_].path, epubChapter_, epubOffsets_[0]);
  return true;
}

String CrossPointReaderApp::displayName(const String& path) {
  const int slash = path.lastIndexOf('/');
  return slash >= 0 ? path.substring(slash + 1) : path;
}

const char* CrossPointReaderApp::formatLabel(BookFormat format) {
  switch (format) {
    case BookFormat::Epub: return "EPUB";
    case BookFormat::Markdown: return "MARKDOWN";
    case BookFormat::PdfPages: return "PDF";
    default: return "TEXT";
  }
}

bool CrossPointReaderApp::loadProgress(const String& path, uint32_t& offset) {
  Preferences preferences; preferences.begin("paperreader", true);
  const bool opened = preferences.getBool(progressKey('o', path).c_str(), false);
  offset = preferences.getULong(progressKey('p', path).c_str(), 0);
  preferences.end();
  return opened;
}

void CrossPointReaderApp::saveProgress(const String& path, uint32_t offset) {
  Preferences preferences; preferences.begin("paperreader", false);
  preferences.putBool(progressKey('o', path).c_str(), true);
  preferences.putULong(progressKey('p', path).c_str(), offset);
  preferences.end();
}

bool CrossPointReaderApp::loadEpubProgress(const String& path, uint16_t& chapter, EpubPosition& position) {
  Preferences preferences; preferences.begin("paperreader", true);
  const bool opened = preferences.getBool(epubProgressKey("eo", path).c_str(), false);
  chapter = preferences.getUShort(epubProgressKey("ec", path).c_str(), 0);
  position.block = preferences.getUShort(epubProgressKey("eb", path).c_str(), 0);
  position.character = preferences.getUShort(epubProgressKey("ep", path).c_str(), 0);
  preferences.end();
  return opened;
}

void CrossPointReaderApp::saveEpubProgress(const String& path, uint16_t chapter, const EpubPosition& position) {
  Preferences preferences; preferences.begin("paperreader", false);
  preferences.putBool(epubProgressKey("eo", path).c_str(), true);
  preferences.putUShort(epubProgressKey("ec", path).c_str(), chapter);
  preferences.putUShort(epubProgressKey("eb", path).c_str(), position.block);
  preferences.putUShort(epubProgressKey("ep", path).c_str(), position.character);
  preferences.end();
}

bool CrossPointReaderApp::loadPdfManifest(const String& folder, String& title, uint16_t& pages) {
  title = displayName(folder);
  pages = 0;
  File manifest = SD.open(folder + "/manifest.txt", FILE_READ);
  if (!manifest) return false;
  bool isPaperS3Pdf = false;
  while (manifest.available()) {
    String line = manifest.readStringUntil('\n');
    line.trim();
    if (line == "PAPERS3_PDF=1") isPaperS3Pdf = true;
    else if (line.startsWith("TITLE=")) title = line.substring(6);
    else if (line.startsWith("PAGES=")) pages = static_cast<uint16_t>(line.substring(6).toInt());
  }
  manifest.close();
  return isPaperS3Pdf && pages > 0;
}

void CrossPointReaderApp::scanLibrary(AppContext& context) {
  bookCount_ = selected_ = page_ = textPage_ = 0;
  reading_ = readingPdfPages_ = readingEpub_ = false;
  epub_.close();
  textPageEnd_ = 0;
  if (!context.storage.mounted()) { status_ = "INSERT A MICROSD CARD TO USE THE LIBRARY"; return; }
  SD.mkdir("/PaperOS"); SD.mkdir("/PaperOS/books");
  SD.mkdir("/PaperOS/cache");  // EPUB metadata and image cache live under the PaperOS root.
  File folder = SD.open("/PaperOS/books");
  if (!folder || !folder.isDirectory()) { status_ = "COULD NOT OPEN /PaperOS/books"; return; }
  while (bookCount_ < MaxBooks) {
    File entry = folder.openNextFile();
    if (!entry) break;
    const String path = libraryPath(entry.name());
    if (entry.isDirectory()) {
      String title;
      uint16_t pages = 0;
      if (loadPdfManifest(path, title, pages)) {
        BookEntry& book = books_[bookCount_++];
        book = {path, title, BookFormat::PdfPages, pages, 0, false, 0};
        book.opened = loadProgress(path, book.resumeOffset);
      }
    } else {
      BookFormat format;
      bool supported = true;
      if (isEpub(path)) format = BookFormat::Epub;
      else if (isText(path)) format = BookFormat::Text;
      else if (isMarkdown(path)) format = BookFormat::Markdown;
      else supported = false;
      if (supported) {
        BookEntry& book = books_[bookCount_++];
        book = {path, displayName(path), format, 0, static_cast<uint32_t>(entry.size()), false, 0};
        book.opened = loadProgress(path, book.resumeOffset);
      }
    }
    entry.close();
  }
  folder.close();
  status_ = bookCount_ ? "TAP A BOOK TO OPEN IT" : "COPY EPUB, TXT, MD, OR PDF FOLDERS TO /PaperOS/books";
}

void CrossPointReaderApp::draw(AppContext& context) {
  if (reading_) drawReader(context); else drawLibrary(context);
}

void CrossPointReaderApp::drawLibrary(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE);
  ui::Chrome::drawHeader(context, "Library");
  const int header = ui::Chrome::headerHeight();
  const int footerTop = ui::Chrome::footerTop();
  const int top = header + 16;
  const int gap = 20;
  const int statusY = footerTop - 18;
  // Reserve a quiet status rail at the bottom-right and distribute five book
  // cards above it. Keeping status outside the card flow prevents long text
  // from colliding with the first title.
  // Keep cards compact and give each title a clear breathing gap instead of
  // filling every available pixel above the status rail.
  const int cardHeight = (statusY - top - 64 - gap * (PageSize - 1)) / PageSize;
  for (uint8_t row = 0; row < PageSize; ++row) {
    const uint8_t index = page_ * PageSize + row;
    if (index >= bookCount_) break;
    const int y = top + row * (cardHeight + gap);
    frame(18, y, 504, cardHeight);
    ui::drawIcon(ui::Icon::Book, 52, y + cardHeight / 2, 34);
    M5.Display.setFont(&fonts::FreeSans12pt7b);
    drawWrapped(books_[index].title, 78, y + 16, 400, 2, TFT_BLACK, TFT_WHITE);
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    const char* readingStatus = books_[index].opened ? "In progress" : "New";
    const String details = books_[index].format == BookFormat::PdfPages
      ? String("PDF | ") + books_[index].pages + " pages | " + readingStatus
      : regularCase(formatLabel(books_[index].format)) + " | " + fileSizeLabel(books_[index].bytes) + " | " + readingStatus;
    drawWrapped(details, 78, y + cardHeight - 28, 400, 1, TFT_BLACK, TFT_WHITE);
  }
  M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(BR_DATUM);
  M5.Display.drawString(regularCase(status_), M5.Display.width() - 20, statusY);
  M5.Display.setTextDatum(TL_DATUM);
  M5.Display.setFont(nullptr); M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  ui::ChromeOptions libraryChrome; libraryChrome.showBack = false; libraryChrome.showHome = true;
  libraryChrome.showPrevious = page_ > 0; libraryChrome.showNext = (page_ + 1) * PageSize < bookCount_;
  ui::Chrome::drawFooter(libraryChrome);
}

void CrossPointReaderApp::drawReaderFooter() {
  ui::ChromeOptions chrome;
  chrome.showBack = false;
  chrome.showHome = false;
  chrome.showClose = true;
  chrome.showPrevious = readingPdfPages_ ? pdfPage_ > 1 : (textPage_ > 0 || (readingEpub_ && epubChapter_ > 0));
  chrome.showNext = readingPdfPages_ ? pdfPage_ < books_[selected_].pages :
    (readingEpub_ ? (epubPageEnd_.block < epub_.blockCount() || epubChapter_ + 1 < epub_.chapterCount()) : textPageEnd_ > textOffsets_[textPage_]);
  ui::Chrome::drawFooter(chrome);
}

void CrossPointReaderApp::drawTextPage() {
  const int header = ui::Chrome::headerHeight();
  const int bottom = ui::Chrome::footerTop() - 28;
  const int left = readerScale_ == 2 ? 42 : 30;
  const int lineHeight = 29 * readerScale_ + (readerSpacing_ == 0 ? -3 : readerSpacing_ == 2 ? 5 : 0);
  const int width = M5.Display.width() - left * 2;
  uint32_t pageStart = textOffsets_[textPage_];
  textPageEnd_ = pageStart;
  File file = SD.open(books_[selected_].path, FILE_READ);
  if (file && pageStart > file.size()) {
    textOffsets_[textPage_] = 0;
    books_[selected_].resumeOffset = 0;
    pageStart = 0;
  }
  if (!file || !file.seek(pageStart)) {
    M5.Display.setTextSize(2); M5.Display.drawString("COULD NOT READ THIS FILE", left, header + 52);
    if (file) file.close();
    return;
  }
  selectReaderBodyFont();
  String line, word;
  uint32_t lineStart = pageStart, wordStart = pageStart;
  int y = header + 54;
  const auto commitLine = [&]() -> bool {
    if (line.isEmpty()) return true;
    if (y + lineHeight > bottom) return false;
    M5.Display.drawString(line, left, y);
    y += lineHeight;
    line = "";
    return true;
  };
  while (file.available()) {
    wordStart = file.position();
    word = "";
    int raw = file.read();
    while (raw >= 0 && raw != ' ' && raw != '\n' && raw != '\r') {
      word += static_cast<char>(raw);
      raw = file.available() ? file.read() : -1;
    }
    if (!word.isEmpty()) {
      const String candidate = line.isEmpty() ? word : line + " " + word;
      if (!line.isEmpty() && M5.Display.textWidth(candidate) > width) {
        if (!commitLine()) { textPageEnd_ = lineStart; break; }
        line = word;
        lineStart = wordStart;
      } else {
        if (line.isEmpty()) lineStart = wordStart;
        line = candidate;
      }
    }
    if (raw == '\n') {
      if (!commitLine()) { textPageEnd_ = lineStart; break; }
      lineStart = file.position();
    }
    if (raw < 0) {
      if (!commitLine()) textPageEnd_ = lineStart;
      else textPageEnd_ = file.position();
      break;
    }
    textPageEnd_ = file.position();
  }
  if (textPageEnd_ <= pageStart && file.position() > pageStart) textPageEnd_ = file.position();
  file.close();
}

void CrossPointReaderApp::drawEpubPage() {
  const int header = ui::Chrome::headerHeight();
  const int bottom = ui::Chrome::footerTop() - 20;
  EpubPosition position = epubOffsets_[textPage_];
  int y = header + 56;
  epubPageEnd_ = position;

  // This renderer consumes semantic blocks from the EPUB adapter.  It is the
  // M5Unified boundary for the upcoming CrossPoint CSS/page-cache engine:
  // parser/layout state stays independent of the device display driver.
  while (position.block < epub_.blockCount()) {
    const auto& block = epub_.block(position.block);
    if (block.kind == CrossPointEpubAdapter::BlockKind::Divider) {
      if (y + 24 > bottom) break;
      M5.Display.drawFastHLine(48, y + 12, M5.Display.width() - 96, TFT_BLACK);
      y += 24; ++position.block; position.character = 0; continue;
    }
    if (block.kind == CrossPointEpubAdapter::BlockKind::Image) {
      if (y + 190 > bottom) break;
      String imagePath, imageError;
      const bool available = epub_.materializeImage(epubChapter_, block.resource, imagePath, imageError);
      M5.Display.fillRoundRect(48, y, M5.Display.width() - 96, 174, 8, TFT_WHITE);
      M5.Display.drawRoundRect(48, y, M5.Display.width() - 96, 174, 8, TFT_BLACK);
      if (available) {
        if (imagePath.endsWith(".png")) M5.Display.drawPngFile(SD, imagePath.c_str(), 56, y + 8);
        else M5.Display.drawJpgFile(SD, imagePath.c_str(), 56, y + 8);
      } else {
        M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(MC_DATUM);
        M5.Display.drawString("IMAGE UNAVAILABLE", M5.Display.width() / 2, y + 87);
        M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
      }
      y += 190; ++position.block; position.character = 0; continue;
    }

    int left = readerScale_ == 2 ? 42 : 30;
    int width = M5.Display.width() - left * 2, lineHeight = 29, before = 10, after = 10;
    switch (block.kind) {
      case CrossPointEpubAdapter::BlockKind::Heading1:
        M5.Display.setFont(&fonts::FreeSansBold18pt7b); lineHeight = 34; before = 18; after = 14; break;
      case CrossPointEpubAdapter::BlockKind::Heading2:
        M5.Display.setFont(&fonts::FreeSansBold12pt7b); lineHeight = 29; before = 16; after = 12; break;
      case CrossPointEpubAdapter::BlockKind::Heading3:
        M5.Display.setFont(&fonts::FreeSansBold12pt7b); lineHeight = 27; before = 12; after = 9; break;
      case CrossPointEpubAdapter::BlockKind::Quote:
        selectReaderBodyFont(); left = 52; width = M5.Display.width() - 104; before = 12; after = 12; break;
      case CrossPointEpubAdapter::BlockKind::ListItem:
        selectReaderBodyFont(); left = 48; width = M5.Display.width() - 78; before = 6; after = 6; break;
      default:
        selectReaderBodyFont(); break;
    }
    M5.Display.setTextSize(readerScale_);
    lineHeight = lineHeight * readerScale_ + (readerSpacing_ == 0 ? -3 : readerSpacing_ == 2 ? 5 : 0);
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    if (position.character == 0) y += before;
    if (y + lineHeight > bottom) break;

    int cursor = position.character;
    String line = block.kind == CrossPointEpubAdapter::BlockKind::ListItem && cursor == 0 ? "- " : "";
    int lineStart = cursor;
    const auto drawStyledLine = [&](const String& rendered, int start) {
      int x = left + (block.indent && start == 0 ? 22 : 0);
      if (block.alignment == 1) x = (M5.Display.width() - M5.Display.textWidth(rendered)) / 2;
      else if (block.alignment == 2) x = left + width - M5.Display.textWidth(rendered);
      M5.Display.drawString(rendered, x, y);
    };
    while (cursor < static_cast<int>(block.text.length())) {
      while (cursor < static_cast<int>(block.text.length()) && (block.text[cursor] == ' ' || block.text[cursor] == '\n')) ++cursor;
      if (cursor >= static_cast<int>(block.text.length())) break;
      const int wordStart = cursor;
      String word;
      while (cursor < static_cast<int>(block.text.length()) && block.text[cursor] != ' ' && block.text[cursor] != '\n') word += block.text[cursor++];
      const String candidate = line.isEmpty() ? word : line + " " + word;
      if (!line.isEmpty() && M5.Display.textWidth(candidate) > width) {
        if (y + lineHeight > bottom) { epubPageEnd_ = {position.block, static_cast<uint16_t>(lineStart)}; M5.Display.setFont(nullptr); return; }
        drawStyledLine(line, lineStart); y += lineHeight;
        line = word; lineStart = wordStart;
      } else {
        if (line.isEmpty()) lineStart = wordStart;
        line = candidate;
      }
    }
    if (!line.isEmpty()) {
      if (y + lineHeight > bottom) { epubPageEnd_ = {position.block, static_cast<uint16_t>(lineStart)}; M5.Display.setFont(nullptr); return; }
      drawStyledLine(line, lineStart); y += lineHeight;
    }
    y += after;
    ++position.block; position.character = 0;
    epubPageEnd_ = position;
  }
  M5.Display.setFont(nullptr);
}

void CrossPointReaderApp::drawReader(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE);
  ui::Chrome::drawHeader(context, readingPdfPages_ ? "PDF Reader" : "Reader");
  // A dedicated, centred title rail makes the book name readable without
  // competing with e-reader body text. Tapping it opens Reader Settings.
  M5.Display.setFont(&fonts::FreeSansBold12pt7b);
  const String readerTitle = readingEpub_ ? epub_.title() : books_[selected_].title;
  String visibleTitle = readerTitle;
  while (visibleTitle.length() && M5.Display.textWidth(visibleTitle + "...") > M5.Display.width() - 48) visibleTitle.remove(visibleTitle.length() - 1);
  if (visibleTitle != readerTitle) visibleTitle += "...";
  M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString(visibleTitle, M5.Display.width() / 2, ui::Chrome::headerHeight() + 25);
  if (readingEpub_) {
    M5.Display.setFont(&fonts::FreeSans9pt7b); M5.Display.setTextDatum(MR_DATUM);
    M5.Display.drawString(String(epubProgressPercent()) + "%", M5.Display.width() - 20, ui::Chrome::headerHeight() + 25);
  }
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
  drawReaderContent();
  ui::ChromeOptions chrome;
  // Reading is a child view of Library. CLOSE returns to the list; PREV/NEXT
  // remain at the outer slots for comfortable one-handed page turns.
  chrome.showBack = false;
  chrome.showHome = false;
  chrome.showClose = true;
  chrome.showPrevious = readingPdfPages_ ? pdfPage_ > 1 : (textPage_ > 0 || (readingEpub_ && epubChapter_ > 0));
  chrome.showNext = readingPdfPages_ ? pdfPage_ < books_[selected_].pages :
    (readingEpub_ ? (epubPageEnd_.block < epub_.blockCount() || epubChapter_ + 1 < epub_.chapterCount())
                  : textPageEnd_ > textOffsets_[textPage_]);
  ui::Chrome::drawFooter(chrome);
}

void CrossPointReaderApp::drawReaderContent() {
  // The common chrome is intentionally outside this rectangle. On a page
  // turn we touch only this area, which reduces e-paper flicker and avoids
  // needlessly redrawing status, title, and navigation controls.
  const int top = ui::Chrome::headerHeight() + 54;
  const int height = ui::Chrome::footerTop() - top;
  M5.Display.fillRect(0, top, M5.Display.width(), height, TFT_WHITE);
  if (readingPdfPages_) drawPdfPage(); else if (readingEpub_) drawEpubPage(); else drawTextPage();
}

void CrossPointReaderApp::drawAppearance(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE);
  ui::Chrome::drawHeader(context, "Reader Settings", false, false);
  const int top = ui::Chrome::headerHeight() + 38;
  const int rows[] = {top, top + 122, top + 244, top + 366, top + 488};
  const char* labels[] = {"Font", "Text Size", "Line Spacing", "Chapters", "Jump Forward"};
  const String values[] = {readerFont_ == 0 ? "Serif" : "Sans", readerScale_ == 1 ? "Normal" : "Large",
                           readerSpacing_ == 0 ? "Compact" : readerSpacing_ == 1 ? "Normal" : "Relaxed", ">", String(epubProgressPercent()) + "% +10%"};
  for (int i = 0; i < 5; ++i) {
    drawAppearanceRow(rows[i], labels[i], values[i]);
  }
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
  ui::ChromeOptions chrome;
  chrome.showBack = false;
  chrome.showHome = false;
  chrome.showClose = true;
  ui::Chrome::drawFooter(chrome);
}

void CrossPointReaderApp::drawChapters(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE);
  ui::Chrome::drawHeader(context, "Chapters", false, false);
  const uint8_t first = chapterPickerPage_ * 10;
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  for (uint8_t row = 0; row < 10; ++row) {
    const uint8_t index = first + row; if (index >= epub_.tocCount()) break;
    const int y = ui::Chrome::headerHeight() + 16 + row * 66;
    frame(20, y, 500, 54);
    String title = epub_.tocEntry(index).title;
    while (title.length() && M5.Display.textWidth(title + "...") > 430) title.remove(title.length() - 1);
    if (title != epub_.tocEntry(index).title) title += "...";
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.drawString(title, M5.Display.width() / 2, y + 27);
    M5.Display.setTextDatum(TL_DATUM);
  }
  M5.Display.setFont(nullptr);
  M5.Display.fillRect(0, ui::Chrome::footerTop() - 3, M5.Display.width(), 3, TFT_BLACK);
  ui::ChromeOptions chrome; chrome.showBack = false; chrome.showHome = false; chrome.showClose = true;
  chrome.showPrevious = chapterPickerPage_ > 0;
  chrome.showNext = (chapterPickerPage_ + 1) * 10 < epub_.tocCount();
  ui::Chrome::drawFooter(chrome);
}

void CrossPointReaderApp::drawPdfPage() {
  // Reserve the complete title rail before drawing a page image. Starting at
  // +30 allowed 780px PDF PNGs to overwrite the title by a few pixels.
  const int top = ui::Chrome::headerHeight() + 54;
  const int height = ui::Chrome::footerTop() - top;
  M5.Display.fillRect(0, top, M5.Display.width(), height, TFT_WHITE);
  char filename[24];
  snprintf(filename, sizeof(filename), "/page_%03u.png", pdfPage_);
  const String pagePath = books_[selected_].path + filename;
  if (SD.exists(pagePath)) {
    // The desktop converter emits 540x780 portrait images, so the image fits
    // between shared chrome without a large runtime raster buffer.
    M5.Display.drawPngFile(SD, pagePath.c_str(), 0, top);
  } else {
    M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.setTextDatum(MC_DATUM);
    M5.Display.drawString("PAGE IMAGE IS MISSING", M5.Display.width() / 2, top + height / 2 - 16);
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.drawString(filename, M5.Display.width() / 2, top + height / 2 + 22);
    M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
  }
}

void CrossPointReaderApp::openSelected(AppContext& context) {
  if (!bookCount_) return;
  if (books_[selected_].format == BookFormat::Epub) {
    String error;
    if (!epub_.open(books_[selected_].path, error)) { Serial.printf("[EPUB] failed: %s\n", error.c_str()); status_ = error; drawLibrary(context); return; }
    reading_ = readingEpub_ = true; readingPdfPages_ = false;
    epubChapter_ = epub_.initialChapter(); textPage_ = 0; textOffsets_[0] = textPageEnd_ = 0;
    epubOffsets_[0] = {0, 0}; epubPageEnd_ = {0, 0};
    // Resume needs a complete structured location: a character offset alone
    // is meaningless after EPUB chapter layout has been rebuilt.
    uint16_t savedChapter = 0; EpubPosition savedPosition{};
    if (loadEpubProgress(books_[selected_].path, savedChapter, savedPosition) && savedChapter < epub_.chapterCount()) {
      const bool restoredChapter = savedChapter == epubChapter_ || epub_.loadChapter(savedChapter, error);
      if (restoredChapter) {
        epubChapter_ = savedChapter;
        if (savedPosition.block < epub_.blockCount()) epubOffsets_[0] = savedPosition;
      }
    }
    books_[selected_].opened = true;
    saveEpubProgress(books_[selected_].path, epubChapter_, epubOffsets_[0]);
    drawReader(context); return;
  }
  reading_ = true;
  readingPdfPages_ = books_[selected_].format == BookFormat::PdfPages;
  if (readingPdfPages_) {
    // PDF books use the same persisted numeric slot as text books, but the
    // value is a one-based rendered-page number rather than a byte offset.
    // This keeps converted PDF/PNG books at their last page after reopening.
    const uint32_t savedPage = books_[selected_].resumeOffset;
    pdfPage_ = savedPage >= 1 && savedPage <= books_[selected_].pages ? savedPage : 1;
    books_[selected_].opened = true;
    saveProgress(books_[selected_].path, pdfPage_);
    drawReader(context);
    return;
  }
  textPage_ = 0;
  textOffsets_[0] = books_[selected_].resumeOffset;
  textPageEnd_ = 0;
  books_[selected_].opened = true; saveProgress(books_[selected_].path, textOffsets_[0]);
  drawReader(context);
}

void CrossPointReaderApp::onTick(AppContext& context, uint32_t) {
  const auto& touch = M5.Touch.getDetail();
  if (!touch.wasPressed()) return;
  if (reading_) {
    if (chapterPickerOpen_) {
      ui::ChromeOptions pickerChrome; pickerChrome.showBack = false; pickerChrome.showHome = false; pickerChrome.showClose = true;
      pickerChrome.showPrevious = chapterPickerPage_ > 0; pickerChrome.showNext = (chapterPickerPage_ + 1) * 10 < epub_.tocCount();
      const auto action = ui::Chrome::hitTestFooter(touch.x, touch.y, pickerChrome);
      if (action == ui::FooterAction::Close) { chapterPickerOpen_ = false; appearanceOpen_ = true; drawAppearance(context); return; }
      if (action == ui::FooterAction::Previous && chapterPickerPage_ > 0) { --chapterPickerPage_; drawChapters(context); return; }
      if (action == ui::FooterAction::Next && (chapterPickerPage_ + 1) * 10 < epub_.tocCount()) { ++chapterPickerPage_; drawChapters(context); return; }
      const int row = (touch.y - ui::Chrome::headerHeight() - 16) / 66;
      const uint8_t index = chapterPickerPage_ * 10 + row;
      if (row >= 0 && row < 10 && index < epub_.tocCount()) { String error; if (epub_.loadChapter(epub_.tocEntry(index).chapter, error)) { epubChapter_ = epub_.tocEntry(index).chapter; resetEpubPagination(); chapterPickerOpen_ = appearanceOpen_ = false; drawReader(context); } }
      return;
    }
    if (appearanceOpen_) {
      ui::ChromeOptions appearanceChrome;
      appearanceChrome.showBack = false;
      appearanceChrome.showHome = false;
      appearanceChrome.showClose = true;
      if (ui::Chrome::hitTestFooter(touch.x, touch.y, appearanceChrome) == ui::FooterAction::Close) {
        appearanceOpen_ = false;
        drawReader(context);
        return;
      }
      const int firstRow = ui::Chrome::headerHeight() + 38;
      const int row = (touch.y - firstRow) / 122;
      if (touch.x >= 20 && touch.x <= 520 && touch.y >= firstRow && row == 3 && touch.y < firstRow + row * 122 + 94 && readingEpub_) {
        chapterPickerOpen_ = true; appearanceOpen_ = false; chapterPickerPage_ = 0; drawChapters(context); return;
      }
      if (touch.x >= 20 && touch.x <= 520 && touch.y >= firstRow && row == 4 && touch.y < firstRow + row * 122 + 94 && readingEpub_) {
        String error; const uint8_t target = epubProgressPercent() >= 90 ? 100 : epubProgressPercent() + 10;
        if (jumpToEpubPercent(target, error)) { appearanceOpen_ = false; drawReader(context); }
        return;
      }
      if (touch.x >= 20 && touch.x <= 520 && touch.y >= firstRow && row >= 0 && row < 3 && touch.y < firstRow + row * 122 + 94) {
        if (row == 0) readerFont_ = readerFont_ == 0 ? 1 : 0;
        if (row == 1) readerScale_ = readerScale_ == 1 ? 2 : 1;
        if (row == 2) readerSpacing_ = (readerSpacing_ + 1) % 3;
        saveAppearance();
        if (readingEpub_) resetEpubPagination();
        // E-paper partial refresh: only the changed option row is redrawn.
        const char* label = row == 0 ? "Font" : row == 1 ? "Text Size" : "Line Spacing";
        const String value = row == 0 ? (readerFont_ == 0 ? "Serif" : "Sans")
          : row == 1 ? (readerScale_ == 1 ? "Normal" : "Large")
          : (readerSpacing_ == 0 ? "Compact" : readerSpacing_ == 1 ? "Normal" : "Relaxed");
        drawAppearanceRow(firstRow + row * 122, label, value);
      }
      return;
    }
    // The book title is a compact, non-invasive entry point to the reader
    // appearance panel; page-turn controls remain in the footer.
    if (touch.y >= ui::Chrome::headerHeight() && touch.y < ui::Chrome::headerHeight() + 42) {
      appearanceOpen_ = true;
      drawAppearance(context);
      return;
    }
    ui::ChromeOptions chrome;
    chrome.showBack = false; chrome.showHome = false; chrome.showClose = true;
    chrome.showPrevious = readingPdfPages_ ? pdfPage_ > 1 : (textPage_ > 0 || (readingEpub_ && epubChapter_ > 0));
    chrome.showNext = readingPdfPages_ ? pdfPage_ < books_[selected_].pages :
      (readingEpub_ ? (epubPageEnd_.block < epub_.blockCount() || epubChapter_ + 1 < epub_.chapterCount())
                    : textPageEnd_ > textOffsets_[textPage_]);
    const auto action = ui::Chrome::hitTestFooter(touch.x, touch.y, chrome);
    if (action == ui::FooterAction::Close) {
      if (readingEpub_) saveEpubProgress(books_[selected_].path, epubChapter_, epubOffsets_[textPage_]);
      else if (readingPdfPages_) saveProgress(books_[selected_].path, pdfPage_);
      else saveProgress(books_[selected_].path, textOffsets_[textPage_]);
      reading_ = readingPdfPages_ = readingEpub_ = false;
      epub_.close();
      drawLibrary(context);
      return;
    }
    if (action == ui::FooterAction::Previous && readingPdfPages_ && pdfPage_ > 1) { --pdfPage_; saveProgress(books_[selected_].path, pdfPage_); drawReaderContent(); drawReaderFooter(); return; }
    if (action == ui::FooterAction::Next && readingPdfPages_ && pdfPage_ < books_[selected_].pages) { ++pdfPage_; saveProgress(books_[selected_].path, pdfPage_); drawReaderContent(); drawReaderFooter(); return; }
    if (action == ui::FooterAction::Previous && readingEpub_ && textPage_ == 0 && epubChapter_ > 0) {
      String error; if (epub_.loadChapter(--epubChapter_, error)) { textPage_ = 0; epubOffsets_[0] = {0, 0}; epubPageEnd_ = {0, 0}; saveEpubProgress(books_[selected_].path, epubChapter_, epubOffsets_[0]); drawReaderContent(); drawReaderFooter(); } return;
    }
    if (action == ui::FooterAction::Previous && !readingPdfPages_ && textPage_ > 0) { --textPage_; drawReaderContent(); drawReaderFooter(); return; }
    if (action == ui::FooterAction::Next && readingEpub_ && epubPageEnd_.block >= epub_.blockCount() && epubChapter_ + 1 < epub_.chapterCount()) {
      String error; if (epub_.loadChapter(++epubChapter_, error)) { textPage_ = 0; epubOffsets_[0] = {0, 0}; epubPageEnd_ = {0, 0}; saveEpubProgress(books_[selected_].path, epubChapter_, epubOffsets_[0]); drawReaderContent(); drawReaderFooter(); } return;
    }
    if (action == ui::FooterAction::Next && readingEpub_ && epubPageEnd_.block < epub_.blockCount() && textPage_ + 1 < MaxTextPages) {
      epubOffsets_[++textPage_] = epubPageEnd_;
      saveEpubProgress(books_[selected_].path, epubChapter_, epubOffsets_[textPage_]);
      drawReaderContent(); drawReaderFooter();
      return;
    }
    if (action == ui::FooterAction::Next && !readingPdfPages_ && !readingEpub_ && textPageEnd_ > textOffsets_[textPage_] && textPage_ + 1 < MaxTextPages) {
      textOffsets_[++textPage_] = textPageEnd_;
      saveProgress(books_[selected_].path, textOffsets_[textPage_]);
      drawReaderContent(); drawReaderFooter();
    }
    return;
  }
  ui::ChromeOptions chrome;
  chrome.showBack = false; chrome.showHome = true;
  chrome.showPrevious = page_ > 0; chrome.showNext = (page_ + 1) * PageSize < bookCount_;
  const auto action = ui::Chrome::hitTestFooter(touch.x, touch.y, chrome);
  if (action == ui::FooterAction::Home) { if (navigator_) navigator_(AppId::Launcher); return; }
  if (action == ui::FooterAction::Previous && page_ > 0) { --page_; drawLibrary(context); return; }
  if (action == ui::FooterAction::Next && (page_ + 1) * PageSize < bookCount_) { ++page_; drawLibrary(context); return; }
  const int top = ui::Chrome::headerHeight() + 70;
  const int gap = 10;
  const int cardHeight = (ui::Chrome::footerTop() - top - 14 - gap * (PageSize - 1)) / PageSize;
  if (touch.y >= top && touch.y < top + PageSize * (cardHeight + gap)) {
    const uint8_t row = (touch.y - top) / (cardHeight + gap);
    const uint8_t candidate = page_ * PageSize + row;
    if (candidate < bookCount_ && touch.y < top + row * (cardHeight + gap) + cardHeight) {
      selected_ = candidate;
      openSelected(context);
    }
  }
}
