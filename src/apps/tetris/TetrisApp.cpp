#include "apps/tetris/TetrisApp.h"

#include <M5Unified.h>
#include <Preferences.h>
#include "app/AppContext.h"
#include "ui/Chrome.h"
#include "ui/Typography.h"
#include "ui/Theme.h"

namespace {
// Four 4x4 masks per tetromino.  A bit represents one filled block.
constexpr uint16_t kPieces[7][4] = {
  {0x0F00, 0x2222, 0x0F00, 0x2222}, // I
  {0x0660, 0x0660, 0x0660, 0x0660}, // O
  {0x0E40, 0x4C40, 0x4E00, 0x4640}, // T
  {0x06C0, 0x8C40, 0x06C0, 0x8C40}, // S
  {0x0C60, 0x4C80, 0x0C60, 0x4C80}, // Z
  {0x08E0, 0x6440, 0x0E20, 0x44C0}, // J
  {0x02E0, 0x4460, 0x0E80, 0xC440}  // L
};

bool filled(int piece, int rotation, int row, int col) {
  return (kPieces[piece][rotation & 3] & (0x8000u >> (row * 4 + col))) != 0;
}
ui::ChromeOptions gameChrome() { ui::ChromeOptions options; options.showHome = true; return options; }
void gameButton(int x, int y, int width, int height, const char* label) {
  ui::Theme::drawButtonFrame(x, y, width, height);
  ui::Typography::apply(ui::TextRole::BodyBold, 0.82f); M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString(label, x + width / 2, y + height / 2 + 1);
  M5.Display.setTextDatum(TL_DATUM); ui::Typography::reset();
}
}

bool TetrisApp::onStart(AppContext& context) {
  loadHighScore();
  reset();
  M5.Speaker.setVolume(120);
  lastDropMs_ = millis();
  draw(context);
  return true;
}

void TetrisApp::loadHighScore() { Preferences preferences; preferences.begin("paperos", true); highScore_ = preferences.getUShort("tetrisHigh", 0); preferences.end(); }
void TetrisApp::saveHighScore() const { Preferences preferences; preferences.begin("paperos", false); preferences.putUShort("tetrisHigh", highScore_); preferences.end(); }

void TetrisApp::reset() {
  memset(board_, 0, sizeof(board_));
  memset(renderedBoard_, 0, sizeof(renderedBoard_));
  score_ = 0; gameOver_ = false; piece_ = 0; nextPiece_ = 0;
  spawn();
}

void TetrisApp::spawn() {
  piece_ = nextPiece_;
  nextPiece_ = (nextPiece_ + 1) % 7;
  rotation_ = 0; pieceX_ = 3; pieceY_ = 0;
  if (collides(piece_, rotation_, pieceX_, pieceY_)) gameOver_ = true;
}

bool TetrisApp::collides(int piece, int rotation, int originX, int originY) const {
  for (int row = 0; row < 4; ++row) for (int col = 0; col < 4; ++col) {
    if (!filled(piece, rotation, row, col)) continue;
    const int x = originX + col, y = originY + row;
    if (x < 0 || x >= Columns || y >= Rows || (y >= 0 && board_[y][x])) return true;
  }
  return false;
}

void TetrisApp::rotate() {
  const int next = (rotation_ + 1) & 3;
  if (!collides(piece_, next, pieceX_, pieceY_)) rotation_ = next;
}

void TetrisApp::move(int dx) {
  if (!collides(piece_, rotation_, pieceX_ + dx, pieceY_)) pieceX_ += dx;
}

bool TetrisApp::drop() {
  if (!collides(piece_, rotation_, pieceX_, pieceY_ + 1)) { ++pieceY_; return false; }
  lockPiece();
  const uint8_t cleared = clearLines();
  if (score_ > highScore_) { highScore_ = score_; saveHighScore(); }
  spawn();
  if (gameOver_) playTone(160, 280);
  else if (cleared) playTone(980, 140);
  else playTone(440, 45);
  return true;
}

void TetrisApp::lockPiece() {
  for (int row = 0; row < 4; ++row) for (int col = 0; col < 4; ++col) {
    if (!filled(piece_, rotation_, row, col)) continue;
    const int x = pieceX_ + col, y = pieceY_ + row;
    if (y >= 0 && y < Rows && x >= 0 && x < Columns) board_[y][x] = true;
  }
}

uint8_t TetrisApp::clearLines() {
  int removed = 0;
  for (int row = Rows - 1; row >= 0; --row) {
    bool full = true;
    for (int col = 0; col < Columns; ++col) if (!board_[row][col]) { full = false; break; }
    if (!full) continue;
    ++removed;
    for (int y = row; y > 0; --y) for (int col = 0; col < Columns; ++col) board_[y][col] = board_[y - 1][col];
    for (int col = 0; col < Columns; ++col) board_[0][col] = false;
    ++row;
  }
  if (removed) score_ += static_cast<uint16_t>(removed * removed * 100);
  return static_cast<uint8_t>(removed);
}

