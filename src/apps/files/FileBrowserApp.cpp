#include "apps/files/FileBrowserApp.h"

#include <SD.h>
#include <M5Unified.h>
#include "app/AppContext.h"
#include "services/Services.h"
#include "ui/Chrome.h"
#include "ui/Icons.h"
#include "ui/Popup.h"
#include "ui/Theme.h"

namespace {
constexpr int kRowHeight = 100;
constexpr int kRowBoxHeight = 84;
constexpr int kActionMargin = 14;
constexpr int kActionGap = 8;

int infoTop() { return ui::Chrome::headerHeight() + 12; }
int rowTop() { return ui::Chrome::headerHeight() + 56; }
// Keep file actions just above the persistent footer, regardless of rotation.
int commandTop() { return ui::Chrome::footerTop() - 54; }
int actionButtonWidth() { return (M5.Display.width() - kActionMargin * 2 - kActionGap * 4) / 5; }
int actionButtonX(int index) { return kActionMargin + index * (actionButtonWidth() + kActionGap); }
int deleteDialogTop() { return (M5.Display.height() - 208) / 2; }

void drawFramedBox(int x, int y, int width, int height, int radius = 10) {
  ui::Theme::drawFrame(x, y, width, height, radius);
}

String trimTextToWidth(const String& value, int maxWidth) {
  if (M5.Display.textWidth(value) <= maxWidth) return value;

  const String suffix = "...";
  String trimmed = value;
  while (!trimmed.isEmpty() &&
         M5.Display.textWidth(trimmed + suffix) > maxWidth) {
    trimmed.remove(trimmed.length() - 1);
  }
  return trimmed + suffix;
}

bool pathIsInside(const String& candidate, const String& parent) {
  return candidate == parent || candidate.startsWith(parent + "/");
}

// Arduino SD returns different names from openNextFile() depending on the
// driver build: a complete absolute path, a path relative to the directory,
// or a slash-prefixed filename. Resolve all three forms against the folder
// actually being scanned so nested folders never fall back to the SD root.
String entryPath(const String& folder, const String& entryName) {
  if (entryName.startsWith(folder + "/") || entryName == folder) return entryName;
  if (entryName.startsWith("/PaperOS/")) return entryName;
  String relative = entryName;
  while (relative.startsWith("/")) relative.remove(0, 1);
  return folder + "/" + relative;
}
}

bool FileBrowserApp::onStart(AppContext& context) { currentPath_ = "/PaperOS"; moveMode_ = false; selectionMode_ = false; deleteConfirmationVisible_ = false; scan(context); draw(context); return true; }

String FileBrowserApp::nameOf(const String& path) const {
  if (path == "/") return "/";
  const int slash = path.lastIndexOf('/');
  return slash < 0 ? path : path.substring(slash + 1);
}

String FileBrowserApp::joinPath(const String& folder, const String& name) const {
  return folder == "/" ? "/" + name : folder + "/" + name;
}

String FileBrowserApp::parentPath() const {
  if (currentPath_ == "/") return "/";
  const int slash = currentPath_.lastIndexOf('/');
  return slash <= 0 ? "/" : currentPath_.substring(0, slash);
}

void FileBrowserApp::scan(AppContext& context) {
  entryCount_ = 0; page_ = 0; selected_ = -1; selectionMode_ = false; deleteConfirmationVisible_ = false;
  if (!context.storage.mounted()) { status_ = "INSERT A MICROSD CARD"; return; }
  File folder = SD.open(currentPath_);
  if (!folder || !folder.isDirectory()) { status_ = "CANNOT OPEN " + currentPath_; return; }
  while (entryCount_ < MaxEntries) {
    File entry = folder.openNextFile();
    if (!entry) break;
    Entry& target = entries_[entryCount_++];
    target.path = entryPath(currentPath_, String(entry.name())); target.directory = entry.isDirectory(); target.size = entry.size();
    Serial.printf("[FILES] %s%s\n", target.directory ? "DIR  " : "FILE ", target.path.c_str());
    entry.close();
  }
  folder.close();
  status_ = entryCount_ ? String(entryCount_) + " ITEMS" : "THIS FOLDER IS EMPTY";
}

