#include "apps/minesweeper/MinesweeperApp.h"

#include <M5Unified.h>
#include <esp_system.h>
#include "app/AppContext.h"
#include "ui/Chrome.h"
#include "ui/Popup.h"
#include "ui/Theme.h"
#include "ui/Typography.h"

namespace {
ui::ChromeOptions gameChrome() { ui::ChromeOptions options; options.showHome = true; return options; }

void gameButton(int x, int y, int width, int height, const char* label, bool selected = false) {
  ui::Theme::drawButtonFrame(x, y, width, height, selected);
  ui::Typography::apply(ui::TextRole::BodyBold, 0.90f);
  M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString(label, x + width / 2, y + height / 2 + 1);
  M5.Display.setTextDatum(TL_DATUM); ui::Typography::reset();
}
}

bool MinesweeperApp::onStart(AppContext& context) {
  reset();
  randomSeed(esp_random());
  draw(context);
  return true;
}

void MinesweeperApp::reset() {
  memset(mines_, 0, sizeof(mines_));
  memset(revealed_, 0, sizeof(revealed_));
  memset(flagged_, 0, sizeof(flagged_));
  memset(around_, 0, sizeof(around_));
  flags_ = 0; revealedSafe_ = 0; minesPlaced_ = false; flagMode_ = false;
  resetConfirmation_ = false; state_ = State::Ready;
}

void MinesweeperApp::placeMines(uint8_t safeRow, uint8_t safeColumn) {
  uint8_t placed = 0;
  while (placed < MineCount) {
    const uint8_t row = random(Rows), column = random(Columns);
    // Keep the opening cell and all immediate neighbours mine-free so every
    // first reveal has a useful opening rather than an arbitrary loss.
    if (mines_[row][column] || (abs(static_cast<int>(row) - safeRow) <= 1 && abs(static_cast<int>(column) - safeColumn) <= 1)) continue;
    mines_[row][column] = true; ++placed;
  }
  for (uint8_t row = 0; row < Rows; ++row)
    for (uint8_t column = 0; column < Columns; ++column) around_[row][column] = countAround(row, column);
  minesPlaced_ = true; state_ = State::Playing;
}

uint8_t MinesweeperApp::countAround(uint8_t row, uint8_t column) const {
  uint8_t count = 0;
  for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
    if (!dx && !dy) continue;
    const int y = row + dy, x = column + dx;
    if (y >= 0 && y < Rows && x >= 0 && x < Columns && mines_[y][x]) ++count;
  }
  return count;
}

void MinesweeperApp::reveal(uint8_t startRow, uint8_t startColumn) {
  if (flagged_[startRow][startColumn] || revealed_[startRow][startColumn] || state_ == State::Lost || state_ == State::Won) return;
  if (!minesPlaced_) placeMines(startRow, startColumn);
  if (mines_[startRow][startColumn]) { revealed_[startRow][startColumn] = true; state_ = State::Lost; revealAllMines(); return; }

  uint8_t queueRows[Rows * Columns], queueColumns[Rows * Columns];
  uint8_t read = 0, write = 0;
  queueRows[write] = startRow; queueColumns[write++] = startColumn;
  while (read < write) {
    const uint8_t row = queueRows[read], column = queueColumns[read++];
    if (revealed_[row][column] || flagged_[row][column] || mines_[row][column]) continue;
    revealed_[row][column] = true; ++revealedSafe_;
    if (around_[row][column] != 0) continue;
    for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
      const int y = row + dy, x = column + dx;
      if (y >= 0 && y < Rows && x >= 0 && x < Columns && !revealed_[y][x] && !flagged_[y][x] && write < Rows * Columns) {
        queueRows[write] = y; queueColumns[write++] = x;
      }
    }
  }
  if (revealedSafe_ == Rows * Columns - MineCount) state_ = State::Won;
}

void MinesweeperApp::revealAllMines() {
  for (uint8_t row = 0; row < Rows; ++row) for (uint8_t column = 0; column < Columns; ++column)
    if (mines_[row][column]) revealed_[row][column] = true;
}

void MinesweeperApp::drawCell(uint8_t row, uint8_t column) {
  const int x = Left + column * Cell, y = Top + row * Cell;
  M5.Display.fillRect(x, y, Cell, Cell, TFT_BLACK);
  const int inset = ui::Theme::Border;
  M5.Display.fillRect(x + inset, y + inset, Cell - inset * 2, Cell - inset * 2, TFT_WHITE);
  if (!revealed_[row][column]) {
    if (flagged_[row][column]) {
      M5.Display.fillRect(x + 22, y + 12, 4, 26, TFT_BLACK);
      M5.Display.fillTriangle(x + 26, y + 12, x + 39, y + 18, x + 26, y + 24, TFT_BLACK);
    }
    return;
  }
  if (mines_[row][column]) {
    M5.Display.fillCircle(x + Cell / 2, y + Cell / 2, 10, TFT_BLACK);
    for (int i = -1; i <= 1; ++i) { M5.Display.drawFastHLine(x + 13, y + Cell / 2 + i, 24, TFT_BLACK); M5.Display.drawFastVLine(x + Cell / 2 + i, y + 13, 24, TFT_BLACK); }
    return;
  }
  const uint8_t count = around_[row][column];
  if (count) {
    ui::Typography::apply(ui::TextRole::BodyBold, 1.05f); M5.Display.setTextDatum(MC_DATUM);
    M5.Display.drawString(String(count), x + Cell / 2, y + Cell / 2 + 1);
    M5.Display.setTextDatum(TL_DATUM); ui::Typography::reset();
  }
}