void TetrisApp::drawCell(int column, int row, bool filled) {
  if (column < 0 || column >= Columns || row < 0 || row >= Rows) return;
  const int x = Left + column * Cell + 3, y = Top + row * Cell + 3;
  M5.Display.fillRect(x, y, Cell - 6, Cell - 6, filled ? TFT_BLACK : TFT_WHITE);
}

void TetrisApp::drawPiece(int piece, int rotation, int originX, int originY, bool filledState) {
  for (int row = 0; row < 4; ++row) for (int col = 0; col < 4; ++col)
    if (filled(piece, rotation, row, col)) drawCell(originX + col, originY + row, filledState);
}

void TetrisApp::captureRenderedState() {
  memcpy(renderedBoard_, board_, sizeof(board_));
  previousPiece_ = piece_; previousRotation_ = rotation_;
  previousX_ = pieceX_; previousY_ = pieceY_;
}

void TetrisApp::drawInitialBoard() {
  M5.Display.fillRect(Left - 3, Top - 3, Columns * Cell + 6, Rows * Cell + 6, TFT_BLACK);
  M5.Display.fillRect(Left, Top, Columns * Cell, Rows * Cell, TFT_WHITE);
  for (int row = 0; row < Rows; ++row) for (int col = 0; col < Columns; ++col) {
    if (board_[row][col]) drawCell(col, row, true);
  }
  if (!gameOver_) drawPiece(piece_, rotation_, pieceX_, pieceY_, true);
  captureRenderedState();
}

void TetrisApp::drawBoardDelta() {
  // Only cells touched by the former/current tetromino and static cells whose
  // value changed are painted. This avoids a full-board e-paper refresh.
  M5.Display.startWrite();
  drawPiece(previousPiece_, previousRotation_, previousX_, previousY_, false);
  for (int row = 0; row < Rows; ++row) for (int col = 0; col < Columns; ++col)
    if (renderedBoard_[row][col] != board_[row][col]) drawCell(col, row, board_[row][col]);
  if (!gameOver_) drawPiece(piece_, rotation_, pieceX_, pieceY_, true);
  M5.Display.endWrite();
  captureRenderedState();
  if (gameOver_) drawGameOver();
}

void TetrisApp::drawStatus() {
  // The score/status strip is separate from the controls so it can change
  // without causing the static button outlines to flash.
  M5.Display.fillRect(144, 96, 252, 42, TFT_WHITE);
  ui::Typography::apply(ui::TextRole::BodyBold, 0.84f); M5.Display.setTextDatum(ML_DATUM);
  M5.Display.drawString(String("SCORE ") + score_ + "  HI " + highScore_, 152, 117);
  M5.Display.setTextDatum(TL_DATUM); ui::Typography::reset();
}

void TetrisApp::drawNextPiece() {
  constexpr int PreviewLeft = 414, PreviewTop = 198, PreviewCell = 20;
  M5.Display.fillRect(PreviewLeft, PreviewTop, 96, 104, TFT_WHITE);
  for (int row = 0; row < 4; ++row) for (int col = 0; col < 4; ++col) {
    if (!filled(nextPiece_, 0, row, col)) continue;
    M5.Display.fillRect(PreviewLeft + 8 + col * PreviewCell, PreviewTop + 10 + row * PreviewCell,
                        PreviewCell - 3, PreviewCell - 3, TFT_BLACK);
  }
}

void TetrisApp::drawControls() {
  M5.Display.fillRect(18, 780, 504, 92, TFT_WHITE);
  const char* labels[] = {"LEFT", "ROTATE", "RIGHT", "DROP"};
  for (int i = 0; i < 4; ++i) {
    const int x = 18 + i * 130;
    gameButton(x, 780, 114, 64, labels[i]);
  }
  ui::Typography::apply(ui::TextRole::Caption); M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString("SWIPE BOARD OR USE CONTROLS", 270, 856);
  M5.Display.setTextDatum(TL_DATUM); ui::Typography::reset();
}

void TetrisApp::drawGameOver() {
  const int x = Left + 28, y = Top + (Rows * Cell) / 2 - 42, width = Columns * Cell - 56;
  M5.Display.fillRoundRect(x, y, width, 84, 10, TFT_BLACK); M5.Display.fillRoundRect(x + 3, y + 3, width - 6, 78, 7, TFT_WHITE);
  ui::Typography::apply(ui::TextRole::Heading, 0.90f); M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString("GAME OVER", x + width / 2, y + 30);
  ui::Typography::apply(ui::TextRole::Caption); M5.Display.drawString("TAP RESET TO PLAY AGAIN", x + width / 2, y + 60);
  M5.Display.setTextDatum(TL_DATUM); ui::Typography::reset();
}