ui::ChromeOptions FileBrowserApp::chromeOptions() const {
  ui::ChromeOptions options;
  // BACK is contextual: it appears only after the user enters a folder and
  // returns to its parent. HOME always exits Files to the launcher.
  options.showBack = currentPath_ != "/PaperOS";
  options.showHome = true;
  options.showPrevious = page_ > 0;
  options.showNext = (page_ + 1) * PageSize < entryCount_;
  return options;
}

void FileBrowserApp::drawCommands() {
  const ui::Icon icons[] = {
      moveMode_ ? ui::Icon::Close : ui::Icon::Select,
      moveMode_ ? ui::Icon::Check : ui::Icon::FolderAdd,
      moveMode_ ? ui::Icon::Refresh : ui::Icon::Delete,
      moveMode_ ? ui::Icon::Refresh : ui::Icon::Move,
      ui::Icon::Refresh};
  for (int i = 0; i < 5; ++i) {
    const int x = actionButtonX(i);
    const int width = actionButtonWidth();
    // These are deliberately borderless icon actions. The fixed five-column
    // hit areas remain unchanged, but removing tiny outlines gives the icons
    // more visual weight and avoids crowded e-paper edges.
    ui::drawIcon(icons[i], x + width / 2, commandTop() + 21, 36);
  }
  const uint8_t first = entryCount_ ? page_ * PageSize + 1 : 0;
  const uint8_t last = entryCount_ ? min<uint8_t>(entryCount_, (page_ + 1) * PageSize) : 0;
  const String pagination = String(first) + "-" + last + " OF " + entryCount_ + " ITEMS";

  M5.Display.setTextSize(1); M5.Display.setCursor(20, infoTop());
  const String folderLine = moveMode_ ? "MOVE TARGET: " + currentPath_ : "FOLDER: " + currentPath_;
  M5.Display.print(trimTextToWidth(folderLine, 500));
  M5.Display.setCursor(20, infoTop() + 22);
  const String statusLine = moveMode_
      ? pagination + "  |  CHOOSE A FOLDER OR TAP MOVE HERE"
      : pagination + "  |  " + status_;
  M5.Display.print(trimTextToWidth(statusLine, 500));
}

void FileBrowserApp::drawRows() {
  for (uint8_t row = 0; row < PageSize; ++row) {
    const uint8_t index = page_ * PageSize + row;
    if (index >= entryCount_) break;
    const int y = rowTop() + row * kRowHeight;
    const bool selected = !moveMode_ && selectionMode_ && index == selected_;
    if (selected) M5.Display.fillRoundRect(18, y, 504, kRowBoxHeight, 10, TFT_BLACK);
    else drawFramedBox(18, y, 504, kRowBoxHeight);
    const uint16_t foreground = selected ? TFT_WHITE : TFT_BLACK;
    const uint16_t background = selected ? TFT_BLACK : TFT_WHITE;
    M5.Display.setTextColor(foreground, background);
    if (selected) M5.Display.fillCircle(52, y + 42, 15, TFT_WHITE);
    else ui::drawIcon(entries_[index].directory ? ui::Icon::Folder : ui::Icon::Note, 52, y + 42, 32);

    M5.Display.setTextSize(2);
    M5.Display.setCursor(86, y + 12);
    const int textWidth = entries_[index].directory ? 372 : 410;
    M5.Display.print(trimTextToWidth(nameOf(entries_[index].path), textWidth));
    M5.Display.setTextSize(1);
    M5.Display.setCursor(86, y + 52);
    const String detail = entries_[index].directory
        ? (selectionMode_ ? "FOLDER  |  SELECTED" : "FOLDER  |  TAP TO OPEN")
        : String(entries_[index].size) + (selectionMode_ ? " BYTES  |  SELECTED" : " BYTES  |  USE SELECT TO ACT");
    M5.Display.print(trimTextToWidth(detail, textWidth));
    if (entries_[index].directory) ui::drawIcon(ui::Icon::ArrowRight, 490, y + 42, ui::Theme::NextIconSize);
  }
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
}