void MinesweeperApp::drawBoard() {
  ui::Theme::drawFrame(Left - ui::Theme::Border, Top - ui::Theme::Border, Columns * Cell + ui::Theme::Border * 2, Rows * Cell + ui::Theme::Border * 2, 4);
  for (uint8_t row = 0; row < Rows; ++row) for (uint8_t column = 0; column < Columns; ++column) drawCell(row, column);
}

void MinesweeperApp::drawStatus() {
  M5.Display.fillRect(20, 84, 500, 64, TFT_WHITE);
  ui::Typography::apply(ui::TextRole::BodyBold, 0.90f); M5.Display.setTextDatum(ML_DATUM);
  M5.Display.drawString(String("MINES ") + MineCount, 32, 110);
  M5.Display.drawString(String("FLAGS ") + flags_, 208, 110);
  const char* message = state_ == State::Ready ? "Tap a cell to start" : state_ == State::Won ? "Board cleared" : state_ == State::Lost ? "Mine hit" : "Clear the board";
  ui::Typography::apply(ui::TextRole::Caption); M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString(message, 270, 138);
  M5.Display.setTextDatum(TL_DATUM); ui::Typography::reset();
}

void MinesweeperApp::drawControls() {
  const int y = 682;
  M5.Display.fillRect(20, y - 8, 500, 70, TFT_WHITE);
  gameButton(32, y, 142, 54, "RESET");
  gameButton(194, y, 142, 54, flagMode_ ? "FLAG ON" : "FLAG");
  gameButton(356, y, 142, 54, "REVEAL", !flagMode_);
}

void MinesweeperApp::draw(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE);
  ui::Chrome::drawHeader(context, "Minesweeper");
  ui::Chrome::drawFooter(gameChrome());
  drawStatus(); drawBoard(); drawControls();
}

void MinesweeperApp::refreshCell(uint8_t row, uint8_t column) {
  drawCell(row, column);
  const int x = Left + column * Cell, y = Top + row * Cell;
  M5.Display.display(x, y, Cell, Cell);
}

void MinesweeperApp::refreshStatus() {
  drawStatus(); drawControls();
  // Keep the static grid and shared chrome untouched when only a flag count
  // or the active mode changed.
  M5.Display.display(20, 84, 500, 64);
  M5.Display.display(20, 674, 500, 70);
}

bool MinesweeperApp::inBoard(int x, int y) const { return x >= Left && x < Left + Columns * Cell && y >= Top && y < Top + Rows * Cell; }

void MinesweeperApp::drawResetConfirmation() {
  M5.Display.fillScreen(TFT_WHITE);
  ui::Popup::drawFrame(54, 330, 432, 270, "NEW GAME");
  ui::Typography::apply(ui::TextRole::BodyBold, 0.95f); M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString("Start a new Minesweeper board?", 270, 452);
  gameButton(94, 500, 154, 56, "NO"); gameButton(292, 500, 154, 56, "YES");
  M5.Display.setTextDatum(TL_DATUM); ui::Typography::reset();
}

void MinesweeperApp::onTick(AppContext& context, uint32_t) {
  const auto& touch = M5.Touch.getDetail();
  if (!touch.wasPressed()) return;
  if (resetConfirmation_) {
    if (ui::Popup::hitClose(touch.x, touch.y, 54, 330, 432) || (touch.x >= 94 && touch.x <= 248 && touch.y >= 500 && touch.y <= 556)) { resetConfirmation_ = false; draw(context); return; }
    if (touch.x >= 292 && touch.x <= 446 && touch.y >= 500 && touch.y <= 556) { reset(); draw(context); return; }
    return;
  }
  if (ui::Chrome::hitTestFooter(touch.x, touch.y, gameChrome()) == ui::FooterAction::Home) { if (navigator_) navigator_(AppId::Launcher); return; }
  if (touch.y >= 682 && touch.y <= 736) {
    if (touch.x < 174) { resetConfirmation_ = true; drawResetConfirmation(); return; }
    if (touch.x < 336) { flagMode_ = true; drawControls(); M5.Display.display(20, 674, 500, 70); return; }
    flagMode_ = false; drawControls(); M5.Display.display(20, 674, 500, 70); return;
  }
  if (!inBoard(touch.x, touch.y) || state_ == State::Won || state_ == State::Lost) return;
  const uint8_t row = (touch.y - Top) / Cell, column = (touch.x - Left) / Cell;
  if (flagMode_) {
    if (!revealed_[row][column]) { flagged_[row][column] = !flagged_[row][column]; flags_ += flagged_[row][column] ? 1 : -1; refreshCell(row, column); refreshStatus(); }
    return;
  }
  reveal(row, column);
  // A flood reveal can alter many cells. Redrawing the board region once is
  // faster and less visually noisy than issuing a full-screen update.
  drawBoard(); drawStatus();
  M5.Display.display(20, 84, 500, 64);
  M5.Display.display(Left - 3, Top - 3, Columns * Cell + 6, Rows * Cell + 6);
}