void TetrisApp::draw(AppContext& context) {
  // This is intentionally only used when the app opens.  On an e-paper panel
  // clearing the whole screen for every falling block is both slow and noisy.
  M5.Display.fillScreen(TFT_WHITE);
  ui::Chrome::drawHeader(context, "Tetris");
  ui::Chrome::drawFooter(gameChrome());
  M5.Display.fillRoundRect(18, 82, 504, 674, 12, TFT_BLACK); M5.Display.fillRoundRect(21, 85, 498, 668, 9, TFT_WHITE);
  gameButton(30, 94, 108, 44, "RESET");
  M5.Display.fillRoundRect(408, 174, 108, 142, 8, TFT_BLACK); M5.Display.fillRoundRect(411, 177, 102, 136, 5, TFT_WHITE);
  ui::Typography::apply(ui::TextRole::Caption); M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString("NEXT", 462, 188); M5.Display.setTextDatum(TL_DATUM); ui::Typography::reset();
  drawGameArea();
  drawControls();
}

void TetrisApp::drawGameArea(bool refreshStatus) {
  // Keep Chrome and the button strip untouched after initial paint.  Falling
  // blocks therefore repaint only the board; the score strip changes solely
  // after a line clear, reset, or game over.
  drawInitialBoard();
  if (refreshStatus) drawStatus();
  drawNextPiece();
}

void TetrisApp::playTone(uint16_t frequency, uint16_t durationMs) const {
  M5.Speaker.tone(frequency, durationMs);
}

void TetrisApp::onTick(AppContext& context, uint32_t nowMs) {
  const auto& touch = M5.Touch.getDetail();
  if (touch.wasPressed()) {
    if (ui::Chrome::hitTestFooter(touch.x, touch.y, gameChrome()) != ui::FooterAction::None) {
      if (navigator_) navigator_(AppId::Launcher); return;
    }
    if (touch.x >= 30 && touch.x <= 138 && touch.y >= 94 && touch.y <= 138) {
      reset(); lastDropMs_ = nowMs; playTone(720, 70); drawGameArea(); return;
    }
    if (touch.y >= 780 && touch.y <= 844 && !gameOver_) {
      const uint16_t previousScore = score_;
      const bool wasGameOver = gameOver_;
      const int previousNext = nextPiece_;
      const int button = touch.x / 130;
      if (button == 0) { move(-1); playTone(320, 25); }
      else if (button == 1) { rotate(); playTone(620, 30); }
      else if (button == 2) { move(1); playTone(360, 25); }
      else drop();
      lastDropMs_ = nowMs;
      drawBoardDelta();
      if (score_ != previousScore || gameOver_ != wasGameOver) drawStatus();
      if (nextPiece_ != previousNext) drawNextPiece();
      return;
    }
  }
  // Read the complete gesture on release.  Restrict swipes to the playfield
  // so the fixed buttons and shared footer keep their normal tap behaviour.
  if (!gameOver_ && touch.wasReleased()
      && touch.base_x >= Left && touch.base_x < Left + Columns * Cell
      && touch.base_y >= Top && touch.base_y < Top + Rows * Cell) {
    const int dx = touch.distanceX();
    const int dy = touch.distanceY();
    constexpr int kSwipeDistance = 36;
    if (abs(dx) >= kSwipeDistance || abs(dy) >= kSwipeDistance) {
      const uint16_t previousScore = score_;
      const bool wasGameOver = gameOver_;
      const int previousNext = nextPiece_;
      if (abs(dx) > abs(dy)) {
        if (dx < 0) { move(-1); playTone(320, 25); }
        else { move(1); playTone(360, 25); }
      } else if (dy < 0) {
        rotate(); playTone(620, 30);
      } else {
        drop();
      }
      lastDropMs_ = nowMs;
      drawBoardDelta();
      if (score_ != previousScore || gameOver_ != wasGameOver) drawStatus();
      if (nextPiece_ != previousNext) drawNextPiece();
      return;
    }
  }
  if (!gameOver_ && nowMs - lastDropMs_ >= 900) {
    const uint16_t previousScore = score_;
    const bool wasGameOver = gameOver_;
    const int previousNext = nextPiece_;
    drop(); lastDropMs_ = nowMs;
    drawBoardDelta();
    if (score_ != previousScore || gameOver_ != wasGameOver) drawStatus();
    if (nextPiece_ != previousNext) drawNextPiece();
  }
}