void FileBrowserApp::draw(AppContext& context) {
  M5.Display.clear();
  ui::Chrome::drawHeader(context, moveMode_ ? "Move selected item" : "Files");
  drawCommands(); drawRows();
  ui::Chrome::drawFooter(chromeOptions());
  if (deleteConfirmationVisible_) drawDeleteConfirmation();
}

void FileBrowserApp::drawDeleteConfirmation() {
  const int width = M5.Display.width() - 76;
  const int x = 38;
  const int y = deleteDialogTop();
  ui::Popup::drawFrame(x, y, width, 208, "DELETE ITEM?");
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setTextSize(1); M5.Display.setCursor(x + 28, y + 86);
  M5.Display.print(trimTextToWidth(nameOf(entries_[selected_].path), width - 56));
  M5.Display.setCursor(x + 28, y + 96);
  M5.Display.print("THIS ACTION CANNOT BE UNDONE.");

  const int buttonWidth = (width - 76) / 2;
  const int buttonY = y + 140;
  drawFramedBox(x + 24, buttonY, buttonWidth, 46, 6);
  drawFramedBox(x + 52 + buttonWidth, buttonY, buttonWidth, 46, 6);
  M5.Display.setTextSize(1);
  M5.Display.drawString("CANCEL", x + 24 + (buttonWidth - M5.Display.textWidth("CANCEL")) / 2, buttonY + 16);
  M5.Display.drawString("DELETE", x + 52 + buttonWidth + (buttonWidth - M5.Display.textWidth("DELETE")) / 2, buttonY + 16);
}

void FileBrowserApp::openDirectory(AppContext& context, const String& path) { currentPath_ = path; scan(context); draw(context); }

void FileBrowserApp::selectOrOpen(AppContext& context, uint8_t index) {
  if (index >= entryCount_) return;
  const Entry& entry = entries_[index];
  if (moveMode_) {
    if (entry.directory) openDirectory(context, entry.path);
    else { status_ = "CHOOSE A FOLDER AS THE DESTINATION"; draw(context); }
    return;
  }
  if (!selectionMode_) {
    if (entry.directory) openDirectory(context, entry.path);
    else { status_ = "USE SELECT TO CHOOSE A FILE"; draw(context); }
    return;
  }
  selected_ = index;
  status_ = "SELECTED. USE DELETE OR MOVE.";
  draw(context);
}

void FileBrowserApp::toggleSelection(AppContext& context) {
  if (selectionMode_) {
    selectionMode_ = false;
    selected_ = -1;
    status_ = "SELECTION CLEARED";
  } else {
    selectionMode_ = true;
    selected_ = -1;
    status_ = "SELECT MODE: TAP AN ITEM";
  }
  draw(context);
}

void FileBrowserApp::createFolder(AppContext& context) {
  if (!context.storage.mounted()) { status_ = "SD CARD IS NOT MOUNTED"; draw(context); return; }
  for (uint8_t i = 1; i < 100; ++i) {
    const String path = joinPath(currentPath_, String("New Folder ") + i);
    if (!SD.exists(path) && SD.mkdir(path)) { status_ = "CREATED " + nameOf(path); scan(context); draw(context); return; }
  }
  status_ = "COULD NOT CREATE A NEW FOLDER"; draw(context);
}

void FileBrowserApp::deleteSelected(AppContext& context) {
  if (selected_ < 0) { status_ = "SELECT AN ITEM FIRST"; draw(context); return; }
  deleteConfirmationVisible_ = true;
  draw(context);
}

void FileBrowserApp::confirmDelete(AppContext& context) {
  if (selected_ < 0) return;
  const Entry& entry = entries_[selected_];
  const bool removed = entry.directory ? SD.rmdir(entry.path) : SD.remove(entry.path);
  status_ = removed ? "DELETED " + nameOf(entry.path) : (entry.directory ? "FOLDER MUST BE EMPTY TO DELETE" : "DELETE FAILED");
  scan(context); draw(context);
}

void FileBrowserApp::beginMove(AppContext& context) {
  if (selected_ < 0) { status_ = "SELECT AN ITEM FIRST"; draw(context); return; }
  moveSource_ = entries_[selected_].path; moveMode_ = true; selectionMode_ = false; selected_ = -1;
  status_ = "SELECT A DESTINATION FOLDER"; draw(context);
}

void FileBrowserApp::moveHere(AppContext& context) {
  if (moveSource_.isEmpty()) return;
  const String destination = joinPath(currentPath_, nameOf(moveSource_));
  if (destination == moveSource_) { status_ = "ITEM IS ALREADY IN THIS FOLDER"; draw(context); return; }
  if (SD.exists(destination)) { status_ = "DESTINATION ALREADY HAS THIS NAME"; draw(context); return; }
  File source = SD.open(moveSource_);
  const bool sourceIsDirectory = source && source.isDirectory();
  if (source) source.close();
  if (sourceIsDirectory && pathIsInside(currentPath_, moveSource_)) { status_ = "CANNOT MOVE A FOLDER INTO ITSELF"; draw(context); return; }
  const bool moved = SD.rename(moveSource_, destination);
  status_ = moved ? "MOVED TO " + currentPath_ : "MOVE FAILED";
  moveMode_ = false; moveSource_ = ""; scan(context); draw(context);
}

void FileBrowserApp::goBack(AppContext& context) {
  if (moveMode_) { moveMode_ = false; moveSource_ = ""; scan(context); draw(context); return; }
  if (currentPath_ != "/") { openDirectory(context, parentPath()); return; }
  if (navigator_) navigator_(AppId::Launcher);
}

void FileBrowserApp::onTick(AppContext& context, uint32_t) {
  const auto& touch = M5.Touch.getDetail();
  if (!touch.wasPressed()) return;
  if (deleteConfirmationVisible_) {
    if (ui::Popup::hitClose(touch.x, touch.y, 38, deleteDialogTop(), M5.Display.width() - 76)) {
      deleteConfirmationVisible_ = false; status_ = "DELETE CANCELLED"; draw(context); return;
    }
    const int y = deleteDialogTop() + 140;
    const int width = M5.Display.width() - 76;
    const int buttonWidth = (width - 76) / 2;
    if (touch.y >= y && touch.y < y + 46) {
      if (touch.x >= 62 && touch.x < 62 + buttonWidth) {
        deleteConfirmationVisible_ = false; status_ = "DELETE CANCELLED"; draw(context);
      } else if (touch.x >= 90 + buttonWidth && touch.x < 90 + buttonWidth * 2) {
        deleteConfirmationVisible_ = false; confirmDelete(context);
      }
    }
    return;
  }
  const auto footer = ui::Chrome::hitTestFooter(touch.x, touch.y, chromeOptions());
  if (footer == ui::FooterAction::Back) { goBack(context); return; }
  if (footer == ui::FooterAction::Home) { if (navigator_) navigator_(AppId::Launcher); return; }
  if (footer == ui::FooterAction::Previous) { --page_; draw(context); return; }
  if (footer == ui::FooterAction::Next) { ++page_; draw(context); return; }
  if (touch.x >= kActionMargin && touch.x < M5.Display.width() - kActionMargin && touch.y >= commandTop() && touch.y <= commandTop() + 42) {
    const int command = (touch.x - kActionMargin) / (actionButtonWidth() + kActionGap);
    if (command < 0 || command > 4 || touch.x >= actionButtonX(command) + actionButtonWidth()) return;
    if (moveMode_) {
      if (command == 0) { moveMode_ = false; moveSource_ = ""; scan(context); draw(context); }
      else if (command == 1) moveHere(context);
      else if (command >= 2) { scan(context); draw(context); }
    } else {
      if (command == 0) toggleSelection(context);
      else if (command == 1) createFolder(context);
      else if (command == 2) deleteSelected(context);
      else if (command == 3) beginMove(context);
      else { scan(context); draw(context); }
    }
    return;
  }
  if (touch.y >= rowTop() && touch.y < rowTop() + PageSize * kRowHeight) {
    const uint8_t index = page_ * PageSize + (touch.y - rowTop()) / kRowHeight;
    selectOrOpen(context, index);
  }
}
